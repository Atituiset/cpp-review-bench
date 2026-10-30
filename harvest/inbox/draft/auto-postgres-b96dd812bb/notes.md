# auto-postgres-b96dd812bb

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
| 外部依赖数（dep_count） | 20 |
| 编译错误数（gcc syntax-only） | 11（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a054b77e40d0579a9ee1218e2447d5af045d1083 (https://github.com/postgres/postgres/commit/a054b77e40d0579a9ee1218e2447d5af045d1083)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 233；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a054b77e40d0579a9ee1218e2447d5af045d1083 pg_plan_advice: Suppress advice from unflattened set operations. :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -33,17 +33,16 @@ static RTEKind unique_nonjoin_rtekind(Bitmapset *relids, List *rtable);
  * If there is at least one ElidedNode for this plan node, pass the uppermost
  * one as elided_node, else pass NULL.
  *
- * Set the 'beneath_any_gather' node if we are underneath a Gather or
- * Gather Merge node (except for a single-copy Gather node, for which
- * GATHER or GATHER_MERGE advice should not be emitted).
+ * Set the 'needs_no_gather' flag if NO_GATHER advice should be generated for
+ * this scan.
  *
  * Set the 'within_join_problem' flag if we're inside of a join problem and
  * not otherwise.
  */
 pgpa_scan *
 pgpa_build_scan(pgpa_plan_walker_context *walker, Plan *plan,
 				ElidedNode *elided_node,
-				bool beneath_any_gather, bool within_join_problem)
+				bool needs_no_gather, bool within_join_problem)
 {
 	pgpa_scan_strategy strategy = PGPA_SCAN_ORDINARY;
 	Bitmapset  *relids = NULL;
@@ -224,17 +223,12 @@ pgpa_build_scan(pgpa_plan_walker_context *walker, Plan *plan,
 		return NULL;
 	}
 
-	/*
-	 * Add the appropriate set of RTIs to walker->no_gather_scans.
-	 *
-	 * Add nothing if we're beneath a Gather or Gather Merge node, since
-	 * NO_GATHER advice is clearly inappropriate in that situation.
-	 */
-	if (!beneath_any_gather)
+	/* Caller tells us whether NO_GATHER() advice for this scan is needed. */
+	if (needs_no_gather)
 		walker->no_gather_scans =
 			bms_add_members(walker->no_gather_scans, relids);
 
-	/* Caller tells us whether NO_GATHER() advice for this scan is needed. */
+	/* Now create and return the final pgpa_scan object. */
 	return pgpa_make_scan(walker, plan, strategy, relids);
 }
 
@@ -260,6 +254,80 @@ pgpa_make_scan(pgpa_plan_walker_context *walker, Plan *plan,
 	return scan;
 }
 
+/*
+ * Does this Plan node or associated ElidedNode look like the topmost node
+ * generated while planning an unflattened set operation?
+ *
+ * The return value of this function is a little bit inexact, because for
+ * example if a set operation produces a HashAggregate on top of an Append,
+ * this function will only identify the latter. Since the HashAggregate
+ * node is not RTI-bearing, the difference doesn't currently matter.
+ *
+ * Pass the plan node as 'plan', and if we're considering an elided node
+ * associated with that plan node, also pass it as 'elided_node'.
+ */
+bool
+pgpa_is_setop_parent(Plan *plan, ElidedNode *elided_node, List *rtable)
+{
+	Bitmapset  *relids;
+	int			rti = -1;
+
+	/*
+	 * If we're considering a SetOp or RecursiveUnion, then this is definitely
+	 * a setop parent. If we're considering an Append, a MergeAppend, or an
+	 * upper Result node, this might be a setop parent. If it's anything else,
+	 * this is not a setop parent.
+	 */
+	if (elided_node != NULL)
+	{
+		if (elided_node->elided_type == T_Append ||
+			elided_node->elided_type == T_MergeAppend)
+			relids = elided_node->relids;
+		else
+			return false;
+	}
+	else
+	{
+		if (IsA(plan, SetOp) || IsA(plan, RecursiveUnion))
+			return true;
+		else if (IsA(plan, Append) || IsA(plan, MergeAppend) ||
+				 (IsA(plan, Result) &&
+				  (((Result *) plan)->result_type == RESULT_TYPE_UPPER)))
+			relids = pgpa_relids(plan);
+		else
+			return false;
+	}
+
+	/*
+	 * As of this writing, we reach here with relids == NULL only for a
+	 * partitionwise aggregate, which is not a setop parent.
+	 */
+	if (relids == NULL)
+		return false;
+
+	/*
+	 * Conceptually, this Append or MergeAppend, or the Result node that
+	 * replaced it, could be a partitionwise scan, a partitionwise join, a
+	 * flattened UNION ALL, or an unflattened set operation. We want to return
+	 * true only in that last case; for our purposes, if a UNION ALL is
+	 * flattened, it no longer counts as a set operation.
+	 *
+	 * An unflattened set operation has no RTI of its own, so a plan node
+	 * implementing one carries the RTIs of its children, which must always be
+	 * subqueries with rte->inh == false. A flattened set operation does have
+	 * an RTI, and that 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_add_members`
- 外部函数：`lappend`
- 外部函数：`palloc_object`
- 外部函数：`pgpa_build_scan`
- 大写宏：`GATHER`
- 大写宏：`GATHER_MERGE`
- 大写宏：`NO_GATHER`
- 大写宏：`NULL`
- 大写宏：`PGPA_SCAN_ORDINARY`
- 外部类型：`Add`
- 外部类型：`Bitmapset`
- 外部类型：`Caller`
- 外部类型：`Create`
- 外部类型：`Determine`
- 外部类型：`ElidedNode`
- 外部类型：`Gather`
- 外部类型：`If`
- 外部类型：`Merge`
- 外部类型：`Plan`
- 外部类型：`RTIs`
- 外部类型：`Set`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-b96dd812bb` → 本草稿移入 `cases/defect/auto-postgres-b96dd812bb/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
