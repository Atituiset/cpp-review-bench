// AUTO-DRAFT from postgres/postgres PR #73e69062c120f669f71531dabeacfb8041c85daf
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	}
}

/*
 * ExecInitPartitionExecPruning
 *		Initialize the data structures needed for runtime "exec" partition
/* …（同文件无关代码省略）… */
	prunestate->other_subplans = bms_copy(pruneinfo->other_subplans);
	prunestate->do_initial_prune = false;	/* may be set below */
	prunestate->do_exec_prune = false;	/* may be set below */
	prunestate->initialized = false;	/* set in
										 * InitExecPartitionPruneContexts */
	prunestate->num_partprunedata = n_part_hierarchies;

	/*
/* …（同文件无关代码省略）… */
	Assert(parent_plan != NULL);
	estate = parent_plan->state;

	/*
	 * PartitionPruneStates are sometimes shared and this one may have been
	 * initialized already.  Sharing of states occurs for EPQ, for example.
	 * See EvalPlanQualStart().
	 */
	if (prunestate->initialized)
		return;

	prunestate->initialized = true;

	/*
	 * No need to fix subplans maps if initial pruning didn't eliminate any
	 * subplans.
