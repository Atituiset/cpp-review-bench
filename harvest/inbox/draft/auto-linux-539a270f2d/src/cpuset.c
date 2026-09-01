// AUTO-DRAFT from torvalds/linux PR #c3b510de420d70def08190083d388e0873c1aa84
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#define PRS_ISOLATED		2
/* …（同文件无关代码省略）… */
		cpumask_copy(cs->effective_cpus, parent->effective_cpus);
}

/*
 * isolated_cpus_update - Update the isolated_cpus mask
 * @old_prs: old partition_root_state
/* …（同文件无关代码省略）… */
 */
static void isolated_cpus_update(int old_prs, int new_prs, struct cpumask *xcpus)
{
	WARN_ON_ONCE(old_prs == new_prs);
	lockdep_assert_held(&callback_lock);
	lockdep_assert_held(&cpuset_mutex);
	if (new_prs == PRS_ISOLATED) {
		if (cpumask_subset(xcpus, isolated_cpus))
			return;
		cpumask_or(isolated_cpus, isolated_cpus, xcpus);
	} else {
		if (!cpumask_intersects(xcpus, isolated_cpus))
			return;
		cpumask_andnot(isolated_cpus, isolated_cpus, xcpus);
	}
	update_housekeeping = true;
}

/*
