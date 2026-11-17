// AUTO-DRAFT from torvalds/linux PR #666539eecdf4db623f5e3c3e55cf9f2dfad24aa4
#define ASSOC_ARRAY_FAN_OUT		16	/* Number of slots per node */
/* …（同文件无关代码省略）… */
	struct assoc_array_ptr		**leaf_p;
	struct assoc_array_ptr		*dead_leaf;
	struct assoc_array_ptr		*new_meta[3];
	struct assoc_array_ptr		*excised_meta[1];  // <<< BUG ANCHOR
	struct assoc_array_ptr		*excised_subtree;
	struct assoc_array_ptr		**set_backpointers[ASSOC_ARRAY_FAN_OUT];
	struct assoc_array_ptr		*set_backpointers_to;
