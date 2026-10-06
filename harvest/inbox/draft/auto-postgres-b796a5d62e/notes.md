# auto-postgres-b796a5d62e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#d4c15309e7ed6e30e360b5911e1f4d3ca754aa2b](https://github.com/postgres/postgres/commit/d4c15309e7ed6e30e360b5911e1f4d3ca754aa2b) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-06 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 270 |
| 编译错误数（gcc syntax-only） | 90（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d4c15309e7ed6e30e360b5911e1f4d3ca754aa2b (https://github.com/postgres/postgres/commit/d4c15309e7ed6e30e360b5911e1f4d3ca754aa2b)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1563；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d4c15309e7ed6e30e360b5911e1f4d3ca754aa2b Flatten join alias Vars before rechecking subquery pullup safety :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -145,6 +145,8 @@ static bool is_safe_append_member(Query *subquery);
 static bool jointree_contains_lateral_outer_refs(PlannerInfo *root,
 												 Node *jtnode, bool restricted,
 												 Relids safe_upper_varnos);
+static void flatten_join_alias_vars_in_jointree(PlannerInfo *root,
+												Node *jtnode);
 static void perform_pullup_replace_vars(PlannerInfo *root,
 										pullup_replace_vars_context *rvcontext,
 										AppendRelInfo *containing_appendrel);
@@ -1525,6 +1527,26 @@ pull_up_simple_subquery(PlannerInfo *root, Node *jtnode, RangeTblEntry *rte,
 	 */
 	pull_up_subqueries(subroot);
 
+	/*
+	 * We must flatten any join alias Vars in the subquery's targetlist,
+	 * because pulling up the subquery's subqueries might have changed their
+	 * expansions into arbitrary expressions.  That could affect
+	 * pullup_replace_vars' decisions about whether PlaceHolderVar wrappers
+	 * are needed for tlist entries, and if the subquery is LATERAL, the
+	 * expansions might contain lateral references that the is_simple_subquery
+	 * recheck below has to see.  The latter also requires flattening join
+	 * alias Vars in the jointree quals of a LATERAL subquery.  (Likely it'd
+	 * be better to do flatten_join_alias_vars on the whole query tree at some
+	 * earlier stage, maybe even in the rewriter; but for now let's just fix
+	 * these cases here.)
+	 */
+	subquery->targetList = (List *)
+		flatten_join_alias_vars(subroot, subroot->parse,
+								(Node *) subquery->targetList);
+	if (rte->lateral)
+		flatten_join_alias_vars_in_jointree(subroot,
+											(Node *) subquery->jointree);
+
 	/*
 	 * Now we must recheck whether the subquery is still simple enough to pull
 	 * up.  If not, abandon processing it.
@@ -1551,19 +1573,6 @@ pull_up_simple_subquery(PlannerInfo *root, Node *jtnode, RangeTblEntry *rte,
 		return jtnode;
 	}
 
-	/*
-	 * We must flatten any join alias Vars in the subquery's targetlist,
-	 * because pulling up the subquery's subqueries might have changed their
-	 * expansions into arbitrary expressions, which could affect
-	 * pullup_replace_vars' decisions about whether PlaceHolderVar wrappers
-	 * are needed for tlist entries.  (Likely it'd be better to do
-	 * flatten_join_alias_vars on the whole query tree at some earlier stage,
-	 * maybe even in the rewriter; but for now let's just fix this case here.)
-	 */
-	subquery->targetList = (List *)
-		flatten_join_alias_vars(subroot, subroot->parse,
-								(Node *) subquery->targetList);
-
 	/*
 	 * Adjust level-0 varnos in subquery so that we can append its rangetable
 	 * to upper query's.  We have to fix the subquery's append_rel_list as
@@ -2548,6 +2557,40 @@ jointree_contains_lateral_outer_refs(PlannerInfo *root, Node *jtnode,
 	return false;
 }
 
+/*
+ * flatten_join_alias_vars_in_jointree
+ *		Apply flatten_join_alias_vars to all quals in the given jointree,
+ *		replacing them in place.  The jointree must belong to root->parse.
+ */
+static void
+flatten_join_alias_vars_in_jointree(PlannerInfo *root, Node *jtnode)
+{
+	if (jtnode == NULL)
+		return;
+	if (IsA(jtnode, RangeTblRef))
+		return;
+	else if (IsA(jtnode, FromExpr))
+	{
+		FromExpr   *f = (FromExpr *) jtnode;
+		ListCell   *l;
+
+		foreach(l, f->fromlist)
+			flatten_join_alias_vars_in_jointree(root, lfirst(l));
+		f->quals = flatten_join_alias_vars(root, root->parse, f->quals);
+	}
+	else if (IsA(jtnode, JoinExpr))
+	{
+		JoinExpr   *j = (JoinExpr *) jtnode;
+
+		flatten_join_alias_vars_in_jointree(root, j->larg);
+		flatten_join_alias_vars_in_jointree(root, j->rarg);
+		j->quals = flatten_join_alias_vars(root, root->parse, j->quals);
+	}
+	else
+		elog(ERROR, "unrecognized node type: %d",
+			 (int) nodeTag(jtnode));
+}
+
 /*
  * Perform pullup_replace_vars everyplace it's needed in the query tree.
  *
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`add_nulling_relids`
- 外部函数：`appendrel`
- 外部函数：`bms_add_member`
- 外部函数：`bms_add_members`
- 外部函数：`bms_copy`
- 外部函数：`bms_del_member`
- 外部函数：`bms_del_members`
- 外部函数：`bms_intersect`
- 外部函数：`bms_is_empty`
- 外部函数：`bms_is_member`
- 外部函数：`bms_is_subset`
- 外部函数：`bms_join`
- 外部函数：`bms_make_singleton`
- 外部函数：`bms_next_member`
- 外部函数：`bms_overlap`
- 外部函数：`bms_singleton_member`
- 外部函数：`bms_union`
- 外部函数：`build_generation_expression`
- 外部函数：`c`
- 外部函数：`castNode`
- 外部函数：`check_stack_depth`
- 外部函数：`contain_nonstrict_functions`
- 外部函数：`contain_vars_of_level`
- 外部函数：`contain_volatile_functions`
- 外部函数：`copyObject`
- 外部函数：`elog`
- 外部函数：`equal`
- 外部函数：`eval_const_expressions`
- 外部函数：`expression_returns_set`
- 外部函数：`expression_tree_walker`
- 外部函数：`flatten_join_alias_vars`
- 外部函数：`get_expr_result_type`
- 外部函数：`get_relation_notnullatts`
- 外部函数：`info`
- 外部函数：`inline_function_in_from`
- 外部函数：`lappend`
- 外部函数：`length`
- 外部函数：`lfirst`
- 外部函数：`linitial`
- 外部函数：`linitial_node`
- 外部函数：`list_concat`
- 外部函数：`list_length`
- 外部函数：`list_make1`
- 外部函数：`makeAlias`
- 外部函数：`makeFromExpr`
- 外部函数：`makeNode`
- 外部函数：`makeTargetEntry`
- 外部函数：`makeVar`
- 外部函数：`makeVarFromTargetEntry`
- 外部函数：`make_placeholder_expr`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`nodeTag`
- 外部函数：`palloc0_array`
- 外部函数：`palloc_object`
- 外部函数：`possible`
- 外部函数：`pull_up_sublinks_qual_recurse`
- 外部函数：`pull_varnos`
- 外部函数：`pull_varnos_of_level`
- 外部函数：`pullup`
- 外部函数：`query_or_expression_tree_walker`
- 外部函数：`query_tree_walker`
- 外部函数：`replace_rte_variables`
- 外部函数：`rt_fetch`
- 外部函数：`setops`
- 外部函数：`subnodes`
- 外部函数：`subquery_planner`
- 外部函数：`table`
- 外部函数：`table_close`
- 外部函数：`table_open`
- 外部函数：`tlist_same_datatypes`
- 外部函数：`varnullingrels`
- 大写宏：`ALL`
- 大写宏：`ATTRIBUTE_GENERATED_VIRTUAL`
- 大写宏：`CHECK_FOR_INTERRUPTS`
- 大写宏：`CMD_SELECT`
- 大写宏：`CONFLICT`
- 大写宏：`ERROR`
- 大写宏：`EXCLUDED`
- 大写宏：`FOR`
- 大写宏：`FROM`
- 大写宏：`FUNCTION`
- 大写宏：`JOIN`
- 大写宏：`JOIN_ANTI`
- 大写宏：`JOIN_FULL`
- 大写宏：`JOIN_INNER`
- 大写宏：`JOIN_LEFT`
- 大写宏：`JOIN_RIGHT`
- 大写宏：`JOIN_RIGHT_ANTI`
- 大写宏：`JOIN_RIGHT_SEMI`
- 大写宏：`JOIN_SEMI`
- 大写宏：`LATERAL`
- 大写宏：`LIMIT`
- 大写宏：`NIL`
- 大写宏：`NOTE`
- 大写宏：`NULL`
- 大写宏：`OFFSET`
- 大写宏：`OID`
- 大写宏：`ORDER`
- 大写宏：`ORDINALITY`
- 大写宏：`PHV`
- 大写宏：`RECORD`
- 大写宏：`REPLACEVARS_REPORT_ERROR`
- 大写宏：`REPLACE_WRAP_ALL`
- 大写宏：`REPLACE_WRAP_NONE`
- 大写宏：`REPLACE_WRAP_VARFREE`
- 大写宏：`RESULT`
- 大写宏：`RLS`
- 大写宏：`ROW`
- 大写宏：`RTE`
- 大写宏：`RTE_CTE`
- 大写宏：`RTE_FUNCTION`
- 大写宏：`RTE_GROUP`
- 大写宏：`RTE_JOIN`
- 大写宏：`RTE_NAMEDTUPLESTORE`
- 大写宏：`RTE_RELATION`
- 大写宏：`RTE_RESULT`
- 大写宏：`RTE_SUBQUERY`
- 大写宏：`RTE_TABLEFUNC`
- 大写宏：`RTE_VALUES`
- 大写宏：`SETOP_UNION`
- 大写宏：`SHARE`
- 大写宏：`TYPEFUNC_SCALAR`
- 大写宏：`UNION`
- 大写宏：`UPDATE`
- 大写宏：`VALUES`
- 大写宏：`WHERE`
- 大写宏：`WITH`
- 大写宏：`XXX`
- 外部类型：`Add`
- 外部类型：`Adjust`
- 外部类型：`Also`
- 外部类型：`Alternatively`
- 外部类型：`Although`
- 外部类型：`And`
- 外部类型：`Append`
- 外部类型：`AppendRelInfo`
- 外部类型：`AppendRelInfos`
- 外部类型：`Apply`
- 外部类型：`Assert`
- 外部类型：`AttrNumber`
- 外部类型：`Because`
- 外部类型：`Begin`
- 外部类型：`Build`
- 外部类型：`But`
- 外部类型：`CTEs`
- 外部类型：`Cache`
- 外部类型：`Calculate`
- 外部类型：`Caller`
- 外部类型：`Can`
- 外部类型：`Check`
- 外部类型：`Clear`
- 外部类型：`Const`
- 外部类型：`Convert`
- 外部类型：`Create`
- 外部类型：`CurrentMemoryContext`
- 外部类型：`Currently`
- 外部类型：`Do`
- 外部类型：`Don`
- 外部类型：`Else`
- 外部类型：`Expr`
- 外部类型：`Fail`
- 外部类型：`Finally`
- 外部类型：`First`
- 外部类型：`For`
- 外部类型：`Form_pg_attribute`
- 外部类型：`FromExpr`
- 外部类型：`Furthermore`
- 外部类型：`Generate`
- 外部类型：`Getting`
- 外部类型：`Give`
- 外部类型：`However`
- 外部类型：`IDs`
- 外部类型：`Identify`
- 外部类型：`If`
- 外部类型：`Implicit`
- 外部类型：`In`
- 外部类型：`Index`
- 外部类型：`Initialize`
- 外部类型：`Insert`
- 外部类型：`InvalidAttrNumber`
- 外部类型：`InvalidOid`
- 外部类型：`Is`
- 外部类型：`It`
- 外部类型：`JoinDomain`
- 外部类型：`JoinExpr`
- 外部类型：`JoinExprs`
- 外部类型：`Just`
- 外部类型：`Leaf`
- 外部类型：`Length`
- 外部类型：`Let`
- 外部类型：`Likely`
- 外部类型：`Likewise`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`Make`
- 外部类型：`Mark`
- 外部类型：`Maybe`
- 外部类型：`MergeAction`
- 外部类型：`MinMaxAggInfo`
- 外部类型：`Miscellaneous`
- 外部类型：`More`
- 外部类型：`Must`
- 外部类型：`Need`
- 外部类型：`No`
- 外部类型：`NoLock`
- 外部类型：`Node`
- 外部类型：`Note`
- 外部类型：`Nothing`
- 外部类型：`Notice`
- 外部类型：`Now`
- 外部类型：`OffsetVarNodes`
- 外部类型：`Oid`
- 外部类型：`Only`
- 外部类型：`Or`
- 外部类型：`Otherwise`
- 外部类型：`PHVs`
- 外部类型：`Per`
- 外部类型：`Perform`
- 外部类型：`Perhaps`
- 外部类型：`PlaceHolderInfo`
- 外部类型：`PlaceHolderVar`
- 外部类型：`PlaceHolderVars`
- 外部类型：`PlannerInfo`
- 外部类型：`Propagate`
- 外部类型：`Pull`
- 外部类型：`Query`
- 外部类型：`RTE_RESULTs`
- 外部类型：`RTEs`
- 外部类型：`RangeTblEntry`
- 外部类型：`RangeTblFunction`
- 外部类型：`RangeTblRef`
- 外部类型：`Recurse`
- 外部类型：`Recursion`
- 外部类型：`Recursively`
- 外部类型：`Relation`
- 外部类型：`Relids`
- 外部类型：`Replace`
- 外部类型：`Return`
- 外部类型：`RowExpr`
- 外部类型：`SRFs`
- 外部类型：`Scan`
- 外部类型：`See`
- 外部类型：`Set`
- 外部类型：`SetOperationStmt`
- 外部类型：`Shouldn`
- 外部类型：`Similarly`
- 外部类型：`Simple`
- 外部类型：`Since`
- 外部类型：`So`
- 外部类型：`SpecialJoinInfo`
- 外部类型：`SubLinks`
- 外部类型：`Successful`
- 外部类型：`System`
- 外部类型：`TableFunc`
- 外部类型：`TableSampleClause`
- 外部类型：`TargetEntry`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`Their`
- 外部类型：`Then`
- 外部类型：`There`
- 外部类型：`This`
- 外部类型：`Top`
- 外部类型：`TupleDesc`
- 外部类型：`TypeFuncClass`
- 外部类型：`Upper`
- 外部类型：`Use`
- 外部类型：`Using`
- 外部类型：`Var`
- 外部类型：`Various`
- 外部类型：`Vars`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-b796a5d62e` → 本草稿移入 `cases/defect/auto-postgres-b796a5d62e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
