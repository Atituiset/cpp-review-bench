# auto-postgres-e8c415a61c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#62e46c00c657fc2e092ceec0cd79d16035798afe](https://github.com/postgres/postgres/commit/62e46c00c657fc2e092ceec0cd79d16035798afe) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 28 |
| 编译错误数（gcc syntax-only） | 21（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #62e46c00c657fc2e092ceec0cd79d16035798afe (https://github.com/postgres/postgres/commit/62e46c00c657fc2e092ceec0cd79d16035798afe)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1247；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 62e46c00c657fc2e092ceec0cd79d16035798afe Fix ON CONFLICT inference for index expressions simplified to a Var :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -940,6 +940,8 @@ infer_arbiter_indexes(PlannerInfo *root)
 		Form_pg_index idxForm;
 		Bitmapset  *indexedAttrs;
 		List	   *idxExprs;
+		List	   *idxNonVarExprs;
+		ListCell   *idxExprItem;
 		List	   *predExprs;
 		AttrNumber	natt;
 		bool		match;
@@ -1044,31 +1046,54 @@ infer_arbiter_indexes(PlannerInfo *root)
 		if (idxForm->indisexclusion)
 			continue;
 
-		/* Build BMS representation of plain (non expression) index attrs */
+		/* Simplify the index expressions the same way arbiterElems were */
+		idxExprs = RelationGetIndexExpressions(idxRel);
+		if (idxExprs)
+		{
+			if (varno != 1)
+				ChangeVarNodes((Node *) idxExprs, 1, varno, 0);
+
+			idxExprs = (List *) eval_const_expressions(root, (Node *) idxExprs);
+		}
+
+		/*
+		 * Build BMS representation of plain index attrs.  An index expression
+		 * that was simplified to a plain Var is treated as a plain attr too,
+		 * to match how arbiterElems are classified above.
+		 */
 		indexedAttrs = NULL;
+		idxNonVarExprs = NIL;
+		idxExprItem = list_head(idxExprs);
 		for (natt = 0; natt < idxForm->indnkeyatts; natt++)
 		{
 			int			attno = idxRel->rd_index->indkey.values[natt];
 
-			if (attno != 0)
-				indexedAttrs = bms_add_member(indexedAttrs,
-											  attno - FirstLowInvalidHeapAttributeNumber);
-		}
+			if (attno == 0)
+			{
+				Node	   *idxExpr;
 
-		/* Non-expression attributes (if any) must match */
-		if (!bms_equal(indexedAttrs, inferAttrs))
-			continue;
+				if (idxExprItem == NULL)
+					elog(ERROR, "wrong number of index expressions");
+				idxExpr = (Node *) lfirst(idxExprItem);
+				idxExprItem = lnext(idxExprs, idxExprItem);
 
-		/* Expression attributes (if any) must match */
-		idxExprs = RelationGetIndexExpressions(idxRel);
-		if (idxExprs)
-		{
-			if (varno != 1)
-				ChangeVarNodes((Node *) idxExprs, 1, varno, 0);
+				if (!IsA(idxExpr, Var) || ((Var *) idxExpr)->varattno == 0)
+				{
+					idxNonVarExprs = lappend(idxNonVarExprs, idxExpr);
+					continue;
+				}
 
-			idxExprs = (List *) eval_const_expressions(root, (Node *) idxExprs);
+				attno = ((Var *) idxExpr)->varattno;
+			}
+
+			indexedAttrs = bms_add_member(indexedAttrs,
+										  attno - FirstLowInvalidHeapAttributeNumber);
 		}
 
+		/* Plain attributes (if any) must match */
+		if (!bms_equal(indexedAttrs, inferAttrs))
+			continue;
+
 		/* Check the arbiterElems against this index. */
 		match = true;
 		foreach_ptr(InferenceElem, elem, onconflict->arbiterElems)
@@ -1118,7 +1143,7 @@ infer_arbiter_indexes(PlannerInfo *root)
 		 * indexes redundantly repeat the same attribute, or if attributes
 		 * redundantly appear multiple times within an inference clause.
 		 */
-		if (list_difference(idxExprs, inferElems) != NIL)
+		if (list_difference(idxNonVarExprs, inferElems) != NIL)
 			continue;
 
 		predExprs = RelationGetIndexPredicate(idxRel);
@@ -1244,12 +1269,13 @@ infer_collation_opclass_match(InferenceElem *elem, Relation idxRel,
 		}
 
 		/* If one matching index att found, good enough -- return true */
-		if (IsA(elem->expr, Var))
+		if (attno != 0)
 		{
-			if (((Var *) elem->expr)->varattno == attno)
+			if (IsA(elem->expr, Var) &&
+				((Var *) elem->expr)->varattno == attno)
 				return true;
 		}
-		else if (attno == 0)
+		else
 		{
 			Node	   *nattExpr = list_nth(idxExprs, (natt - 1) - nplain);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`attributes`
- 外部函数：`bms_add_member`
- 外部函数：`bms_equal`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`eval_const_expressions`
- 外部函数：`lappend`
- 外部函数：`list_difference`
- 外部函数：`list_nth`
- 外部函数：`plain`
- 大写宏：`BMS`
- 大写宏：`ERRCODE_FEATURE_NOT_SUPPORTED`
- 大写宏：`ERROR`
- 大写宏：`NIL`
- 大写宏：`NULL`
- 外部类型：`AttrNumber`
- 外部类型：`Bitmapset`
- 外部类型：`Build`
- 外部类型：`Check`
- 外部类型：`Expression`
- 外部类型：`FirstLowInvalidHeapAttributeNumber`
- 外部类型：`Form_pg_index`
- 外部类型：`If`
- 外部类型：`InferenceElem`
- 外部类型：`List`
- 外部类型：`Node`
- 外部类型：`Non`
- 外部类型：`Var`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-postgres-e8c415a61c` → 本草稿移入 `cases/defect/auto-postgres-e8c415a61c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
