// AUTO-DRAFT from postgres/postgres PR #73e69062c120f669f71531dabeacfb8041c85daf
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
typedef struct PartitionedRelPruningData
{
	Relation	partrel;
	int			nparts;
	int		   *subplan_map;
	int		   *subpart_map;
	int		   *leafpart_rti_map;
	Bitmapset  *present_parts;
	List	   *initial_pruning_steps;
	List	   *exec_pruning_steps;
	PartitionPruneContext initial_context;
	PartitionPruneContext exec_context;
} PartitionedRelPruningData;
/* …（同文件无关代码省略）… */
typedef struct PartitionPruningData
{
	int			num_partrelprunedata;	/* number of array entries */
	PartitionedRelPruningData partrelprunedata[FLEXIBLE_ARRAY_MEMBER];
} PartitionPruningData;
/* …（同文件无关代码省略）… */
 *						startup (at any hierarchy level).
 * do_exec_prune		true if pruning should be performed during
 *						executor run (at any hierarchy level).
 * initialized			true if InitExecPartitionPruneContexts has been called
 *						on this PartitionPruneState
 * num_partprunedata	Number of items in "partprunedata" array.
 * partprunedata		Array of PartitionPruningData pointers for the plan's
 *						partitioned relation(s), one for each partitioning
/* …（同文件无关代码省略）… */
typedef struct PartitionPruneState
{
	ExprContext *econtext;
	Bitmapset  *execparamids;
	Bitmapset  *other_subplans;
	MemoryContext prune_context;
	bool		do_initial_prune;
	bool		do_exec_prune;
	bool		initialized;
	int			num_partprunedata;
	PartitionPruningData *partprunedata[FLEXIBLE_ARRAY_MEMBER];
} PartitionPruneState;

extern void ExecDoInitialPruning(EState *estate);
extern PartitionPruneState *ExecInitPartitionExecPruning(PlanState *planstate,
														 int n_total_subplans,
														 int part_prune_index,
