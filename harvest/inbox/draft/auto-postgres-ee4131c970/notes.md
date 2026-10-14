# auto-postgres-ee4131c970

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#0d6103e537e1299046e5906149f9a7d2108be2cf](https://github.com/postgres/postgres/commit/0d6103e537e1299046e5906149f9a7d2108be2cf) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 151 |
| 编译错误数（gcc syntax-only） | 38（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #0d6103e537e1299046e5906149f9a7d2108be2cf (https://github.com/postgres/postgres/commit/0d6103e537e1299046e5906149f9a7d2108be2cf)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1472；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 0d6103e537e1299046e5906149f9a7d2108be2cf Give antijoins made from sublinks a relid :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -93,6 +93,7 @@ static bool contain_outer_selfref_walker(Node *node, Index *depth);
 static void inline_cte(PlannerInfo *root, CommonTableExpr *cte);
 static bool inline_cte_walker(Node *node, inline_cte_walker_context *context);
 static bool sublink_testexpr_is_not_nullable(PlannerInfo *root, SubLink *sublink);
+static int	add_antijoin_rte(Query *parse);
 static bool simplify_EXISTS_query(PlannerInfo *root, Query *query);
 static Query *convert_EXISTS_to_ANY(PlannerInfo *root, Query *subselect,
 									Node **testexpr, List **paramIds);
@@ -1469,7 +1470,8 @@ convert_ANY_sublink_to_join(PlannerInfo *root, SubLink *sublink,
 	result->join_using_alias = NULL;
 	result->quals = quals;
 	result->alias = NULL;
-	result->rtindex = 0;		/* we don't need an RTE for it */
+	/* a semijoin doesn't need an RTE, but see add_antijoin_rte() */
+	result->rtindex = under_not ? add_antijoin_rte(parse) : 0;
 
 	return result;
 }
@@ -1781,11 +1783,56 @@ convert_EXISTS_sublink_to_join(PlannerInfo *root, SubLink *sublink,
 	result->join_using_alias = NULL;
 	result->quals = whereClause;
 	result->alias = NULL;
-	result->rtindex = 0;		/* we don't need an RTE for it */
+	/* a semijoin doesn't need an RTE, but see add_antijoin_rte() */
+	result->rtindex = under_not ? add_antijoin_rte(parse) : 0;
 
 	return result;
 }
 
+/*
+ * add_antijoin_rte: make a join RTE for an antijoin built from a sublink
+ *
+ * Giving the antijoin a relid, like an antijoin made by reducing a left join
+ * has, is what tells the rest of the planner that anything coming from above
+ * the join must not be evaluated within its RHS.  Normally nothing above can
+ * reference the RHS at all, but a PlaceHolderVar whose phrels cover the whole
+ * join can make a qual appear to; the join's relid in phrels then keeps such
+ * a qual from being pushed down into the RHS, where it would have the
+ * opposite of the intended effect.  (For a semijoin that would be harmless,
+ * so those are left without a relid.)
+ *
+ * Returns the new RTE's index.
+ */
+static int
+add_antijoin_rte(Query *parse)
+{
+	RangeTblEntry *rte;
+
+	/* Manufacture a join RTE to use. */
+	rte = makeNode(RangeTblEntry);
+	rte->rtekind = RTE_JOIN;
+	rte->jointype = JOIN_ANTI;
+	rte->joinmergedcols = 0;
+	rte->joinaliasvars = NIL;	/* nothing can reference the join's output */
+	rte->joinleftcols = NIL;	/* so it has no columns */
+	rte->joinrightcols = NIL;	/* ditto */
+	rte->join_using_alias = NULL;
+
+	rte->alias = NULL;
+	rte->eref = makeAlias("*ANTI*", NIL);
+	rte->lateral = false;
+	rte->inh = false;
+	rte->inFromCl = false;		/* not something the user wrote in FROM */
+
+	/*
+	 * Add completed RTE to the query's range table list, so that we know its
+	 * index.
+	 */
+	parse->rtable = lappend(parse->rtable, rte);
+
+	return list_length(parse->rtable);
+}
+
 /*
  * simplify_EXISTS_query: remove any useless stuff in an EXISTS's subquery
  *
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`castNode`
- 外部函数：`convert_EXISTS_to_ANY`
- 外部函数：`copyObject`
- 外部函数：`elog`
- 外部函数：`exprType`
- 外部函数：`expression_tree_walker`
- 外部函数：`func_strict`
- 外部函数：`is_andclause`
- 外部函数：`is_orclause`
- 外部函数：`lappend`
- 外部函数：`linitial`
- 外部函数：`list_concat`
- 外部函数：`list_length`
- 外部函数：`op`
- 外部函数：`op_hashjoinable`
- 外部函数：`op_is_safe_index_member`
- 外部函数：`query_tree_walker`
- 外部函数：`references`
- 外部函数：`simplify_EXISTS_query`
- 外部函数：`strcmp`
- 大写宏：`ANY`
- 大写宏：`ANY_SUBLINK`
- 大写宏：`ARRAY_EQ_OP`
- 大写宏：`CTE`
- 大写宏：`ERROR`
- 大写宏：`FOR`
- 大写宏：`GETSTRUCT`
- 大写宏：`MULTIRANGE_EQ_OP`
- 大写宏：`NIL`
- 大写宏：`NULL`
- 大写宏：`OPEROID`
- 大写宏：`QTW_EXAMINE_RTES_AFTER`
- 大写宏：`RANGE_EQ_OP`
- 大写宏：`RECORD_EQ_OP`
- 大写宏：`RTE`
- 大写宏：`RTE_CTE`
- 大写宏：`RTE_SUBQUERY`
- 大写宏：`UPDATE`
- 外部类型：`BoolExpr`
- 外部类型：`CTEs`
- 外部类型：`CommonTableExpr`
- 外部类型：`Convert`
- 外部类型：`Form_pg_operator`
- 外部类型：`Found`
- 外部类型：`Generate`
- 外部类型：`Hash`
- 外部类型：`HeapTuple`
- 外部类型：`Historically`
- 外部类型：`List`
- 外部类型：`Node`
- 外部类型：`Oid`
- 外部类型：`OpExpr`
- 外部类型：`PlannerInfo`
- 外部类型：`Punt`
- 外部类型：`Query`
- 外部类型：`RangeTblEntry`
- 外部类型：`RowCompareExpr`
- 外部类型：`Since`
- 外部类型：`Start`
- 外部类型：`SubLink`
- 外部类型：`Visit`
- 外部类型：`We`
- 外部类型：`Zero`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-ee4131c970` → 本草稿移入 `cases/defect/auto-postgres-ee4131c970/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
