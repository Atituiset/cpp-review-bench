// AUTO-DRAFT from torvalds/linux PR #673dab7eac1618b6622bbee9eb2aaa47817631e3
  // <<< BUG ANCHOR
#ifdef CONFIG_SCHED_CACHE
	struct callback_head		cache_work;
	int				preferred_llc;
	/* 1: task was enqueued to its preferred LLC, 0 otherwise */
	int				pref_llc_queued;
/* …（同文件无关代码省略）… */
struct sched_cache_time {
	u64 runtime;
	unsigned long epoch;
};

struct sched_cache_stat {
	struct sched_cache_time __percpu *pcpu_sched;
	raw_spinlock_t lock;
	unsigned long epoch;
	u64 nr_running_avg;
	unsigned long next_scan;
	unsigned long footprint;
	int cpu;
} ____cacheline_aligned_in_smp;

#else

struct sched_cache_stat { };

#endif
