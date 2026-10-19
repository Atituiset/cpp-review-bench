// AUTO-DRAFT from torvalds/linux PR #88328f850d956d267d11e2ee225ee1eaa902b8e0
static inline unsigned long avg_vruntime_weight(struct cfs_rq *cfs_rq, unsigned long w)
{
#ifdef CONFIG_64BIT
	if (cfs_rq->sum_shift)
		w = max(2UL, w >> cfs_rq->sum_shift);
#endif
	return w;
}
/* …（同文件无关代码省略）… */
		load = cfs_rq->sum_weight;
		if (curr && curr->on_rq)
			load += avg_vruntime_weight(cfs_rq, curr->h_load.weight);
  // <<< BUG ANCHOR
		weight = avg_vruntime_weight(cfs_rq, se->h_load.weight);
		lag *= load + weight;
		if (WARN_ON_ONCE(!load))
			load = 1;
		lag = div64_long(lag, load);

		/*
		 * A heavy entity (relative to the tree) will pull the
