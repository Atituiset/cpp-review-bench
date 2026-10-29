# auto-postgres-74c87b9573

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#f400289946f7d0e66a4169cf4aa56079f1422a2b](https://github.com/postgres/postgres/commit/f400289946f7d0e66a4169cf4aa56079f1422a2b) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-29 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 152 |
| 编译错误数（gcc syntax-only） | 39（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #f400289946f7d0e66a4169cf4aa56079f1422a2b (https://github.com/postgres/postgres/commit/f400289946f7d0e66a4169cf4aa56079f1422a2b)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT f400289946f7d0e66a4169cf4aa56079f1422a2b Don't replace clone clauses with constant TRUE :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2156,6 +2156,12 @@ create_join_clause(PlannerInfo *root,
  * We handle this by generating a constant-TRUE clause that is marked with
  * the same required_relids etc as the removed outer-join clause, thus
  * making it a join clause between the correct relations.
+ *
+ * However, we can't do that for a clone clause, since it may be applied
+ * above a commuted outer join that has already null-extended OUTERVAR but
+ * whose relid has not yet been added to the joinrel.  The original strict
+ * clause is needed there to reject the null-extended rows, so we throw
+ * back a clone as-is, marked redundant for selectivity estimation.
  */
 void
 reconsider_outer_join_clauses(PlannerInfo *root)
@@ -2181,6 +2187,15 @@ reconsider_outer_join_clauses(PlannerInfo *root)
 				/* remove it from the list */
 				root->left_join_clauses =
 					foreach_delete_current(root->left_join_clauses, cell);
+				/* throw back a clone as-is (see notes above) */
+				if (rinfo->has_clone || rinfo->is_clone)
+				{
+					/* but the thrown-back clause has no extra selectivity */
+					rinfo->norm_selec = 2.0;
+					rinfo->outer_selec = 1.0;
+					distribute_restrictinfo_to_rels(root, rinfo);
+					continue;
+				}
 				/* throw back a dummy replacement clause (see notes above) */
 				rinfo = make_restrictinfo(root,
 										  (Expr *) makeBoolConst(true, false),
@@ -2209,6 +2224,15 @@ reconsider_outer_join_clauses(PlannerInfo *root)
 				/* remove it from the list */
 				root->right_join_clauses =
 					foreach_delete_current(root->right_join_clauses, cell);
+				/* throw back a clone as-is (see notes above) */
+				if (rinfo->has_clone || rinfo->is_clone)
+				{
+					/* but the thrown-back clause has no extra selectivity */
+					rinfo->norm_selec = 2.0;
+					rinfo->outer_selec = 1.0;
+					distribute_restrictinfo_to_rels(root, rinfo);
+					continue;
+				}
 				/* throw back a dummy replacement clause (see notes above) */
 				rinfo = make_restrictinfo(root,
 										  (Expr *) makeBoolConst(true, false),
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`canonicalize_ec_expression`
- 外部函数：`collation`
- 外部函数：`equal`
- 外部函数：`exprType`
- 外部函数：`foreach_current_index`
- 外部函数：`func_strict`
- 外部函数：`get_eclass_for_sort_expr`
- 外部函数：`get_leftop`
- 外部函数：`get_mergejoin_opfamilies`
- 外部函数：`get_rightop`
- 外部函数：`is_opclause`
- 外部函数：`lappend`
- 外部函数：`lfirst`
- 外部函数：`makeNode`
- 外部函数：`make_restrictinfo`
- 外部函数：`op_input_types`
- 外部函数：`set_opfuncid`
- 大写宏：`FALSE`
- 大写宏：`IS_NOT_NULL`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`README`
- 外部类型：`Add`
- 外部类型：`Child`
- 外部类型：`Clauses`
- 外部类型：`ECs`
- 外部类型：`Ensure`
- 外部类型：`EquivalenceClass`
- 外部类型：`EquivalenceClasses`
- 外部类型：`EquivalenceMember`
- 外部类型：`Expr`
- 外部类型：`Extract`
- 外部类型：`If`
- 外部类型：`It`
- 外部类型：`JoinDomain`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`Make`
- 外部类型：`Match`
- 外部类型：`Merge`
- 外部类型：`NULLs`
- 外部类型：`Never`
- 外部类型：`Node`
- 外部类型：`Note`
- 外部类型：`NullTest`
- 外部类型：`Oid`
- 外部类型：`OpExpr`
- 外部类型：`PlannerInfo`
- 外部类型：`Reject`
- 外部类型：`Relids`
- 外部类型：`RestrictInfo`
- 外部类型：`Should`
- 外部类型：`Since`
- 外部类型：`So`
- 外部类型：`Sweep`
- 外部类型：`The`
- 外部类型：`These`
- 外部类型：`This`
- 外部类型：`Use`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-74c87b9573` → 本草稿移入 `cases/defect/auto-postgres-74c87b9573/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
