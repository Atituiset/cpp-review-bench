// AUTO-DRAFT from torvalds/linux PR #762122d75e50e4919ef77afc2dffdc4931c5be87
#endif	/* CONFIG_EXT_SUB_SCHED */
	}
  // <<< BUG ANCHOR
	if (unlikely(!SCX_HAS_OP(sch, dispatch)) || !scx_rq_online(rq))
		return SCX_DSP_NONE;

	dspc->rq = rq;
