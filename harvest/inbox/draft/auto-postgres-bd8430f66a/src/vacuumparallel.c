// AUTO-DRAFT from postgres/postgres PR #cc053b6e127763e4168f18585ca90a67600c270e
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
/* …（同文件无关代码省略）… */

	/*
	 * Initialize shared cost-based vacuum delay parameters if it's for
	 * autovacuum.
	 */
	if (shared->is_autovacuum)
	{
		parallel_vacuum_set_cost_parameters(&shared->cost_params);
		pg_atomic_init_u32(&shared->cost_params.generation, 1);
/* …（同文件无关代码省略）… */
static inline void
parallel_vacuum_set_cost_parameters(PVSharedCostParams *params)
{
	params->cost_delay = vacuum_cost_delay;
	params->cost_limit = vacuum_cost_limit;
	params->cost_page_dirty = VacuumCostPageDirty;
	params->cost_page_hit = VacuumCostPageHit;
	params->cost_page_miss = VacuumCostPageMiss;
}
