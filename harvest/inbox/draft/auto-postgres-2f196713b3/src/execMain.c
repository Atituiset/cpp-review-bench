// AUTO-DRAFT from postgres/postgres PR #73e69062c120f669f71531dabeacfb8041c85daf
	 * and MergeAppend subplans as the parent did.
	 */
	rcestate->es_part_prune_infos = parentestate->es_part_prune_infos;
	rcestate->es_part_prune_states = parentestate->es_part_prune_states;  // <<< BUG ANCHOR
	rcestate->es_part_prune_results = parentestate->es_part_prune_results;

	/* We'll also borrow the es_partition_directory from the parent state */
	rcestate->es_partition_directory = parentestate->es_partition_directory;

	/*
	 * Initialize private state information for each SubPlan.  We must do this
	 * before running ExecInitNode on the main query tree, since
