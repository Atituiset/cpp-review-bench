// AUTO-DRAFT from torvalds/linux PR #c3b510de420d70def08190083d388e0873c1aa84

	int nr_threaded_children;	/* # of live threaded child cgroups */

	/* sequence number for cgroup.kill, serialized by css_set_lock. */
	unsigned int kill_seq;

	struct kernfs_node *kn;		/* cgroup kernfs entry */
