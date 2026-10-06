# auto-postgres-f898dff625

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#6b858a36ddb80bfa4f478afebe3e84de72600e02](https://github.com/postgres/postgres/commit/6b858a36ddb80bfa4f478afebe3e84de72600e02) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-06 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 245 |
| 编译错误数（gcc syntax-only） | 53（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #6b858a36ddb80bfa4f478afebe3e84de72600e02 (https://github.com/postgres/postgres/commit/6b858a36ddb80bfa4f478afebe3e84de72600e02)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 384；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 6b858a36ddb80bfa4f478afebe3e84de72600e02 Don't reprocess PHV copies within outer aggregates' arguments :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -50,6 +50,7 @@ typedef struct process_sublinks_context
 {
 	PlannerInfo *root;
 	bool		isTopQual;
+	bool		skipPHVs;		/* don't descend into PlaceHolderVars? */
 } process_sublinks_context;
 
 typedef struct finalize_primnode_context
@@ -376,12 +377,20 @@ build_subplan(PlannerInfo *root, Plan *plan, Path *path,
 		 * comments for SS_replace_correlation_vars).  Do that now.  A
 		 * PlaceHolderVar needs no such treatment: subquery_planner already
 		 * preprocessed the PHVs of its owning level, so its expression is
-		 * fully processed and may already contain SubPlans.
+		 * fully processed and may already contain SubPlans.  The same goes
+		 * for any PlaceHolderVars within the arguments, so skip those.
 		 */
 		if (IsA(arg, Aggref) ||
 			IsA(arg, GroupingFunc) ||
 			IsA(arg, ReturningExpr))
-			arg = SS_process_sublinks(root, arg, false);
+		{
+			process_sublinks_context context;
+
+			context.root = root;
+			context.isTopQual = false;
+			context.skipPHVs = true;
+			arg = process_sublinks_mutator(arg, &context);
+		}
 
 		splan->parParam = lappend_int(splan->parParam, pitem->paramId);
 		splan->args = lappend(splan->args, arg);
@@ -2217,6 +2226,7 @@ SS_process_sublinks(PlannerInfo *root, Node *expr, bool isQual)
 
 	context.root = root;
 	context.isTopQual = isQual;
+	context.skipPHVs = false;
 	return process_sublinks_mutator(expr, &context);
 }
 
@@ -2226,6 +2236,7 @@ process_sublinks_mutator(Node *node, process_sublinks_context *context)
 	process_sublinks_context locContext;
 
 	locContext.root = context->root;
+	locContext.skipPHVs = context->skipPHVs;
 
 	if (node == NULL)
 		return NULL;
@@ -2259,10 +2270,11 @@ process_sublinks_mutator(Node *node, process_sublinks_context *context)
 	 * ReturningExpr they'll be handled when build_subplan collects it into
 	 * the arguments to be passed down to the current subplan, while an outer
 	 * PHV's expression has already been preprocessed by its owning level.
+	 * That also holds for a PHV within such an argument (skipPHVs).
 	 */
 	if (IsA(node, PlaceHolderVar))
 	{
-		if (((PlaceHolderVar *) node)->phlevelsup > 0)
+		if (((PlaceHolderVar *) node)->phlevelsup > 0 || context->skipPHVs)
 			return node;
 	}
 	else if (IsA(node, Aggref))
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`behavior`
- 外部函数：`bms_add_member`
- 外部函数：`canonicalize_qual`
- 外部函数：`castNode`
- 外部函数：`choose_plan_name`
- 外部函数：`contain_aggs_of_level`
- 外部函数：`contain_exec_param`
- 外部函数：`contain_subplans`
- 外部函数：`contain_var_clause`
- 外部函数：`contain_vars_of_level`
- 外部函数：`contain_volatile_functions`
- 外部函数：`copyObject`
- 外部函数：`cost_subplan`
- 外部函数：`create_plan`
- 外部函数：`either`
- 外部函数：`elog`
- 外部函数：`eval_const_expressions`
- 外部函数：`exprCollation`
- 外部函数：`exprType`
- 外部函数：`exprTypmod`
- 外部函数：`expression_tree_mutator`
- 外部函数：`fetch_upper_rel`
- 外部函数：`format_type_be`
- 外部函数：`func_strict`
- 外部函数：`generate_new_exec_param`
- 外部函数：`get_cheapest_fractional_path`
- 外部函数：`get_commutator`
- 外部函数：`get_hash_memory_limit`
- 外部函数：`get_promoted_array_type`
- 外部函数：`input`
- 外部函数：`int8`
- 外部函数：`is_andclause`
- 外部函数：`is_orclause`
- 外部函数：`lappend`
- 外部函数：`lappend_int`
- 外部函数：`lappend_oid`
- 外部函数：`lfirst`
- 外部函数：`lfirst_oid`
- 外部函数：`linitial`
- 外部函数：`linitial_node`
- 外部函数：`list_concat`
- 外部函数：`list_copy`
- 外部函数：`list_length`
- 外部函数：`list_make1_int`
- 外部函数：`list_make2`
- 外部函数：`list_nth`
- 外部函数：`list_nth_cell`
- 外部函数：`lsecond`
- 外部函数：`makeNode`
- 外部函数：`makeNullConst`
- 外部函数：`makeTargetEntry`
- 外部函数：`make_andclause`
- 外部函数：`make_ands_explicit`
- 外部函数：`make_ands_implicit`
- 外部函数：`make_opclause`
- 外部函数：`make_orclause`
- 外部函数：`materialize_finished_plan`
- 外部函数：`nodeTag`
- 外部函数：`op_hashjoinable`
- 外部函数：`preprocess_expression`
- 外部函数：`processed`
- 外部函数：`subplan`
- 外部函数：`subquery_planner`
- 大写宏：`ALL`
- 大写宏：`ALL_SUBLINK`
- 大写宏：`AND`
- 大写宏：`ANY`
- 大写宏：`ANY_SUBLINK`
- 大写宏：`ARRAY`
- 大写宏：`ARRAY_EQ_OP`
- 大写宏：`ARRAY_SUBLINK`
- 大写宏：`BOOLOID`
- 大写宏：`CASE`
- 大写宏：`CMD_SELECT`
- 大写宏：`CTE_SUBLINK`
- 大写宏：`DISTINCT`
- 大写宏：`ELSE`
- 大写宏：`END`
- 大写宏：`ERROR`
- 大写宏：`EXISTS`
- 大写宏：`EXISTS_SUBLINK`
- 大写宏：`EXPR`
- 大写宏：`EXPR_SUBLINK`
- 大写宏：`FALSE`
- 大写宏：`FOR`
- 大写宏：`GETSTRUCT`
- 大写宏：`GROUP`
- 大写宏：`HAVING`
- 大写宏：`INT8OID`
- 大写宏：`JOIN`
- 大写宏：`LHS`
- 大写宏：`LIMIT`
- 大写宏：`MULTIEXPR`
- 大写宏：`MULTIEXPR_SUBLINK`
- 大写宏：`MULTIRANGE_EQ_OP`
- 大写宏：`NIL`
- 大写宏：`NOTE`
- 大写宏：`NULL`
- 大写宏：`OFFSET`
- 大写宏：`OPEROID`
- 大写宏：`ORDER`
- 大写宏：`PARAM_EXEC`
- 大写宏：`PARAM_MULTIEXPR`
- 大写宏：`PARAM_SUBLINK`
- 大写宏：`PHV`
- 大写宏：`RANGE_EQ_OP`
- 大写宏：`RECORDOID`
- 大写宏：`RECORD_EQ_OP`
- 大写宏：`REWIND`
- 大写宏：`RHS`
- 大写宏：`ROWCOMPARE`
- 大写宏：`ROWCOMPARE_SUBLINK`
- 大写宏：`RTE`
- 大写宏：`RTE_GROUP`
- 大写宏：`RTE_RESULT`
- 大写宏：`SELECT`
- 大写宏：`SHARE`
- 大写宏：`SIZE_MAX`
- 大写宏：`SUM`
- 大写宏：`THEN`
- 大写宏：`UPDATE`
- 大写宏：`UPPERREL_FINAL`
- 大写宏：`VOID`
- 大写宏：`VOIDOID`
- 大写宏：`WHEN`
- 大写宏：`WHERE`
- 大写宏：`WINDOW`
- 大写宏：`XXX`
- 外部类型：`Add`
- 外部类型：`Adjust`
- 外部类型：`Agg`
- 外部类型：`Aggref`
- 外部类型：`Aggrefs`
- 外部类型：`Aggs`
- 外部类型：`Also`
- 外部类型：`AlternativeSubPlan`
- 外部类型：`And`
- 外部类型：`Any`
- 外部类型：`Anything`
- 外部类型：`Anywhere`
- 外部类型：`Aside`
- 外部类型：`At`
- 外部类型：`AttrNumber`
- 外部类型：`Because`
- 外部类型：`Bitmapset`
- 外部类型：`BoolExpr`
- 外部类型：`Build`
- 外部类型：`But`
- 外部类型：`CTEs`
- 外部类型：`Caller`
- 外部类型：`Check`
- 外部类型：`Clean`
- 外部类型：`Concurrently`
- 外部类型：`Consider`
- 外部类型：`Const`
- 外部类型：`Copy`
- 外部类型：`Couldn`
- 外部类型：`DBMSes`
- 外部类型：`Determining`
- 外部类型：`Do`
- 外部类型：`Don`
- 外部类型：`Due`
- 外部类型：`Expr`
- 外部类型：`First`
- 外部类型：`For`
- 外部类型：`Form_pg_operator`
- 外部类型：`Furthermore`
- 外部类型：`Generate`
- 外部类型：`GroupingFunc`
- 外部类型：`HeapTuple`
- 外部类型：`However`
- 外部类型：`IDs`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`InitPlan`
- 外部类型：`Initialize`
- 外部类型：`InvalidOid`
- 外部类型：`Isolate`
- 外部类型：`It`
- 外部类型：`Lastly`
- 外部类型：`Leave`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`Make`
- 外部类型：`Material`
- 外部类型：`Might`
- 外部类型：`Node`
- 外部类型：`Nodes`
- 外部类型：`Non`
- 外部类型：`Note`
- 外部类型：`Now`
- 外部类型：`Oid`
- 外部类型：`Okay`
- 外部类型：`OpExpr`
- 外部类型：`OpExprs`
- 外部类型：`Ordinarily`
- 外部类型：`Otherwise`
- 外部类型：`PARAM_SUBLINKs`
- 外部类型：`PHVs`
- 外部类型：`Param`
- 外部类型：`Params`
- 外部类型：`Path`
- 外部类型：`Paths`
- 外部类型：`PlaceHolderVar`
- 外部类型：`Plan`
- 外部类型：`PlannerInfo`
- 外部类型：`PlannerParamItem`
- 外部类型：`Postgres`
- 外部类型：`Put`
- 外部类型：`Query`
- 外部类型：`RangeTblEntry`
- 外部类型：`RelOptInfo`
- 外部类型：`ReturningExpr`
- 外部类型：`SRFs`
- 外部类型：`SS_process_sublinks`
- 外部类型：`SS_replace_correlation_vars`
- 外部类型：`Save`
- 外部类型：`See`
- 外部类型：`Select`
- 外部类型：`Separate`
- 外部类型：`Since`
- 外部类型：`Size`
- 外部类型：`So`
- 外部类型：`Still`
- 外部类型：`SubLink`
- 外部类型：`SubLinkType`
- 外部类型：`SubLinks`
- 外部类型：`SubPlan`
- 外部类型：`SubPlans`
- 外部类型：`Such`
- 外部类型：`T_PlannerInfo`
- 外部类型：`TargetEntry`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`Therefore`
- 外部类型：`They`
- 外部类型：`This`
- 外部类型：`To`
- 外部类型：`Try`
- 外部类型：`Un`
- 外部类型：`Use`
- 外部类型：`Var`
- 外部类型：`Vars`
- 外部类型：`We`
- 外部类型：`Whether`
- 外部类型：`Without`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-f898dff625` → 本草稿移入 `cases/defect/auto-postgres-f898dff625/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
