// AUTO-DRAFT from torvalds/linux PR #b8eb5fd5bdb4c03758b056c70e5e1d962ca89757
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
struct tmpmasks {
	cpumask_var_t addmask, delmask;	/* For partition root */
	cpumask_var_t new_cpus;		/* For update_cpumasks_hier() */
};
/* …（同文件无关代码省略）… */
static inline bool cpuset_v2(void)
{
	return !IS_ENABLED(CONFIG_CPUSETS_V1) ||
		cgroup_subsys_on_dfl(cpuset_cgrp_subsys);
}
/* …（同文件无关代码省略）… */
static inline bool is_in_v2_mode(void)
{
	return cpuset_v2() || READ_ONCE(cpuset_v2_mode);
}
/* …（同文件无关代码省略）… */
static inline int alloc_cpumasks(cpumask_var_t *pmasks[], u32 size)
{
	int i;

	for (i = 0; i < size; i++) {
		if (!zalloc_cpumask_var(pmasks[i], GFP_KERNEL)) {
			while (--i >= 0)
				free_cpumask_var(*pmasks[i]);
			return -ENOMEM;
		}
	}
	return 0;
}
/* …（同文件无关代码省略）… */
static inline int alloc_tmpmasks(struct tmpmasks *tmp)
{
	/*
	 * Array of pointers to the three cpumask_var_t fields in tmpmasks.
	 * Note: Array size must match actual number of masks (3)
	 */
	cpumask_var_t *pmask[3] = {
		&tmp->new_cpus,
		&tmp->addmask,
		&tmp->delmask
	};

	return alloc_cpumasks(pmask, ARRAY_SIZE(pmask));
}
/* …（同文件无关代码省略）… */
	static cpumask_t new_cpus;
	static nodemask_t new_mems;
	bool cpus_updated, mems_updated;
	bool on_dfl = is_in_v2_mode();
	struct tmpmasks tmp, *ptmp = NULL;

	if (on_dfl && !alloc_tmpmasks(&tmp))
		ptmp = &tmp;

	lockdep_assert_cpus_held();
	mutex_lock(&cpuset_mutex);

	/* fetch the available cpus/mems and find out which changed how */
	cpumask_copy(&new_cpus, cpu_active_mask);
	new_mems = node_states[N_MEMORY];
