# auto-postgres-522bcce14b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#91b1872ff2de16cd353598edbc650a185e649226](https://github.com/postgres/postgres/commit/91b1872ff2de16cd353598edbc650a185e649226) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 143 |
| 编译错误数（gcc syntax-only） | 19（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #91b1872ff2de16cd353598edbc650a185e649226 (https://github.com/postgres/postgres/commit/91b1872ff2de16cd353598edbc650a185e649226)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 91b1872ff2de16cd353598edbc650a185e649226 Clear more substructure of a dead relation during join removal. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -151,13 +151,15 @@ remove_useless_outer_joins(PlannerInfo *root)
 		 * As in pull_up_simple_subquery, discard no-longer-needed subqueries.
 		 * This is not just an optimization, but is necessary to prevent
 		 * subsequent processing from descending into stale subtrees and
-		 * seeing inconsistent data.  Likewise discard any securityQuals of
-		 * the removed rel.  (Although simple_rte_array[] will be rebuilt
-		 * shortly, we can still use it to find the RTE in the parse tree.)
+		 * seeing inconsistent data.  For the same reason, discard any
+		 * tablesample and securityQuals subtrees of the removed rel.
+		 * (Although simple_rte_array[] will be rebuilt shortly, we can still
+		 * use it to find the RTE in the parse tree.)
 		 */
 		rte = root->simple_rte_array[innerrelid];
 		if (rte->rtekind == RTE_SUBQUERY)
 			rte->subquery = NULL;
+		rte->tablesample = NULL;
 		rte->securityQuals = NIL;
 
 		/*
@@ -1269,6 +1271,13 @@ remove_self_join_rel(PlannerInfo *root,
 		ChangeVarNodes((Node *) root->append_rel_list, toRemove->relid,
 					   toKeep->relid, 0);
 
+	/*
+	 * As in remove_useless_outer_joins, discard the securityQuals of the
+	 * removed rel, so that later passes don't visit them.  We know the kept
+	 * rel has the same quals.  (tablesample is already known NULL.)
+	 */
+	root->simple_rte_array[toRemove->relid]->securityQuals = NIL;
+
 	/* Clean up the quals that the substitution has messed with */
 	fixup_selfjoin_jointree(root, (Node *) root->parse->jointree,
 							toKeep->relid,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_add_member`
- 外部函数：`bms_copy`
- 外部函数：`bms_equal`
- 外部函数：`bms_get_singleton_member`
- 外部函数：`bms_is_member`
- 外部函数：`bms_is_subset`
- 外部函数：`bms_overlap`
- 外部函数：`bms_singleton_member`
- 外部函数：`bms_union`
- 外部函数：`elog`
- 外部函数：`find_base_rel`
- 外部函数：`lfirst`
- 外部函数：`rel_supports_distinctness`
- 外部函数：`remove_join_from_jointree`
- 大写宏：`ERROR`
- 大写宏：`JOIN_LEFT`
- 大写宏：`MERGE`
- 大写宏：`NIL`
- 大写宏：`NULL`
- 大写宏：`PHV`
- 大写宏：`RTE`
- 大写宏：`RTE_SUBQUERY`
- 大写宏：`SQL`
- 外部类型：`Although`
- 外部类型：`As`
- 外部类型：`Before`
- 外部类型：`Compute`
- 外部类型：`FromExpr`
- 外部类型：`Here`
- 外部类型：`If`
- 外部类型：`It`
- 外部类型：`JoinExpr`
- 外部类型：`Likewise`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`Must`
- 外部类型：`Never`
- 外部类型：`Node`
- 外部类型：`PHVs`
- 外部类型：`PlaceHolderInfo`
- 外部类型：`PlaceHolderVars`
- 外部类型：`PlannerInfo`
- 外部类型：`RangeTblEntry`
- 外部类型：`RelOptInfo`
- 外部类型：`Relids`
- 外部类型：`Removing`
- 外部类型：`Similarly`
- 外部类型：`Skip`
- 外部类型：`So`
- 外部类型：`SpecialJoinInfo`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Track`
- 外部类型：`We`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-522bcce14b` → 本草稿移入 `cases/defect/auto-postgres-522bcce14b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
