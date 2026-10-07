# auto-postgres-0bef4c4ce7

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#372761e05a62856d32fb6a9811edf1c51a286b9c](https://github.com/postgres/postgres/commit/372761e05a62856d32fb6a9811edf1c51a286b9c) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-07 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 93 |
| 编译错误数（gcc syntax-only） | 28（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #372761e05a62856d32fb6a9811edf1c51a286b9c (https://github.com/postgres/postgres/commit/372761e05a62856d32fb6a9811edf1c51a286b9c)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 372761e05a62856d32fb6a9811edf1c51a286b9c Fix parser failure with whole-row join alias Vars :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -776,6 +776,9 @@ pull_var_clause_walker(Node *node, pull_var_clause_context *context)
  * PlaceHolderVar or constructed from those, we can just add the
  * varnullingrels bits to the existing nullingrels field(s); otherwise
  * we have to add a PlaceHolderVar wrapper.
+ *
+ * If root is NULL, nulled whole-row JOIN Vars are left unexpanded, so the
+ * result must be flattened again with a root before it can be executed.
  */
 Node *
 flatten_join_alias_vars(PlannerInfo *root, Query *query, Node *node)
@@ -809,7 +812,8 @@ flatten_join_alias_vars(PlannerInfo *root, Query *query, Node *node)
  * PlaceHolderVars.  We can avoid making PlaceHolderVars in the parser's
  * usage because it won't be dealing with arbitrary expressions: so long as
  * adjust_standard_join_alias_expression can handle everything the parser
- * would make as a join alias expression, we're OK.
+ * would make as a join alias expression, we're OK.  (Nulled whole-row join
+ * Vars are the exception; those are left unexpanded.)
  *
  * The "node" might be part of a sub-query of the Query whose join alias
  * Vars are to be expanded.  "sublevels_up" indicates how far below the
@@ -865,6 +869,13 @@ flatten_join_alias_vars_mutator(Node *node,
 			ListCell   *lv;
 			ListCell   *ln;
 
+			/*
+			 * A nulled whole-row expansion needs a PlaceHolderVar, which we
+			 * can't make without a root; leave the Var unexpanded.
+			 */
+			if (context->root == NULL && var->varnullingrels != NULL)
+				return node;
+
 			Assert(list_length(rte->joinaliasvars) == list_length(rte->eref->colnames));
 			forboth(lv, rte->joinaliasvars, ln, rte->eref->colnames)
 			{
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_add_member`
- 外部函数：`bms_add_members`
- 外部函数：`bms_difference`
- 外部函数：`bms_equal`
- 外部函数：`bms_join`
- 外部函数：`expression_tree_walker`
- 外部函数：`field`
- 外部函数：`flatten_join_alias_vars_mutator`
- 外部函数：`level`
- 外部函数：`query_or_expression_tree_walker`
- 外部函数：`query_tree_walker`
- 大写宏：`LATERAL`
- 大写宏：`NULL`
- 大写宏：`PHV`
- 大写宏：`RTE`
- 外部类型：`Another`
- 外部类型：`But`
- 外部类型：`CurrentOfExpr`
- 外部类型：`Hence`
- 外部类型：`However`
- 外部类型：`Ideally`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Must`
- 外部类型：`No`
- 外部类型：`Node`
- 外部类型：`Normal`
- 外部类型：`PlaceHolderInfo`
- 外部类型：`PlaceHolderVar`
- 外部类型：`PlaceHolderVars`
- 外部类型：`PlannerInfo`
- 外部类型：`Query`
- 外部类型：`Recurse`
- 外部类型：`Relids`
- 外部类型：`SubLink`
- 外部类型：`SubLinks`
- 外部类型：`Translated`
- 外部类型：`Typically`
- 外部类型：`Var`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-0bef4c4ce7` → 本草稿移入 `cases/defect/auto-postgres-0bef4c4ce7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
