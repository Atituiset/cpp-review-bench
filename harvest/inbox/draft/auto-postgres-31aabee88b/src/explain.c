// AUTO-DRAFT from postgres/postgres PR #edbb1d2dcb18d0706e644d3048a21e4475debd95
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#define BYTES_TO_KILOBYTES(b) (((b) + 1023) / 1024)
/* …（同文件无关代码省略）… */
static void show_material_info(MaterialState *mstate, ExplainState *es);
static void show_windowagg_info(WindowAggState *winstate, ExplainState *es);
static void show_ctescan_info(CteScanState *ctescanstate, ExplainState *es);
static void show_table_func_scan_info(TableFuncScanState *tscanstate,
									  ExplainState *es);
static void show_recursive_union_info(RecursiveUnionState *rstate,
/* …（同文件无关代码省略）… */
			if (plan->qual)
				show_instrumentation_count("Rows Removed by Filter", 1,
										   planstate, es);
			break;
		case T_TableFuncScan:
			if (es->verbose)
/* …（同文件无关代码省略）… */
static void
show_storage_info(bool diskUsed, int64 maxSpaceUsed, ExplainState *es)
{
	int64		maxSpaceUsedKB = BYTES_TO_KILOBYTES(maxSpaceUsed);
	const char *storageType = diskUsed ? "Disk" : "Memory";

	if (es->format != EXPLAIN_FORMAT_TEXT)
	{
		ExplainPropertyText("Storage", storageType, es);
		ExplainPropertyInteger("Maximum Storage", "kB", maxSpaceUsedKB, es);
	}
	else
	{
		ExplainIndentText(es);
		appendStringInfo(es->str,
						 "Storage: %s  Maximum Storage: " INT64_FORMAT "kB\n",
						 storageType,
						 maxSpaceUsedKB);
	}
}
/* …（同文件无关代码省略）… */
static void
show_material_info(MaterialState *mstate, ExplainState *es)
{
	bool		usedDisk;
	int64		maxSpaceUsed;

	Tuplestorestate *tupstore = mstate->tuplestorestate;

	if (!es->analyze)
		return;

	/*
	 * Nothing to show for the leader if execution didn't get as far as
	 * creating the tuplestore.
	 */
	if (tupstore != NULL)
	{
		tuplestore_get_stats(tupstore, &usedDisk, &maxSpaceUsed);
		show_storage_info(usedDisk, maxSpaceUsed, es);
	}

	if (mstate->shared_info == NULL)
		return;

	/* Show details from parallel workers */
	for (int n = 0; n < mstate->shared_info->num_workers; n++)
	{
		MaterialInstrumentation *si;

		si = &mstate->shared_info->sinstrument[n];

		/* Skip workers that didn't create a tuplestore */
		if (si->maxSpaceUsed == 0)
			continue;

		if (es->workers_state)
			ExplainOpenWorker(n, es);

		show_storage_info(si->usedDisk, si->maxSpaceUsed, es);

		if (es->workers_state)
			ExplainCloseWorker(n, es);
	}
}
/* …（同文件无关代码省略）… */
static void
show_windowagg_info(WindowAggState *winstate, ExplainState *es)
{
	bool		usedDisk;
	int64		maxSpaceUsed;

	Tuplestorestate *tupstore = winstate->buffer;

	/*
	 * Nothing to show if ANALYZE option wasn't used or if execution didn't
	 * get as far as creating the tuplestore.
	 */
	if (!es->analyze || tupstore == NULL)
		return;

	tuplestore_get_stats(tupstore, &usedDisk, &maxSpaceUsed);
	show_storage_info(usedDisk, maxSpaceUsed, es);
}
/* …（同文件无关代码省略）… */
static void
show_ctescan_info(CteScanState *ctescanstate, ExplainState *es)
{
	bool		usedDisk;
	int64		maxSpaceUsed;

	Tuplestorestate *tupstore = ctescanstate->leader->cte_table;

	if (!es->analyze || tupstore == NULL)
		return;

	tuplestore_get_stats(tupstore, &usedDisk, &maxSpaceUsed);
	show_storage_info(usedDisk, maxSpaceUsed, es);
}

/*
 * Show information on Table Function Scan node, storage method and maximum
 * memory/disk space used.
/* …（同文件无关代码省略）… */
static void
show_table_func_scan_info(TableFuncScanState *tscanstate, ExplainState *es)
{
	bool		usedDisk;
	int64		maxSpaceUsed;

	Tuplestorestate *tupstore = tscanstate->tupstore;

	if (!es->analyze || tupstore == NULL)
		return;

	tuplestore_get_stats(tupstore, &usedDisk, &maxSpaceUsed);
	show_storage_info(usedDisk, maxSpaceUsed, es);
}
/* …（同文件无关代码省略）… */
static void
show_recursive_union_info(RecursiveUnionState *rstate, ExplainState *es)
{
	bool		maxUsedDisk,
				tempUsedDisk;
	int64		maxSpaceUsed,
				tempSpaceUsed;

	if (!es->analyze)
		return;

	/*
	 * Recursive union node uses two tuplestores.  We employ the storage type
	 * from one of them which consumed more memory/disk than the other.  The
	 * storage size is sum of the two.
	 */
	tuplestore_get_stats(rstate->working_table, &tempUsedDisk,
						 &tempSpaceUsed);
	tuplestore_get_stats(rstate->intermediate_table, &maxUsedDisk,
						 &maxSpaceUsed);

	if (tempSpaceUsed > maxSpaceUsed)
		maxUsedDisk = tempUsedDisk;

	maxSpaceUsed += tempSpaceUsed;
	show_storage_info(maxUsedDisk, maxSpaceUsed, es);
}
/* …（同文件无关代码省略）… */
static void
show_instrumentation_count(const char *qlabel, int which,
						   PlanState *planstate, ExplainState *es)
{
	double		nfiltered;
	double		nloops;

	if (!es->analyze || !planstate->instrument)
		return;

	if (which == 2)
		nfiltered = planstate->instrument->nfiltered2;
	else
		nfiltered = planstate->instrument->nfiltered1;
	nloops = planstate->instrument->nloops;

	/* In text mode, suppress zero counts; they're not interesting enough */
	if (nfiltered > 0 || es->format != EXPLAIN_FORMAT_TEXT)
	{
		if (nloops > 0)
			ExplainPropertyFloat(qlabel, NULL, nfiltered / nloops, 0, es);
		else
			ExplainPropertyFloat(qlabel, NULL, 0.0, 0, es);
	}
}
/* …（同文件无关代码省略）… */
static void
ExplainOpenWorker(int n, ExplainState *es)
{
	ExplainWorkersState *wstate = es->workers_state;

	Assert(wstate);
	Assert(n >= 0 && n < wstate->num_workers);

	/* Save prior output buffer pointer */
	wstate->prev_str = es->str;

	if (!wstate->worker_inited[n])
	{
		/* First time through, so create the buffer for this worker */
		initStringInfo(&wstate->worker_str[n]);
		es->str = &wstate->worker_str[n];

		/*
		 * Push suitable initial formatting state for this worker's field
		 * group.  We allow one extra logical nesting level, since this group
		 * will eventually be wrapped in an outer "Workers" group.
		 */
		ExplainOpenSetAsideGroup("Worker", NULL, true, 2, es);

		/*
		 * In non-TEXT formats we always emit a "Worker Number" field, even if
		 * there's no other data for this worker.
		 */
		if (es->format != EXPLAIN_FORMAT_TEXT)
			ExplainPropertyInteger("Worker Number", NULL, n, es);

		wstate->worker_inited[n] = true;
	}
	else
	{
		/* Resuming output f
