# auto-postgres-89cdcb285e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#a054b77e40d0579a9ee1218e2447d5af045d1083](https://github.com/postgres/postgres/commit/a054b77e40d0579a9ee1218e2447d5af045d1083) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-30 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 99 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a054b77e40d0579a9ee1218e2447d5af045d1083 (https://github.com/postgres/postgres/commit/a054b77e40d0579a9ee1218e2447d5af045d1083)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 290；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a054b77e40d0579a9ee1218e2447d5af045d1083 pg_plan_advice: Suppress advice from unflattened set operations. :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -23,6 +23,7 @@
 #include "utils/lsyscache.h"
 
 static void pgpa_walk_recursively(pgpa_plan_walker_context *walker, Plan *plan,
+								  bool within_setop,
 								  bool within_join_problem,
 								  pgpa_join_unroller *join_unroller,
 								  List *active_query_features,
@@ -90,15 +91,17 @@ pgpa_plan_walker(pgpa_plan_walker_context *walker, PlannedStmt *pstmt,
 	walker->pstmt = pstmt;
 
 	/* Walk the main plan tree. */
-	pgpa_walk_recursively(walker, pstmt->planTree, false, NULL, NIL, false);
+	pgpa_walk_recursively(walker, pstmt->planTree, false, false, NULL, NIL,
+						  false);
 
 	/* Main plan tree walk won't reach subplans, so walk those. */
 	foreach(lc, pstmt->subplans)
 	{
 		Plan	   *plan = lfirst(lc);
 
 		if (plan != NULL)
-			pgpa_walk_recursively(walker, plan, false, NULL, NIL, false);
+			pgpa_walk_recursively(walker, plan, false, false, NULL, NIL,
+								  false);
 	}
 
 	/* Adjust RTIs from sj_unique_rels for the flattened range table. */
@@ -165,6 +168,11 @@ pgpa_plan_walker(pgpa_plan_walker_context *walker, PlannedStmt *pstmt,
 	 * that we won't advise either GATHER or GATHER_MERGE or NO_GATHER in such
 	 * cases, which might be something we want to improve in the future.
 	 *
+	 * The same thing happens when a Gather or Gather Merge node sits atop a
+	 * set operation that was not flattened, because we decline to add RTIs
+	 * from unflattened set operations to query features. In that case,
+	 * there's no planner hook that would allow us to enforce the advice.
+	 *
 	 * (Should the Partial Aggregates in such a case be created in an
 	 * UPPERREL_GROUP_AGG with a non-empty relid set? Right now that doesn't
 	 * happen, but it seems like it would make life easier for us if it did.)
@@ -224,6 +232,9 @@ pgpa_plan_walker(pgpa_plan_walker_context *walker, PlannedStmt *pstmt,
 /*
  * Main workhorse for the plan tree walk.
  *
+ * If within_setop is true, the portion of the plan tree in which this node
+ * occurs was generated while planning an unflattened set operation.
+ *
  * If within_join_problem is true, we encountered a join at some higher level
  * of the tree walk and haven't yet descended out of the portion of the plan
  * tree that is part of that same join problem. We're no longer in the same
@@ -247,6 +258,7 @@ pgpa_plan_walker(pgpa_plan_walker_context *walker, PlannedStmt *pstmt,
  */
 static void
 pgpa_walk_recursively(pgpa_plan_walker_context *walker, Plan *plan,
+					  bool within_setop,
 					  bool within_join_problem,
 					  pgpa_join_unroller *join_unroller,
 					  List *active_query_features,
@@ -257,7 +269,6 @@ pgpa_walk_recursively(pgpa_plan_walker_context *walker, Plan *plan,
 	bool		join_unroller_toplevel = false;
 	ListCell   *lc;
 	List	   *extraplans = NIL;
-	List	   *elided_nodes = NIL;
 	int			first_partial_plan = 0;
 
 	Assert(within_join_problem || join_unroller == NULL);
@@ -283,61 +294,59 @@ pgpa_walk_recursively(pgpa_plan_walker_context *walker, Plan *plan,
 	}
 
 	/*
-	 * Find all elided nodes for this Plan node.
+	 * Process elided nodes that pertain to this Plan node. In the pstmt's
+	 * list, the logically uppermost elided node appears last, so we walk the
+	 * list in reverse order.
 	 */
-	foreach_node(ElidedNode, n, walker->pstmt->elidedNodes)
+	for (int n = list_length(walker->pstmt->elidedNodes) - 1; n >= 0; --n)
 	{
-		if (n->plan_node_id == plan->plan_node_id)
-			elided_nodes = lappend(elided_nodes, n);
-	}
+		ElidedNode *elided_node = list_nth(walker->pstmt->elidedNodes, n);
 
-	/* If we found any elided_nodes, handle them. */
-	if (elided_nodes != NIL)
-	{
-		int			num_elided_nodes = list_length(elided_nodes);
-		ElidedNode *last_elided_node;
+		/* Ignore elided nodes that do not pertain to this plan node. */
+		if (elided_node->plan_node_id != plan->plan_node_id)
+			continue;
+
+		/* Did an unflattened set operation produce this ElidedNode? */
+		if (!within_setop)
+			within_setop = pgpa_is_setop_parent(plan, elided_node,
+												walker->ps
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`and`
- 外部函数：`lappend`
- 外部函数：`lfirst`
- 外部函数：`list_copy`
- 外部函数：`list_delete_ptr`
- 外部函数：`list_length`
- 外部函数：`list_nth`
- 外部函数：`list_truncate`
- 外部函数：`pgpa_add_feature`
- 外部函数：`pgpa_build_scan`
- 外部函数：`pgpa_filter_out_join_relids`
- 外部函数：`pgpa_qf_add_rtis`
- 外部函数：`that`
- 大写宏：`GATHER`
- 大写宏：`GATHER_MERGE`
- 大写宏：`NIL`
- 大写宏：`NO_GATHER`
- 大写宏：`NULL`
- 大写宏：`PGPAQF_GATHER`
- 大写宏：`PGPAQF_GATHER_MERGE`
- 大写宏：`UPPERREL_GROUP_AGG`
- 外部类型：`Adjust`
- 外部类型：`Aggregates`
- 外部类型：`Append`
- 外部类型：`Build`
- 外部类型：`Check`
- 外部类型：`ElidedNode`
- 外部类型：`Exception`
- 外部类型：`Find`
- 外部类型：`Gather`
- 外部类型：`GatherMerge`
- 外部类型：`However`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`Main`
- 外部类型：`Merge`
- 外部类型：`MergeAppend`
- 外部类型：`Partial`
- 外部类型：`Plan`
- 外部类型：`RTIs`
- 外部类型：`Right`
- 外部类型：`Should`
- 外部类型：`SubqueryScan`
- 外部类型：`These`
- 外部类型：`Walk`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-89cdcb285e` → 本草稿移入 `cases/defect/auto-postgres-89cdcb285e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
