// AUTO-DRAFT from torvalds/linux PR #762122d75e50e4919ef77afc2dffdc4931c5be87
	s32			holding_cpu;
	s32			selected_cpu;
	s32			runnable_cpu;	/* cpu @p is runnable on, -1 if not */
	struct task_struct	*kf_tasks[2];	/* see SCX_CALL_OP_TASK() */

	struct list_head	runnable_node;	/* rq->scx.runnable_list */
