# auto-postgres-2ba94b1ebf

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#1378aa13430e264a990a18597e9d8fdae740927e](https://github.com/postgres/postgres/commit/1378aa13430e264a990a18597e9d8fdae740927e) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 133 |
| 编译错误数（gcc syntax-only） | 35（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1378aa13430e264a990a18597e9d8fdae740927e (https://github.com/postgres/postgres/commit/1378aa13430e264a990a18597e9d8fdae740927e)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 1130；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1378aa13430e264a990a18597e9d8fdae740927e Report per-index vacuum progress in pg_stat_progress_vacuum :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1115,6 +1115,17 @@ parallel_vacuum_process_one_index(ParallelVacuumState *pvs, Relation indrel,
 	IndexBulkDeleteResult *istat = NULL;
 	IndexBulkDeleteResult *istat_res;
 	IndexVacuumInfo ivinfo;
+	const int	progress_index[] = {
+		PROGRESS_VACUUM_PHASE,
+		PROGRESS_VACUUM_CURRENT_INDEX_RELID
+	};
+	int64		progress_val[2];
+	const int	reset_index[] = {
+		PROGRESS_VACUUM_CURRENT_INDEX_RELID,
+		PROGRESS_SCAN_BLOCKS_TOTAL,
+		PROGRESS_SCAN_BLOCKS_DONE
+	};
+	const int64 reset_val[] = {(int64) InvalidOid, 0, 0};
 
 	/*
 	 * Update the pointer to the corresponding bulk-deletion result if someone
@@ -1127,7 +1138,7 @@ parallel_vacuum_process_one_index(ParallelVacuumState *pvs, Relation indrel,
 	ivinfo.heaprel = pvs->heaprel;
 	ivinfo.analyze_only = false;
 	ivinfo.is_autovacuum = pvs->shared->is_autovacuum;
-	ivinfo.report_progress = false;
+	ivinfo.report_progress = true;
 	ivinfo.message_level = DEBUG2;
 	ivinfo.estimated_count = pvs->shared->estimated_count;
 	ivinfo.num_heap_tuples = pvs->shared->reltuples;
@@ -1137,6 +1148,13 @@ parallel_vacuum_process_one_index(ParallelVacuumState *pvs, Relation indrel,
 	pvs->indname = pstrdup(RelationGetRelationName(indrel));
 	pvs->status = indstats->status;
 
+	/* Report the phase and the index we're about to process */
+	progress_val[0] = (indstats->status == PARALLEL_INDVAC_STATUS_NEED_BULKDELETE)
+		? PROGRESS_VACUUM_PHASE_VACUUM_INDEX
+		: PROGRESS_VACUUM_PHASE_INDEX_CLEANUP;
+	progress_val[1] = (int64) RelationGetRelid(indrel);
+	pgstat_progress_update_multi_param(2, progress_index, progress_val);
+
 	switch (indstats->status)
 	{
 		case PARALLEL_INDVAC_STATUS_NEED_BULKDELETE:
@@ -1152,6 +1170,8 @@ parallel_vacuum_process_one_index(ParallelVacuumState *pvs, Relation indrel,
 				 RelationGetRelationName(indrel));
 	}
 
+	pgstat_progress_update_multi_param(3, reset_index, reset_val);
+
 	/*
 	 * Copy the index bulk-deletion result returned from ambulkdelete and
 	 * amvacuumcleanup to the DSM segment if it's the first cycle because they
@@ -1230,8 +1250,8 @@ parallel_vacuum_index_is_parallel_safe(Relation indrel, int num_index_scans,
 /*
  * Perform work within a launched parallel process.
  *
- * Since parallel vacuum workers perform only index vacuum or index cleanup,
- * we don't need to report progress information.
+ * Parallel vacuum workers perform only index vacuum or index cleanup; they
+ * report progress for the index they are processing.
  */
 void
 parallel_vacuum_main(dsm_segment *seg, shm_toc *toc)
@@ -1355,6 +1375,9 @@ parallel_vacuum_main(dsm_segment *seg, shm_toc *toc)
 	/* Prepare to track buffer usage during parallel execution */
 	InstrStartParallelQuery();
 
+	/* Register this worker for vacuum progress reporting */
+	pgstat_progress_start_command(PROGRESS_COMMAND_VACUUM, shared->relid);
+
 	/* Process indexes to perform vacuum/cleanup */
 	parallel_vacuum_process_safe_indexes(&pvs);
 
@@ -1374,6 +1397,8 @@ parallel_vacuum_main(dsm_segment *seg, shm_toc *toc)
 	/* Pop the error context stack */
 	error_context_stack = errcallback.previous;
 
+	pgstat_progress_end_command();
+
 	vac_close_indexes(nindexes, indrels, RowExclusiveLock);
 	table_close(rel, ShareUpdateExclusiveLock);
 	FreeAccessStrategy(pvs.bstrategy);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 外部函数：`ereport`
- 外部函数：`errcontext`
- 外部函数：`get_namespace_name`
- 外部函数：`level`
- 外部函数：`memcpy`
- 外部函数：`parallel_vacuum_process_unsafe_indexes`
- 外部函数：`pfree`
- 外部函数：`pg_atomic_add_fetch_u32`
- 外部函数：`pg_atomic_fetch_add_u32`
- 外部函数：`pg_atomic_read_u32`
- 外部函数：`pg_atomic_sub_fetch_u32`
- 外部函数：`pgstat_progress_parallel_incr_param`
- 外部函数：`pgstat_report_activity`
- 外部函数：`pgstat_report_query_id`
- 外部函数：`pstrdup`
- 外部函数：`shm_toc_lookup`
- 外部函数：`snapshot`
- 外部函数：`table_close`
- 外部函数：`table_open`
- 外部函数：`vac_bulkdel_one_index`
- 外部函数：`vac_cleanup_one_index`
- 外部函数：`vac_close_indexes`
- 外部函数：`vac_open_indexes`
- 外部函数：`vacuum`
- 外部函数：`worker`
- 大写宏：`BAS_VACUUM`
- 大写宏：`BLCKSZ`
- 大写宏：`DEBUG1`
- 大写宏：`DEBUG2`
- 大写宏：`DSA`
- 大写宏：`DSM`
- 大写宏：`ERROR`
- 大写宏：`GUC`
- 大写宏：`NULL`
- 大写宏：`OID`
- 大写宏：`PARALLEL_INDVAC_STATUS_COMPLETED`
- 大写宏：`PARALLEL_INDVAC_STATUS_INITIAL`
- 大写宏：`PARALLEL_INDVAC_STATUS_NEED_BULKDELETE`
- 大写宏：`PARALLEL_INDVAC_STATUS_NEED_CLEANUP`
- 大写宏：`PROC_IN_VACUUM`
- 大写宏：`PROC_IS_AUTOVACUUM`
- 大写宏：`PROC_VACUUM_FOR_WRAPAROUND`
- 大写宏：`PROC_XMIN_FLAGS`
- 大写宏：`PROGRESS_VACUUM_DELAY_TIME`
- 大写宏：`PROGRESS_VACUUM_INDEXES_PROCESSED`
- 大写宏：`STATE_RUNNING`
- 大写宏：`VACUUM`
- 大写宏：`VERBOSE`
- 大写宏：`WAL`
- 外部类型：`APIs`
- 外部类型：`Access`
- 外部类型：`Apply`
- 外部类型：`Buffer`
- 外部类型：`BufferUsage`
- 外部类型：`Call`
- 外部类型：`Copy`
- 外部类型：`Copying`
- 外部类型：`Cost`
- 外部类型：`Counter`
- 外部类型：`Do`
- 外部类型：`Done`
- 外部类型：`During`
- 外部类型：`Each`
- 外部类型：`ErrorContextCallback`
- 外部类型：`Fields`
- 外部类型：`Find`
- 外部类型：`For`
- 外部类型：`Fortunately`
- 外部类型：`Free`
- 外部类型：`Get`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Increment`
- 外部类型：`IndexBulkDeleteResult`
- 外部类型：`IndexVacuumInfo`
- 外部类型：`Individual`
- 外部类型：`It`
- 外部类型：`Loop`
- 外部类型：`MyProc`
- 外部类型：`No`
- 外部类型：`Note`
- 外部类型：`Number`
- 外部类型：`Oid`
- 外部类型：`Open`
- 外部类型：`Otherwise`
- 外部类型：`PVIndStats`
- 外部类型：`PVShared`
- 外部类型：`PVSharedCostParams`
- 外部类型：`Parallel`
- 外部类型：`ParallelVacuumState`
- 外部类型：`ParallelWorkerNumber`
- 外部类型：`Parameters`
- 外部类型：`Perform`
- 外部类型：`Pop`
- 外部类型：`Prepare`
- 外部类型：`ProcArrayInstallRestoredXmin`
- 外部类型：`Process`
- 外部类型：`Quick`
- 外部类型：`Really`
- 外部类型：`Relation`
- 外部类型：`Report`
- 外部类型：`Reset`
- 外部类型：`Return`
- 外部类型：`RowExclusiveLock`
- 外部类型：`Set`
- 外部类型：`Setup`
- 外部类型：`ShareUpdateExclusiveLock`
- 外部类型：`Shared`
- 外部类型：`Since`
- 外部类型：`Skip`
- 外部类型：`So`
- 外部类型：`Statistics`
- 外部类型：`Strategy`
- 外部类型：`Target`
- 外部类型：`The`
- 外部类型：`These`
- 外部类型：`They`
- 外部类型：`This`
- 外部类型：`TidStore`
- 外部类型：`Track`
- 外部类型：`Update`
- 外部类型：`VacDeadItemsInfo`
- 外部类型：`VacuumActiveNWorkers`
- 外部类型：`VacuumCostBalance`
- 外部类型：`VacuumCostBalanceLocal`
- 外部类型：`VacuumCostDelay`
- 外部类型：`VacuumCostLimit`
- 外部类型：`VacuumCostPageDirty`
- 外部类型：`VacuumCostPageHit`
- 外部类型：`VacuumCostPageMiss`
- 外部类型：`VacuumSharedCostBalance`
- 外部类型：`WalUsage`
- 外部类型：`We`
- 外部类型：`slock_t`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-2ba94b1ebf` → 本草稿移入 `cases/defect/auto-postgres-2ba94b1ebf/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
