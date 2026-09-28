// AUTO-DRAFT from torvalds/linux PR #b1fa457bddf009907b023bc0c09ba4eca3191a8e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define PRS_ISOLATED		2
/* …（同文件无关代码省略）… */
static inline struct cpumask *user_xcpus(struct cpuset *cs)
{
	return cpumask_empty(cs->exclusive_cpus) ? cs->cpus_allowed
						 : cs->exclusive_cpus;
}
/* …（同文件无关代码省略）… */
static bool isolated_cpus_can_update(struct cpumask *add_cpus,
				     struct cpumask *del_cpus)
{
	cpumask_var_t full_hk_cpus;
	int res = true;

	if (!housekeeping_enabled(HK_TYPE_KERNEL_NOISE))
		return true;

	if (del_cpus && cpumask_weight_and(del_cpus,
			housekeeping_cpumask(HK_TYPE_KERNEL_NOISE)))
		return true;

	if (!alloc_cpumask_var(&full_hk_cpus, GFP_KERNEL))
		return false;

	cpumask_and(full_hk_cpus, housekeeping_cpumask(HK_TYPE_KERNEL_NOISE),
		    housekeeping_cpumask(HK_TYPE_DOMAIN));
	cpumask_andnot(full_hk_cpus, full_hk_cpus, isolated_cpus);
	cpumask_and(full_hk_cpus, full_hk_cpus, cpu_active_mask);
	if (!cpumask_weight_andnot(full_hk_cpus, add_cpus))
		res = false;

	free_cpumask_var(full_hk_cpus);
	return res;
}
/* …（同文件无关代码省略）… */
static bool prstate_housekeeping_conflict(int prstate, struct cpumask *new_cpus)
{
	if (!housekeeping_enabled(HK_TYPE_DOMAIN_BOOT))
		return false;

	if ((prstate != PRS_ISOLATED) &&
	    !cpumask_subset(new_cpus, housekeeping_cpumask(HK_TYPE_DOMAIN_BOOT)))
		return true;

	return false;
}
/* …（同文件无关代码省略）… */
static int rm_siblings_excl_cpus(struct cpuset *parent, struct cpuset *cs,
					struct cpumask *excpus)
{
	struct cgroup_subsys_state *css;
	struct cpuset *sibling;
	int retval = 0;

	if (cpumask_empty(excpus))
		return 0;

	/*
	 * Remove exclusive CPUs from siblings
	 */
	rcu_read_lock();
	cpuset_for_each_child(sibling, css, parent) {
		struct cpumask *sibling_xcpus;

		if (sibling == cs)
			continue;

		/*
		 * If exclusive_cpus is defined, effective_xcpus will always
		 * be a subset. Otherwise, effective_xcpus will only be set
		 * in a valid partition root.
		 */
		sibling_xcpus = cpumask_empty(sibling->exclusive_cpus)
			      ? sibling->effective_xcpus
			      : sibling->exclusive_cpus;

		if (cpumask_intersects(excpus, sibling_xcpus)) {
			cpumask_andnot(excpus, excpus, sibling_xcpus);
			retval++;
		}
	}
	rcu_read_unlock();

	return retval;
}
/* …（同文件无关代码省略）… */
static int compute_excpus(struct cpuset *cs, struct cpumask *excpus)
{
	struct cpuset *parent = parent_cs(cs);

	cpumask_and(excpus, user_xcpus(cs), parent->effective_xcpus);

	if (!cpumask_empty(cs->exclusive_cpus))
		return 0;

	return rm_siblings_excl_cpus(parent, cs, excpus);
}
/* …（同文件无关代码省略）… */
	 * above it or remote partition root underneath it is not allowed.
	 */
	compute_excpus(cs, tmp->new_cpus);
	WARN_ON_ONCE(cpumask_intersects(tmp->new_cpus, subpartitions_cpus));
	if (!cpumask_intersects(tmp->new_cpus, cpu_active_mask) ||
	    cpumask_subset(top_cpuset.effective_cpus, tmp->new_cpus))
		return PERR_INVCPUS;
	if (((new_prs == PRS_ISOLATED) &&
	     !isolated_cpus_can_update(tmp->new_cpus, NULL)) ||
	    prstate_housekeeping_conflict(new_prs, tmp->new_cpus))
