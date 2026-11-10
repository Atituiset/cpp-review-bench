# auto-postgres-183d446a4e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#edbb1d2dcb18d0706e644d3048a21e4475debd95](https://github.com/postgres/postgres/commit/edbb1d2dcb18d0706e644d3048a21e4475debd95) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 36 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #edbb1d2dcb18d0706e644d3048a21e4475debd95 (https://github.com/postgres/postgres/commit/edbb1d2dcb18d0706e644d3048a21e4475debd95)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 36；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT edbb1d2dcb18d0706e644d3048a21e4475debd95 Add memory/disk usage for Function Scan nodes in EXPLAIN :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -29,21 +29,8 @@
 #include "utils/memutils.h"
 #include "utils/tuplestore.h"
 
-
-/*
- * Runtime data for each function being scanned.
- */
-typedef struct FunctionScanPerFuncState
-{
-	SetExprState *setexpr;		/* state of the expression being evaluated */
-	TupleDesc	tupdesc;		/* desc of the function result type */
-	int			colcount;		/* expected number of result columns */
-	Tuplestorestate *tstore;	/* holds the function result set */
-	int64		rowcount;		/* # of rows in result set, -1 if not known */
-	TupleTableSlot *func_slot;	/* function result slot (or NULL) */
-} FunctionScanPerFuncState;
-
 static TupleTableSlot *FunctionNext(FunctionScanState *node);
+static void update_storage_stats(FunctionScanState *node);
 
 
 /* ----------------------------------------------------------------
@@ -520,6 +507,8 @@ ExecInitFunctionScan(FunctionScan *node, EState *estate, int eflags)
 												  "Table function arguments",
 												  ALLOCSET_DEFAULT_SIZES);
 
+	scanstate->shared_info = NULL;
+
 	return scanstate;
 }
 
@@ -534,6 +523,26 @@ ExecEndFunctionScan(FunctionScanState *node)
 {
 	int			i;
 
+	/*
+	 * When ending a parallel worker, copy the tuplestore usage details into
+	 * shared memory so that they can be picked up by the main process to
+	 * report in EXPLAIN ANALYZE.  The Gather node above us may be rescanned,
+	 * in which case a new set of workers will write to the same slots, so
+	 * merge with any existing values rather than overwriting them.
+	 */
+	if (node->shared_info != NULL && IsParallelWorker())
+	{
+		FunctionScanInstrumentation *si;
+
+		/* fetch storage info from tuplestores and store in node->stats */
+		update_storage_stats(node);
+
+		Assert(ParallelWorkerNumber < node->shared_info->num_workers);
+		si = &node->shared_info->sinstrument[ParallelWorkerNumber];
+		si->maxSpaceUsed = Max(si->maxSpaceUsed, node->stats.maxSpaceUsed);
+		si->usedDisk |= node->stats.usedDisk;
+	}
+
 	/*
 	 * Release slots and tuplestore resources
 	 */
@@ -549,6 +558,41 @@ ExecEndFunctionScan(FunctionScanState *node)
 	}
 }
 
+/*
+ * update_storage_stats
+ *		Update the storage statistics for EXPLAIN ANALYZE with the storage
+ *		space used by each function's tuplestore.
+ *
+ * We expect this to be called before starting new scans so that the
+ * maxSpaceUsed and usedDisk fields are not forgotten between rescans.  We
+ * calculate the total space used by all tuplestores at the end of each scan
+ * and store that if it's higher than any previous scan's.  "usedDisk" is
+ * marked if any tuplestore spilled during any scan.
+ */
+static void
+update_storage_stats(FunctionScanState *node)
+{
+	int64		totalSpaceUsed = 0;
+
+	/* Calculate the sum of the space used by each function's tuplestore. */
+	for (int i = 0; i < node->nfuncs; i++)
+	{
+		FunctionScanPerFuncState *fs = &node->funcstates[i];
+		bool		usedDisk;
+		int64		spaceUsed;
+
+		if (fs->tstore == NULL)
+			continue;
+
+		tuplestore_get_stats(fs->tstore, &usedDisk, &spaceUsed);
+		totalSpaceUsed += spaceUsed;
+		node->stats.usedDisk |= usedDisk;
+	}
+
+	/* Record this one if it beats the previous best. */
+	node->stats.maxSpaceUsed = Max(node->stats.maxSpaceUsed, totalSpaceUsed);
+}
+
 /* ----------------------------------------------------------------
  *		ExecReScanFunctionScan
  *
@@ -586,6 +630,10 @@ ExecReScanFunctionScan(FunctionScanState *node)
 	{
 		ListCell   *lc;
 
+		/* Track tuplestore storage stats, if needed */
+		if (node->ss.ps.instrument)
+			update_storage_stats(node);
+
 		i = 0;
 		foreach(lc, scan->functions)
 		{
@@ -614,3 +662,90 @@ ExecReScanFunctionScan(FunctionScanState *node)
 			tuplestore_rescan(node->funcstates[i].tstore);
 	}
 }
+
+/* ----------------------------------------------------------------
+ *						Parallel Query Support
+ * ----------------------------------------------------------------
+ */
+
+/* ----------------------------------------------------------------
+ *		ExecFunctionScanEstimate
+ *
+ *		Estimate space required t
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`column`
- 外部函数：`func_slots`
- 外部函数：`scanslot`
- 外部函数：`slot`
- 外部函数：`slot_getallattrs`
- 外部函数：`tuplestore_gettupleslot`
- 外部函数：`tuplestore_rescan`
- 大写宏：`ALL`
- 大写宏：`EXEC_FLAG_BACKWARD`
- 大写宏：`NULL`
- 外部类型：`Datum`
- 外部类型：`EState`
- 外部类型：`Fast`
- 外部类型：`FunctionScanPerFuncState`
- 外部类型：`FunctionScanState`
- 外部类型：`Get`
- 外部类型：`If`
- 外部类型：`Increment`
- 外部类型：`Main`
- 外部类型：`No`
- 外部类型：`Otherwise`
- 外部类型：`PortalRunSelect`
- 外部类型：`Runtime`
- 外部类型：`ScanDirection`
- 外部类型：`See`
- 外部类型：`SetExprState`
- 外部类型：`Subsequent`
- 外部类型：`Table`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`TupleDesc`
- 外部类型：`TupleTableSlot`
- 外部类型：`Tuplestorestate`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-183d446a4e` → 本草稿移入 `cases/defect/auto-postgres-183d446a4e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
