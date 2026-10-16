// AUTO-DRAFT from postgres/postgres PR #a39e53a700c5b728f2f7dab964e33b157979fb3a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <time.h>

#include "storage/bufmgr.h"
#include "storage/proc.h"
#include "tcop/tcopprot.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"

/* …（同文件无关代码省略）… */
#define PARALLEL_VACUUM_KEY_SHARED			1
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
	bool		track_cost_delay_timing;
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

	elog(DEBUG1, "starting parallel vacuum worker");

	shared = (PVShared *) shm_toc_lookup(toc, PARALLEL_VACUUM_KEY_SHARED, false);

	/* Set debug_query_string for individual workers */
