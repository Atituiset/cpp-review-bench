# auto-postgres-2f849a20d2

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#022a80fe6c1d82f7777bd52f9d3e5f9952533109](https://github.com/postgres/postgres/commit/022a80fe6c1d82f7777bd52f9d3e5f9952533109) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 45 |
| 编译错误数（gcc syntax-only） | 17（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #022a80fe6c1d82f7777bd52f9d3e5f9952533109 (https://github.com/postgres/postgres/commit/022a80fe6c1d82f7777bd52f9d3e5f9952533109)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 2980；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 022a80fe6c1d82f7777bd52f9d3e5f9952533109 Don't build child-join EC members that reference outside rels :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2864,7 +2864,6 @@ add_child_rel_equivalences(PlannerInfo *root,
 			{
 				/* OK, generate transformed child version */
 				Expr	   *child_expr;
-				Relids		new_relids;
 
 				if (parent_rel->reloptkind == RELOPT_BASEREL)
 				{
@@ -2885,20 +2884,17 @@ add_child_rel_equivalences(PlannerInfo *root,
 				}
 
 				/*
-				 * Transform em_relids to match.  Note we do *not* do
+				 * The member's relids are all within the parent, so the child
+				 * version's relids are just the child's.  Note we do *not* do
 				 * pull_varnos(child_expr) here, as for example the
 				 * transformation might have substituted a constant, but we
 				 * don't want the child member to be marked as constant.
 				 */
-				new_relids = bms_difference(cur_em->em_relids,
-											top_parent_relids);
-				new_relids = bms_add_members(new_relids, child_relids);
-
 				add_child_eq_member(root,
 									cur_ec,
 									i,
 									child_expr,
-									new_relids,
+									bms_copy(child_relids),
 									cur_em->em_jdomain,
 									cur_em,
 									cur_em->em_datatype,
@@ -2976,12 +2972,14 @@ add_child_join_rel_equivalences(PlannerInfo *root,
 			if (bms_membership(cur_em->em_relids) != BMS_MULTIPLE)
 				continue;
 
-			/* Does this member reference child's topmost parent rel? */
-			if (bms_overlap(cur_em->em_relids, top_parent_relids))
+			/*
+			 * Consider only members that can be computed at child's topmost
+			 * parent joinrel, as in add_child_rel_equivalences.
+			 */
+			if (bms_is_subset(cur_em->em_relids, top_parent_relids))
 			{
-				/* Yes, generate transformed child version */
+				/* OK, generate transformed child version */
 				Expr	   *child_expr;
-				Relids		new_relids;
 
 				if (parent_joinrel->reloptkind == RELOPT_JOINREL)
 				{
@@ -3002,16 +3000,6 @@ add_child_join_rel_equivalences(PlannerInfo *root,
 														  child_joinrel->top_parent);
 				}
 
-				/*
-				 * Transform em_relids to match.  Note we do *not* do
-				 * pull_varnos(child_expr) here, as for example the
-				 * transformation might have substituted a constant, but we
-				 * don't want the child member to be marked as constant.
-				 */
-				new_relids = bms_difference(cur_em->em_relids,
-											top_parent_relids);
-				new_relids = bms_add_members(new_relids, child_relids);
-
 				/*
 				 * Add new child member to the EquivalenceClass.  Because this
 				 * is a RELOPT_OTHER_JOINREL which has multiple component
@@ -3031,12 +3019,15 @@ add_child_join_rel_equivalences(PlannerInfo *root,
 				 * for all the component relids, then that would just result
 				 * in eclass_member_iterator_next() finding the member
 				 * multiple times, which is a waste of effort.
+				 *
+				 * As in add_child_rel_equivalences, the child member's relids
+				 * are just the child's.
 				 */
 				add_child_eq_member(root,
 									cur_ec,
 									-1,
 									child_expr,
-									new_relids,
+									bms_copy(child_relids),
 									cur_em->em_jdomain,
 									cur_em,
 									cur_em->em_datatype,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_add_member`
- 外部函数：`bms_add_members`
- 外部函数：`bms_difference`
- 外部函数：`bms_is_empty`
- 外部函数：`bms_membership`
- 外部函数：`bms_overlap`
- 外部函数：`eclass_member_iterator_next`
- 外部函数：`get_eclass_for_sort_expr`
- 外部函数：`lappend`
- 外部函数：`makeNode`
- 外部函数：`palloc0_array`
- 外部函数：`process_equivalence`
- 外部函数：`pull_varnos`
- 外部函数：`repalloc0_array`
- 外部函数：`unlikely`
- 大写宏：`BMS_MULTIPLE`
- 大写宏：`NULL`
- 大写宏：`RELOPT_BASEREL`
- 大写宏：`RELOPT_JOINREL`
- 大写宏：`RELOPT_OTHER_JOINREL`
- 大写宏：`WHERE`
- 外部类型：`Add`
- 外部类型：`Allocate`
- 外部类型：`Because`
- 外部类型：`But`
- 外部类型：`Does`
- 外部类型：`EquivalenceClass`
- 外部类型：`EquivalenceMember`
- 外部类型：`Expr`
- 外部类型：`Index`
- 外部类型：`JoinDomain`
- 外部类型：`List`
- 外部类型：`Lists`
- 外部类型：`No`
- 外部类型：`Note`
- 外部类型：`Oid`
- 外部类型：`PlannerInfo`
- 外部类型：`Record`
- 外部类型：`RelOptInfo`
- 外部类型：`Relids`
- 外部类型：`SRFs`
- 外部类型：`This`
- 外部类型：`Transform`
- 外部类型：`Vars`
- 外部类型：`We`
- 外部类型：`Yes`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-2f849a20d2` → 本草稿移入 `cases/defect/auto-postgres-2f849a20d2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
