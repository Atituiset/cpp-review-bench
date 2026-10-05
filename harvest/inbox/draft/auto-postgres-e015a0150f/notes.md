# auto-postgres-e015a0150f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#a464d31ff6213fe47505c750972b404e4517ebd9](https://github.com/postgres/postgres/commit/a464d31ff6213fe47505c750972b404e4517ebd9) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 9 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a464d31ff6213fe47505c750972b404e4517ebd9 (https://github.com/postgres/postgres/commit/a464d31ff6213fe47505c750972b404e4517ebd9)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 2032；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a464d31ff6213fe47505c750972b404e4517ebd9 Fix relids of EC-derived join clauses for lateral UNION ALL members :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2022,19 +2022,27 @@ create_join_clause(PlannerInfo *root,
 										ec->ec_min_security);
 
 	/*
-	 * If either EM is a child, force the clause's clause_relids to include
-	 * the relid(s) of the child rel.  In normal cases it would already, but
-	 * not if we are considering appendrel child relations with pseudoconstant
-	 * translated variables (i.e., UNION ALL sub-selects with constant output
-	 * items).  We must do this so that join_clause_is_movable_into() will
-	 * think that the clause should be evaluated at the correct place.
+	 * If either EM is a child, set the clause's clause_relids from the
+	 * members' em_relids rather than the relids found in the expressions.
+	 * These normally match, but not for UNION ALL sub-selects whose output
+	 * items are constants (mentioning no rels) or contain lateral references
+	 * (mentioning rels that the child's parameterization supplies).  We must
+	 * do this so that join_clause_is_movable_into() will think that the
+	 * clause should be evaluated at the correct place.
 	 */
-	if (leftem->em_is_child)
-		rinfo->clause_relids = bms_add_members(rinfo->clause_relids,
-											   leftem->em_relids);
-	if (rightem->em_is_child)
-		rinfo->clause_relids = bms_add_members(rinfo->clause_relids,
-											   rightem->em_relids);
+	if (leftem->em_is_child || rightem->em_is_child)
+	{
+		Relids		baserels;
+
+		rinfo->clause_relids = bms_union(leftem->em_relids,
+										 rightem->em_relids);
+
+		/* keep num_base_rels in sync, as in make_restrictinfo() */
+		baserels = bms_difference(rinfo->clause_relids,
+								  root->outer_join_rels);
+		rinfo->num_base_rels = bms_num_members(baserels);
+		bms_free(baserels);
+	}
 
 	/* If it's a child clause, copy the parent's rinfo_serial */
 	if (parent_rinfo)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_add_members`
- 外部函数：`join_clause_is_movable_into`
- 外部函数：`relid`
- 外部函数：`variables`
- 大写宏：`ALL`
- 大写宏：`UNION`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`We`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-postgres-e015a0150f` → 本草稿移入 `cases/defect/auto-postgres-e015a0150f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
