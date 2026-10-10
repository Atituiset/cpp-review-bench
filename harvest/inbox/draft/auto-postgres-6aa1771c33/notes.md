# auto-postgres-6aa1771c33

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#d8ce0050a6cff3efefa5fce8abe3c6df7bd492ce](https://github.com/postgres/postgres/commit/d8ce0050a6cff3efefa5fce8abe3c6df7bd492ce) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 416 |
| 编译错误数（gcc syntax-only） | 81（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d8ce0050a6cff3efefa5fce8abe3c6df7bd492ce (https://github.com/postgres/postgres/commit/d8ce0050a6cff3efefa5fce8abe3c6df7bd492ce)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 3271；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d8ce0050a6cff3efefa5fce8abe3c6df7bd492ce Preserve typmod and collation when reducing an expression to its input :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3262,13 +3262,23 @@ eval_const_expressions_mutator(Node *node,
 				/* Copy the node and const-simplify its arguments */
 				expr = (NullIfExpr *) ece_generic_processing(node);
 
-				/* If either argument is NULL they can't be equal */
+				/*
+				 * If either argument is NULL they can't be equal, so the
+				 * result is the first argument, relabeled if needed to expose
+				 * the NULLIF's typmod and collation
+				 */
 				foreach(arg, expr->args)
 				{
 					if (!IsA(lfirst(arg), Const))
 						has_nonconst_input = true;
 					else if (((Const *) lfirst(arg))->constisnull)
-						return (Node *) linitial(expr->args);
+						return applyRelabelType((Node *) linitial(expr->args),
+												expr->opresulttype,
+												exprTypmod(node),
+												expr->opcollid,
+												COERCE_IMPLICIT_CAST,
+												-1,
+												false);
 				}
 
 				/*
@@ -3750,10 +3760,17 @@ eval_const_expressions_mutator(Node *node,
 
 				/*
 				 * If no non-FALSE alternatives, CASE reduces to the default
-				 * result
+				 * result, relabeled if needed to expose the CASE's typmod and
+				 * collation
 				 */
 				if (newargs == NIL)
-					return defresult;
+					return applyRelabelType(defresult,
+											caseexpr->casetype,
+											exprTypmod(node),
+											caseexpr->casecollid,
+											COERCE_IMPLICIT_CAST,
+											-1,
+											false);
 				/* Otherwise we need a new CASE node */
 				newcase = makeNode(CaseExpr);
 				newcase->casetype = caseexpr->casetype;
@@ -3820,26 +3837,19 @@ eval_const_expressions_mutator(Node *node,
 
 					/*
 					 * We can remove null constants from the list.  For a
-					 * nonnullable expression, if it has not been preceded by
-					 * any non-null-constant expressions then it is the
-					 * result.  Otherwise, it's the next argument, but we can
-					 * drop following arguments since they will never be
-					 * reached.
+					 * nonnullable expression, we can drop following arguments
+					 * since they will never be reached.
 					 */
 					if (IsA(e, Const))
 					{
 						if (((Const *) e)->constisnull)
 							continue;	/* drop null constant */
-						if (newargs == NIL)
-							return e;	/* first expr */
 						newargs = lappend(newargs, e);
 						break;
 					}
 					if (expr_is_nonnullable(context->root, (Expr *) e,
 											NOTNULL_SOURCE_HASHTABLE))
 					{
-						if (newargs == NIL)
-							return e;	/* first expr */
 						newargs = lappend(newargs, e);
 						break;
 					}
@@ -3858,10 +3868,18 @@ eval_const_expressions_mutator(Node *node,
 
 				/*
 				 * If there's exactly one surviving argument, we no longer
-				 * need COALESCE at all: the result is that argument
+				 * need COALESCE at all: the result is that argument,
+				 * relabeled if needed to expose the COALESCE's typmod and
+				 * collation
 				 */
 				if (list_length(newargs) == 1)
-					return (Node *) linitial(newargs);
+					return applyRelabelType((Node *) linitial(newargs),
+											coalesceexpr->coalescetype,
+											exprTypmod(node),
+											coalesceexpr->coalescecollid,
+											COERCE_IMPLICIT_CAST,
+											-1,
+											false);
 
 				newcoalesce = makeNode(CoalesceExpr);
 				newcoalesce->coalescetype = coalesceexpr->coalescetype;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`applyRelabelType`
- 外部函数：`args`
- 外部函数：`array`
- 外部函数：`behavior`
- 外部函数：`bms_is_empty`
- 外部函数：`bms_is_member`
- 外部函数：`castNode`
- 外部函数：`check_functions_in_node`
- 外部函数：`check_sql_fn_retval`
- 外部函数：`check_stack_depth`
- 外部函数：`contain_context_dependent_node_walker`
- 外部函数：`contain_nonstrict_functions_walker`
- 外部函数：`contain_subplans_walker`
- 外部函数：`contain_volatile_functions_walker`
- 外部函数：`copyObject`
- 外部函数：`cost_qual_eval`
- 外部函数：`datumCopy`
- 外部函数：`definition`
- 外部函数：`ece_all_arguments_const`
- 外部函数：`ece_evaluate_expr`
- 外部函数：`elog`
- 外部函数：`empty`
- 外部函数：`enforce_generic_type_consistency`
- 外部函数：`ereport`
- 外部函数：`errcontext`
- 外部函数：`errposition`
- 外部函数：`eval_const_expressions_mutator`
- 外部函数：`exec_simple_check_plan`
- 外部函数：`exprType`
- 外部函数：`exprTypmod`
- 外部函数：`expression_tree_mutator`
- 外部函数：`expression_tree_walker`
- 外部函数：`field`
- 外部函数：`find_base_rel`
- 外部函数：`find_relation_notnullatts`
- 外部函数：`fix_opfuncids`
- 外部函数：`free_parsestate`
- 外部函数：`func_volatile`
- 外部函数：`function`
- 外部函数：`getTypeInputInfo`
- 外部函数：`getTypeOutputInfo`
- 外部函数：`get_expr_result_type`
- 外部函数：`get_func_support`
- 外部函数：`get_typlenbyval`
- 外部函数：`geterrposition`
- 外部函数：`has_subclass`
- 外部函数：`heap_attisnull`
- 外部函数：`internalerrposition`
- 外部函数：`internalerrquery`
- 外部函数：`invalid`
- 外部函数：`is_andclause`
- 外部函数：`is_orclause`
- 外部函数：`jspIsMutable`
- 外部函数：`lappend`
- 外部函数：`lappend_oid`
- 外部函数：`lfirst`
- 外部函数：`lfirst_node`
- 外部函数：`linitial`
- 外部函数：`linitial_node`
- 外部函数：`list_concat_copy`
- 外部函数：`list_copy`
- 外部函数：`list_delete_first`
- 外部函数：`list_delete_first_n`
- 外部函数：`list_delete_last`
- 外部函数：`list_free`
- 外部函数：`list_length`
- 外部函数：`list_make1`
- 外部函数：`list_make3`
- 外部函数：`list_member_oid`
- 外部函数：`list_nth`
- 外部函数：`lookup_rowtype_tupdesc_domain`
- 外部函数：`lsecond`
- 外部函数：`makeBoolConst`
- 外部函数：`makeConst`
- 外部函数：`makeJsonValueExpr`
- 外部函数：`makeNode`
- 外部函数：`makeNullConst`
- 外部函数：`makeVar`
- 外部函数：`make_andclause`
- 外部函数：`make_fn_arguments`
- 外部函数：`make_notclause`
- 外部函数：`make_orclause`
- 外部函数：`make_parsestate`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`negate_clause`
- 外部函数：`negated`
- 外部函数：`nodeTag`
- 外部函数：`object_aclcheck`
- 外部函数：`operator`
- 外部函数：`palloc0_array`
- 外部函数：`paramFetch`
- 外部函数：`parameters`
- 外部函数：`pfree`
- 外部函数：`pg_parse_query`
- 外部函数：`planner_rt_fetch`
- 外部函数：`prepare_sql_fn_parse_info`
- 外部函数：`query_tree_walker`
- 外部函数：`record_plan_function_dependency`
- 外部函数：`record_plan_type_dependency`
- 外部函数：`result`
- 外部函数：`scalar`
- 外部函数：`set_opfuncid`
- 外部函数：`set_sa_opfuncid`
- 外部函数：`sql_fn_parser_setup`
- 外部函数：`stringToNode`
- 外部函数：`substitute_actual_parameters_mutator`
- 外部函数：`table_close`
- 外部函数：`table_open`
- 外部函数：`to_json_is_immutable`
- 外部函数：`to_jsonb_is_immutable`
- 外部函数：`transformTopLevelStmt`
- 大写宏：`ACLCHECK_OK`
- 大写宏：`ACL_EXECUTE`
- 大写宏：`ALLOCSET_DEFAULT_SIZES`
- 大写宏：`ALTER`
- 大写宏：`AND`
- 大写宏：`AND_EXPR`
- 大写宏：`API`
- 大写宏：`ARRAY`
- 大写宏：`ARR_DATA_PTR`
- 大写宏：`ARR_DIMS`
- 大写宏：`ARR_ELEMTYPE`
- 大写宏：`ARR_HASNULL`
- 大写宏：`ARR_NDIM`
- 大写宏：`ATTNULLABLE_VALID`
- 大写宏：`BOOLOID`
- 大写宏：`CASE`
- 大写宏：`CMD_SELECT`
- 大写宏：`COALESCE`
- 大写宏：`COERCE_EXPLICIT_CALL`
- 大写宏：`COERCE_IMPLICIT_CAST`
- 大写宏：`COLUMN`
- 大写宏：`CSTRINGOID`
- 大写宏：`DELETE`
- 大写宏：`DISTINCT`
- 大写宏：`ELSE`
- 大写宏：`END`
- 大写宏：`ERROR`
- 大写宏：`FALSE`
- 大写宏：`FROM`
- 大写宏：`FUNC_MAX_ARGS`
- 大写宏：`GETSTRUCT`
- 大写宏：`INSERT`
- 大写宏：`INT4OID`
- 大写宏：`IS_DOCUMENT`
- 大写宏：`IS_FALSE`
- 大写宏：`IS_NOT_FALSE`
- 大写宏：`IS_NOT_NULL`
- 大写宏：`IS_NOT_TRUE`
- 大写宏：`IS_NOT_UNKNOWN`
- 大写宏：`IS_NULL`
- 大写宏：`IS_TRUE`
- 大写宏：`IS_UNKNOWN`
- 大写宏：`IS_XMLCONCAT`
- 大写宏：`IS_XMLELEMENT`
- 大写宏：`IS_XMLFOREST`
- 大写宏：`IS_XMLPARSE`
- 大写宏：`IS_XMLPI`
- 大写宏：`IS_XMLROOT`
- 大写宏：`IS_XMLSERIALIZE`
- 大写宏：`JSCTOR_JSON_ARRAY_QUERY`
- 大写宏：`JSONPATHOID`
- 大写宏：`JSON_ARRAYAGG`
- 大写宏：`JS_FORMAT_JSONB`
- 大写宏：`NEW`
- 大写宏：`NIL`
- 大写宏：`NOT`
- 大写宏：`NOTNULL_SOURCE_CATALOG`
- 大写宏：`NOTNULL_SOURCE_HASHTABLE`
- 大写宏：`NOTNULL_SOURCE_RELOPT`
- 大写宏：`NOT_EXPR`
- 大写宏：`NULL`
- 大写宏：`OID`
- 大写宏：`OIDOID`
- 大写宏：`OLD`
- 大写宏：`OR_EXPR`
- 大写宏：`OUT`
- 大写宏：`PARAM_EXTERN`
- 大写宏：`PARAM_FLAG_CONST`
- 大写宏：`PG_DETOAST_DATUM_COPY`
- 大写宏：`PHV`
- 大写宏：`PROCOID`
- 大写宏：`PROKIND_FUNCTION`
- 大写宏：`PROVOLATILE_IMMUTABLE`
- 大写宏：`PROVOLATILE_STABLE`
- 大写宏：`RECORD`
- 大写宏：`RECORDOID`
- 大写宏：`RELKIND_PARTITIONED_TABLE`
- 大写宏：`ROW`
- 大写宏：`RTE_RELATION`
- 大写宏：`SELECT`
- 大写宏：`SQL`
- 大写宏：`THEN`
- 大写宏：`TOAST`
- 大写宏：`TRUE`
- 大写宏：`TYPE`
- 大写宏：`VALID`
- 大写宏：`VAR_RETURNING_DEFAULT`
- 大写宏：`VOID`
- 大写宏：`WHEN`
- 大写宏：`XML`
- 外部类型：`ANDs`
- 外部类型：`Additional`
- 外部类型：`Aggref`
- 外部类型：`All`
- 外部类型：`Also`
- 外部类型：`An`
- 外部类型：`And`
- 外部类型：`Anum_pg_proc_proallargtypes`
- 外部类型：`Anum_pg_proc_proargdefaults`
- 外部类型：`Anum_pg_proc_proconfig`
- 外部类型：`Anum_pg_proc_prosqlbody`
- 外部类型：`Anum_pg_proc_prosrc`
- 外部类型：`Argument`
- 外部类型：`Array`
- 外部类型：`ArrayCoerceExpr`
- 外部类型：`ArrayExpr`
- 外部类型：`ArrayType`
- 外部类型：`Because`
- 外部类型：`Bitmapset`
- 外部类型：`BoolExpr`
- 外部类型：`BooleanEqualOperator`
- 外部类型：`BooleanNotEqualOperator`
- 外部类型：`BooleanTest`
- 外部类型：`Build`
- 外部类型：`But`
- 外部类型：`Can`
- 外部类型：`CaseExpr`
- 外部类型：`CaseTestExpr`
- 外部类型：`CaseTestExprs`
- 外部类型：`CaseWhen`
- 外部类型：`Check`
- 外部类型：`CoalesceExpr`
- 外部类型：`Code`
- 外部类型：`CoerceToDomain`
- 外部类型：`CoerceViaIO`
- 外部类型：`CollateExpr`
- 外部类型：`CompactAttribute`
- 外部类型：`Const`
- 外部类型：`Consts`
- 外部类型：`ConvertRowtypeExpr`
- 外部类型：`Copy`
- 外部类型：`CurrentMemoryContext`
- 外部类型：`CurrentOfExpr`
- 外部类型：`Datum`
- 外部类型：`Deconstruct`
- 外部类型：`Delete`
- 外部类型：`DistinctExpr`
- 外部类型：`Do`
- 外部类型：`EState`
- 外部类型：`Each`
- 外部类型：`Else`
- 外部类型：`ErrorContextCallback`
- 外部类型：`Exception`
- 外部类型：`ExecPrepareExpr`
- 外部类型：`Expr`
- 外部类型：`ExprState`
- 外部类型：`Fetch`
- 外部类型：`Fetching`
- 外部类型：`FieldSelect`
- 外部类型：`First`
- 外部类型：`For`
- 外部类型：`Forget`
- 外部类型：`Form_pg_attribute`
- 外部类型：`Form_pg_proc`
- 外部类型：`Found`
- 外部类型：`FuncExpr`
- 外部类型：`Furthermore`
- 外部类型：`Generate`
- 外部类型：`Generic`
- 外部类型：`Get`
- 外部类型：`Give`
- 外部类型：`Given`
- 外部类型：`HeapTuple`
- 外部类型：`Hence`
- 外部类型：`Here`
- 外部类型：`However`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Input`
- 外部类型：`InvalidAttrNumber`
- 外部类型：`InvalidOid`
- 外部类型：`It`
- 外部类型：`JsonConstructorExpr`
- 外部类型：`JsonExpr`
- 外部类型：`JsonFormat`
- 外部类型：`JsonValueExpr`
- 外部类型：`Leave`
- 外部类型：`Like`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`Look`
- 外部类型：`Make`
- 外部类型：`MemoryContext`
- 外部类型：`MinMaxExpr`
- 外部类型：`Must`
- 外部类型：`NULLs`
- 外部类型：`NamedArgExpr`
- 外部类型：`Need`
- 外部类型：`New`
- 外部类型：`NextValueExpr`
- 外部类型：`No`
- 外部类型：`NoLock`
- 外部类型：`Node`
- 外部类型：`Not`
- 外部类型：`NotNullSource`
- 外部类型：`Note`
- 外部类型：`Now`
- 外部类型：`NullIfExpr`
- 外部类型：`NullTest`
- 外部类型：`ORs`
- 外部类型：`Of`
- 外部类型：`Oid`
- 外部类型：`OidIsValid`
- 外部类型：`Okay`
- 外部类型：`Once`
- 外部类型：`One`
- 外部类型：`OpExpr`
- 外部类型：`Ordinarily`
- 外部类型：`Others`
- 外部类型：`Otherwise`
- 外部类型：`Param`
- 外部类型：`ParamExternData`
- 外部类型：`ParamListInfo`
- 外部类型：`ParseComplexProjection`
- 外部类型：`ParseState`
- 外部类型：`PlaceHolderVar`
- 外部类型：`PlannerInfo`
- 外部类型：`Prepare`
- 外部类型：`ProcedureRelationId`
- 外部类型：`Process`
- 外部类型：`QualCost`
- 外部类型：`Query`
- 外部类型：`RangeTblEntry`
- 外部类型：`Recheck`
- 外部类型：`Record`
- 外部类型：`Recurse`
- 外部类型：`Recursively`
- 外部类型：`Reduce`
- 外部类型：`RelOptInfo`
- 外部类型：`RelabelType`
- 外部类型：`RelabelTypes`
- 外部类型：`Relation`
- 外部类型：`Release`
- 外部类型：`Replace`
- 外部类型：`Return`
- 外部类型：`RowExpr`
- 外部类型：`SQLFunctionParseInfoPtr`
- 外部类型：`SQLValueFunction`
- 外部类型：`SQLlanguageId`
- 外部类型：`Scalar`
- 外部类型：`ScalarArrayOpExpr`
- 外部类型：`See`
- 外部类型：`Set`
- 外部类型：`Setup`
- 外部类型：`Simplify`
- 外部类型：`Since`
- 外部类型：`So`
- 外部类型：`Some`
- 外部类型：`SubPlan`
- 外部类型：`SubPlans`
- 外部类型：`SubscriptingRef`
- 外部类型：`SupportRequestSimplify`
- 外部类型：`SupportRequestSimplifyAggref`
- 外部类型：`T_Aggref`
- 外部类型：`T_AlternativeSubPlan`
- 外部类型：`T_ArrayCoerceExpr`
- 外部类型：`T_ArrayExpr`
- 外部类型：`T_BoolExpr`
- 外部类型：`T_BooleanTest`
- 外部类型：`T_CaseExpr`
- 外部类型：`T_CaseTestExpr`
- 外部类型：`T_CoalesceExpr`
- 外部类型：`T_CoerceToDomain`
- 外部类型：`T_CoerceViaIO`
- 外部类型：`T_CollateExpr`
- 外部类型：`T_Const`
- 外部类型：`T_ConvertRowtypeExpr`
- 外部类型：`T_DistinctExpr`
- 外部类型：`T_FieldSelect`
- 外部类型：`T_FuncExpr`
- 外部类型：`T_JsonConstructorExpr`
- 外部类型：`T_JsonValueExpr`
- 外部类型：`T_MinMaxExpr`
- 外部类型：`T_NullIfExpr`
- 外部类型：`T_NullTest`
- 外部类型：`T_OpExpr`
- 外部类型：`T_Param`
- 外部类型：`T_PlaceHolderVar`
- 外部类型：`T_RelabelType`
- 外部类型：`T_RowExpr`
- 外部类型：`T_SQLValueFunction`
- 外部类型：`T_ScalarArrayOpExpr`
- 外部类型：`T_SubPlan`
- 外部类型：`T_SubscriptingRef`
- 外部类型：`T_SupportRequestSimplify`
- 外部类型：`T_SupportRequestSimplifyAggref`
- 外部类型：`T_Var`
- 外部类型：`T_WindowFunc`
- 外部类型：`T_XmlExpr`
- 外部类型：`TargetEntry`
- 外部类型：`Tell`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`Therefore`
- 外部类型：`These`
- 外部类型：`This`
- 外部类型：`TimeZone`
- 外部类型：`To`
- 外部类型：`Treating`
- 外部类型：`TupleDesc`
- 外部类型：`Use`
- 外部类型：`Var`
- 外部类型：`Vars`
- 外部类型：`We`
- 外部类型：`Whew`
- 外部类型：`WindowFunc`
- 外部类型：`XmlExpr`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-6aa1771c33` → 本草稿移入 `cases/defect/auto-postgres-6aa1771c33/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
