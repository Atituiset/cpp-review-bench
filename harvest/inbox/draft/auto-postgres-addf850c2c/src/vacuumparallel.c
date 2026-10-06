// AUTO-DRAFT from postgres/postgres PR #92a734bf5c7ace23131b711025c75aacd8144482
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <time.h>
  // <<< BUG ANCHOR
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

/*
/* …（同文件无关代码省略）… */
	params->cost_page_dirty = VacuumCostPageDirty;
	params->cost_page_hit = VacuumCostPageHit;
	params->cost_page_miss = VacuumCostPageMiss;
}

/*
/* …（同文件无关代码省略）… */
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

/*
/* …（同文件无关代码省略）… */
		vacuum_cost_limit == pv_shared_cost_params->cost_limit &&
		VacuumCostPageDirty == pv_shared_cost_params->cost_page_dirty &&
		VacuumCostPageHit == pv_shared_cost_params->cost_page_hit &&
		VacuumCostPageMiss == pv_shared_cost_params->cost_page_miss)
		return;

	/* Update the shared delay parameters */
/* …（同文件无关代码省略）… */
						  &wal_usage[ParallelWorkerNumber]);

	/* Report any remaining cost-based vacuum delay time */
	if (track_cost_delay_timing)
		pgstat_progress_parallel_incr_param(PROGRESS_VACUUM_DELAY_TIME,
											parallel_vacuum_worker_delay_ns);
