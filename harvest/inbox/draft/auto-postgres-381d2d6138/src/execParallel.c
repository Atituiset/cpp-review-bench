// AUTO-DRAFT from postgres/postgres PR #633dee1bd1b41145f81619423d573754c7ad2b9e
	int			instrument_offset;
	int			num_workers;
	int			num_plan_nodes;
	int			plan_node_id[FLEXIBLE_ARRAY_MEMBER];  // <<< BUG ANCHOR

	/*
	 * Array of num_plan_nodes * num_workers NodeInstrumentation objects
