// AUTO-DRAFT from postgres/postgres PR #1b5dd3a24a48659e8413ef8ddd8adb6653507a9d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

	prunestate->other_subplans = bms_copy(pruneinfo->other_subplans);
	prunestate->do_initial_prune = false;	/* may be set below */
	prunestate->do_exec_prune = false;	/* may be set below */
	prunestate->num_partprunedata = n_part_hierarchies;

	/*
/* …（同文件无关代码省略）… */
	Assert(parent_plan != NULL);
	estate = parent_plan->state;

	/*
	 * No need to fix subplans maps if initial pruning didn't eliminate any
	 * subplans.
