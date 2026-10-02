// AUTO-DRAFT from postgres/postgres PR #1378aa13430e264a990a18597e9d8fdae740927e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#define PARALLEL_VACUUM_KEY_SHARED			1
#define PARALLEL_VACUUM_KEY_QUERY_TEXT		2
#define PARALLEL_VACUUM_KEY_BUFFER_USAGE	3
#define PARALLEL_VACUUM_KEY_WAL_USAGE		4
#define PARALLEL_VACUUM_KEY_INDEX_STATS		5
/* …（同文件无关代码省略）… */
typedef struct PVSharedCostParams
{
	/*
	 * The generation counter is incremented by the leader process each time
	 * it updates the shared cost-based vacuum delay parameters. Parallel
	 * vacuum workers compare it with their local generation,
	 * shared_params_generation_local, to detect whether they need to refresh
	 * their local parameters. The generation starts from 1 so that a freshly
	 * started worker (whose local copy is 0) will always load the initial
	 * parameters on its first check.
	 */
	pg_atomic_uint32 generation;

	slock_t		mutex;			/* protects all fields below */

	/* Parameters to share with parallel workers */
	double		cost_delay;
	int			cost_limit;
	int			cost_page_dirty;
	int			cost_page_hit;
	int			cost_page_miss;
} PVSharedCostParams;
/* …（同文件无关代码省略）… */
typedef struct PVShared
{
	/*
	 * Target table relid, log level (for messages about parallel workers
	 * launched during VACUUM VERBOSE) and query ID.  These fields are not
	 * modified during the parallel vacuum.
	 */
	Oid			relid;
	int			elevel;
	int64		queryid;

	/*
	 * Fields for both index vacuum and cleanup.
	 *
	 * reltuples is the total number of input heap tuples.  We set either old
	 * live tuples in the index vacuum case or the new live tuples in the
	 * index cleanup case.
	 *
	 * estimated_count is true if reltuples is an estimated value.  (Note that
	 * reltuples could be -1 in this case, indicating we have no idea.)
	 */
	double		reltuples;
	bool		estimated_count;

	/*
	 * In single process vacuum we could consume more memory during index
	 * vacuuming or cleanup apart from the memory for heap scanning.  In
	 * parallel vacuum, since individual vacuum workers can consume memory
	 * equal to maintenance_work_mem, the new maintenance_work_mem for each
	 * worker is set such that the parallel operation doesn't consume more
	 * memory than single process vacuum.
	 */
	int			maintenance_work_mem_worker;

	/*
	 * The number of buffers each worker's Buffer Access Strategy ring should
	 * contain.
	 */
	int			ring_nbuffers;

	/*
	 * Shared vacuum cost balance.  During parallel vacuum,
	 * VacuumSharedCostBalance points to this value and it accumulates the
	 * balance of each parallel vacuum worker.
	 */
	pg_atomic_uint32 cost_balance;

	/*
	 * Number of active parallel workers.  This is used for computing the
	 * minimum threshold of the vacuum cost balance before a worker sleeps for
	 * cost-based delay.
	 */
	pg_atomic_uint32 active_nworkers;

	/* Counter for vacuuming and cleanup */
	pg_atomic_uint32 idx;

	/* DSA handle where the TidStore lives */
	dsa_handle	dead_items_dsa_handle;

	/* DSA pointer to the shared TidStore */
	dsa_pointer dead_items_handle;

	/* Statistics of shared dead items */
	VacDeadItemsInfo dead_items_info;

	/*
	 * If 'true' then we are running parallel autovacuum. Otherwise, we are
	 * running parallel maintenance VACUUM.
	 */
	bool		is_autovacuum;

	/*
	 * Cost-based vacuum delay parameters shared between the autovacuum leader
	 * and its parallel workers.
	 */
	PVSharedCostParams cost_params;
} PVShared;
/* …（同文件无关代码省略）… */
typedef enum PVIndVacStatus
{
	PARALLEL_INDVAC_STATUS_INITIAL = 0,
	PARALLEL_INDVAC_STATUS_NEED_BULKDELETE,
	PARALLEL_INDVAC_STATUS_NEED_CLEANUP,
	PARALLEL_INDVAC_STATUS_COMPLETED,
} PVIndVacStatus;
/* …（同文件无关代码省略）… */
typedef struct PVIndStats
{
	/*
	 * The following two fields are set by leader process before executing
	 * parallel index vacuum or parallel index cleanup.  These fields are not
	 * fixed for the entire VACUUM operation.  They are only fixed for an
	 * individual parallel index vacuum and cleanup.
	 *
	 * parallel_workers_can_process is true if both leader and worker can
	 * process the index, otherwise only leader can process it.
	 */
	PVIndVacStatus status;
	bool		parallel_workers_can_process;

	/*
	 * Individual worker or leader stores the result of index vacuum or
	 * cleanup.
	 */
	bool		istat_updated;	/* are the stats updated? */
	IndexBulkDeleteResult istat;
} PVIndStats;
/* …（同文件无关代码省略）… */
void
parallel_vacuum_update_shared_delay_params(void)
{
	uint32		params_generation;

	Assert(IsParallelWorker());

	/* Quick return if the worker is not running for the autovacuum */
	if (pv_shared_cost_params == NULL)
		return;

	params_generation = pg_atomic_read_u32(&pv_shared_cost_params->generation);
	Assert(shared_params_generation_local <= params_generation);

	/* Return if parameters had not changed in the leader */
	if (params_generation == shared_params_generation_local)
		return;

	SpinLockAcquire(&pv_shared_cost_params->mutex);
	VacuumCostDelay = pv_shared_cost_params->cost_delay;
	VacuumCostLimit = pv_shared_cost_params->cost_limit;
	VacuumCostPageDirty = pv_shared_cost_params->cost_page_dirty;
	VacuumCostPageHit = pv_shared_cost_params->cost_page_hit;
	VacuumCostPageMiss = pv_shared_cost_params->cost_page_miss;
	SpinLockRelease(&pv_shared_cost_params->mutex);

	VacuumUpdateCosts();

	shared_params_generation_local = params_generation;

	elog(DEBUG2,
		 "parallel autovacuum worker updated cost params: cost_limit=%d, cost_delay=%g, cost_page_miss=%d, cost_page_dirty=%d, cost_page_hit=%d",
		 vacuum_cost_limit,
		 vacuum_cost_delay,
		 VacuumCostPageMiss,
		 VacuumCostPageDirty,
		 VacuumCostPageHit);
}
/* …（同文件无关代码省略）… */
static void
parallel_vacuum_process_safe_indexes(ParallelVacuumState *pvs)
{
	/*
	 * Increment the active worker count if we are able to launch any worker.
	 */
	if (VacuumActiveNWorkers)
		pg_atomic_add_fetch_u32(VacuumActiveNWorkers, 1);

	/* Loop until all indexes are vacuumed */
	for (;;)
	{
		int			idx;
		PVIndStats *indstats;

		/* Get an index number to process */
		idx = pg_atomic_fetch_add_u32(&(pvs->shared->idx), 1);

		/* Done for all indexes? */
		if (idx >= pvs->nindexes)
			break;

		indstats = &(pvs->indstats[idx]);

		/*
		 * Skip vacuuming index that is unsafe for workers or has an
		 * unsuitable target for parallel index vacuum (this is vacuumed in
		 * parallel_vacuum_process_unsafe_indexes() by the leader).
		 */
		if (!indstats->parallel_workers_can_process)
			continue;

		/* Do vacuum or cleanup of the index */
		parallel_vacuum_process_one_index(pvs, pvs->indrels[idx], indstats);
	}

	/*
	 * We have completed the index vacuum so decrement the active worker
	 * count.
	 */
	if (VacuumActiveNWorkers)
		pg_atomic_sub_fetch_u32(VacuumActiveNWorkers, 1);
}
/* …（同文件无关代码省略）… */
static void
parallel_vacuum_process_one_index(ParallelVacuumState *pvs, Relation indrel,
								  PVIndStats *indstats)
{
	IndexBulkDeleteResult *istat = NULL;
	IndexBulkDeleteResult *istat_res;
	IndexVacuumInfo ivinfo;

	/*
	 * Update the pointer to the corresponding bulk-deletion result if someone
	 * has already updated it
	 */
	if (indstats->istat_updated)
		istat = &(indstats->istat);

	ivinfo.index = indrel;
	ivinfo.heaprel = pvs->heaprel;
	ivinfo.analyze_only = false;
	ivinfo.is_autovacuum = pvs->shared->is_autovacuum;
	ivinfo.report_progress = false;
	ivinfo.message_level = DEBUG2;
	ivinfo.estimated_count = pvs->shared->estimated_count;
	ivinfo.num_heap_tuples = pvs->shared->reltuples;
	ivinfo.strategy = pvs->bstrategy;

	/* Update error traceback information */
	pvs->indname = pstrdup(RelationGetRelationName(indrel));
	pvs->status = indstats->status;

	switch (indstats->status)
	{
		case PARALLEL_INDVAC_STATUS_NEED_BULKDELETE:
			istat_res = vac_bulkdel_one_index(&ivinfo, istat, pvs->dead_items,
											  &pvs->shared->dead_items_info);
			break;
		case PARALLEL_INDVAC_STATUS_NEED_CLEANUP:
			istat_res = vac_cleanup_one_index(&ivinfo, istat);
			break;
		default:
			elog(ERROR, "unexpected parallel vacuum index status %d for index \"%s\"",
				 indstats->status,
				 RelationGetRelationName(indrel));
	}

	/*
	 * Copy the index bulk-deletion result returned from ambulkdelete and
	 * amvacuumcleanup to the DSM segment if it's the first cycle because they
	 * allocate locally and it's possible that an index will be vacuumed by a
	 * different vacuum process the next cycle.  Copying the result normally
	 * happens only the first time an index is vacuumed.  For any additional
	 * vacuum pass, we directly point to the result on the DSM segment and
	 * pass it to vacuum index APIs so that workers can update it directly.
	 *
	 * Since all vacuum workers write the bulk-deletion result at different
	 * slots we can write them without locking.
	 */
	if (!indstats->istat_updated && istat_res != NULL)
	{
		memcpy(&(indstats->istat), istat_res, sizeof(IndexBulkDeleteResult));
		indstats->istat_updated = true;

		/* Free the locally-allocated bulk-deletion result */
		pfree(istat_res);
	}

	/*
	 * Update the status to completed. No need to lock here since each worker
	 * touches different indexes.
	 */
	indstats->status = PARALLEL_INDVAC_STATUS_COMPLETED;

	/* Reset error traceback information */
	pvs->status = PARALLEL_INDVAC_STATUS_COMPLETED;
	pfree(pvs->indname);
	pvs->indname = NULL;

	/*
	 * Call the parallel variant of pgstat_progress_incr_param so workers can
	 * report progress of index vacuum to the leader.
	 */
	pgstat_progress_parallel_incr_param(PROGRESS_VACUUM_INDEXES_PROCESSED, 1);
}
/* …（同文件无关代码省略）… */
/*
 * Perform work within a launched parallel process.
 *
 * Since parallel vacuum workers perform only index vacuum or index cleanup,
 * we don't need to report progress information.
 */
void
parallel_vacuum_main(dsm_segment *seg, shm_toc *toc)
{
	ParallelVacuumState pvs;
	Relation	rel;
	Relation   *indrels;
	PVIndStats *indstats;
	PVShared   *shared;
	TidStore   *dead_items;
	BufferUsage *buffer_usage;
	WalUsage   *wal_usage;
	int			nindexes;
	char	   *sharedquery;
	ErrorContextCallback errcallback;

	/*
	 * A parallel vacuum worker carries only the PROC_IN_VACUUM flag. The
	 * leader, whether it's a backend running a VACUUM command or an
	 * autovacuum worker, sets PROC_IN_VACUUM when it starts vacuuming the
	 * table, and the worker inherits the flag by importing the leader's
	 * snapshot (see ProcArrayInstallRestoredXmin). The leader's other flags
	 * don't reach the worker: the snapshot import copies only the
	 * PROC_XMIN_FLAGS bits, so PROC_VACUUM_FOR_WRAPAROUND isn't carried over,
	 * and PROC_IS_AUTOVACUUM is never set on the worker in the first place
	 * since parallel workers run as regular background workers, not
	 * autovacuum workers.
	 */
	Assert(MyProc->statusFlags == PROC_IN_VACUUM);

	elog(DEBUG1, "starting parallel vacuum worker");

	shared = (PVShared *) shm_toc_lookup(toc, PARALLEL_VACUUM_KEY_SHARED, false);

	/* Set debug_query_string for individual workers */
	sharedquery = shm_toc_lookup(toc, PARALLEL_VACUUM_KEY_QUERY_TEXT, true);
	debug_query_string = sharedquery;
	pgstat_report_activity(STATE_RUNNING, debug_query_string);

	/* Track query ID */
	pgstat_report_query_id(shared->queryid, false);

	/*
	 * Open table.  The lock mode is the same as the leader process.  It's
	 * okay because the lock mode does not conflict among the parallel
	 * workers.
	 */
	rel = table_open(shared->relid, ShareUpdateExclusiveLock);

	/*
	 * Open all indexes. indrels are sorted in order by OID, which should be
	 * matched to the leader's one.
	 */
	vac_open_indexes(rel, RowExclusiveLock, &nindexes, &indrels);
	Assert(nindexes > 0);

	/*
	 * Apply the desired value of maintenance_work_mem within this process.
	 * Really we should use SetConfigOption() to change a GUC, but since we're
	 * already in parallel mode guc.c would complain about that.  Fortunately,
	 * by the same token guc.c will not let any user-defined code change it.
	 * So just avert your eyes while we do this:
	 */
	if (shared->maintenance_work_mem_worker > 0)
		maintenance_work_mem = shared->maintenance_work_mem_worker;

	/* Set index statistics */
	indstats = (PVIndStats *) shm_toc_lookup(toc,
											 PARALLEL_VACUUM_KEY_INDEX_STATS,
											 false);

	/* Find dead_items in shared memory */
	dead_items = TidStoreAttach(shared->dead_items_dsa_handle,
								shared->dead_items_handle);

	/* Set cost-based vacuum delay */
	if (shared->is_autovacuum)
	{
		/*
		 * Parallel autovacuum workers initialize cost-based delay parameters
		 * from the leader's shared state rather than GUC defaults, because
		 * the leader may have applied per-table or autovacuum-specific
		 * overrides. pv_shared_cost_params must be set before calling
		 * parallel_vacuum_update_shared_delay_params().
		 */
		pv_shared_cost_params = &(shared->cost_params);
		parallel_vacuum_update_shared_delay_params();
	}
	else
		VacuumUpdateCosts();

	VacuumCostBalance = 0;
	VacuumCostBalanceLocal = 0;
	VacuumSharedCostBalance = &(shared->cost_balance);
	VacuumActiveNWorkers = &(shared->active_nworkers);

	/* Set parallel vacuum state */
	pvs.indrels = indrels;
	pvs.nindexes = nindexes;
	pvs.indstats = indstats;
	pvs.shared = shared;
	pvs.dead_items = dead_items;
	pvs.relnamespace = get_namespace_name(RelationGetNamespace(rel));
	pvs.relname = pstrdup(RelationGetRelationName(rel));
	pvs.heaprel = rel;

	/* These fields will be filled during index vacuum or cleanup */
	pvs.indname = NULL;
	pvs.status = PARALLEL_INDVAC_STATUS_INITIAL;

	/* Each parallel VACUUM worker gets its own access strategy. */
	pvs.bstrategy = GetAccessStrategyWithSize(BAS_VACUUM,
											  shared->ring_nbuffers * (BLCKSZ / 1024));

	/* Setup error traceback support for ereport() */
	errcallback.callback = parallel_vacuum_error_callback;
	errcallback.arg = &pvs;
	errcallback.previous = error_context_stack;
	error_context_stack = &errcallback;

	/* Prepare to track buffer usage during parallel execution */
	InstrStartParallelQuery();

	/* Process indexes to perform vacuum/cleanup */
	parallel_vacuum_process_safe_indexes(&pvs);

	/* Report buffer/WAL usage during parallel execution */
	buffer_usage = shm_toc_lookup(toc, PARALLEL_VACUUM_KEY_BUFFER_USAGE, false);
	wal_usage = shm_toc_lookup(toc, PARALLEL_VACUUM_KEY_WAL_USAGE, false);
	InstrEndParallelQuery(&buffer_usage[ParallelWorkerNumber],
						  &wal_usage[ParallelWorkerNumber]);

	/* Report any remaining cost-based vacuum delay time */
	if (track_cost_delay_timing)
		pgstat_progress_parallel_incr_param(PROGRESS_VACUUM_DELAY_TIME,
											parallel_vacuum_worker_delay_ns);

	TidStoreDetach(dead_items);

	/* Pop the error context stack */
	error_context_stack = errcallback.previous;

	vac_close_indexes(nindexes, indrels, RowExclusiveLock);
	table_close(rel, ShareUpdateExclusiveLock);
	FreeAccessStrategy(pvs.bstrategy);

	if (shared->is_autovacuum)
		pv_shared_cost_params = NULL;
}
/* …（同文件无关代码省略）… */
static void
parallel_vacuum_error_callback(void *arg)
{
	ParallelVacuumState *errinfo = arg;

	switch (errinfo->status)
	{
		case PARALLEL_INDVAC_STATUS_NEED_BULKDELETE:
			errcontext("while vacuuming index \"%s\" of relation \"%s.%s\"",
					   errinfo->indname,
					   errinfo->relnamespace,
					   errinfo->relname);
			break;
		case PARALLEL_INDVAC_STATUS_NEED_CLEANUP:
			errcontext("while cleaning up index \"%s\" of relation \"%s.%s\"",
					   errinfo->indname,
					   errinfo->relnamespace,
					   errinfo->relname);
			break;
		case PARALLEL_INDVAC_STATUS_INITIAL:
		case PARALLEL_INDVAC_STATUS_COMPLETED:
		default:
			return;
	}
}
