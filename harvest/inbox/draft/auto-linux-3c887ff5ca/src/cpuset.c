// AUTO-DRAFT from torvalds/linux PR #4b15991b3be43e9581aca489ec30a351310bb49d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
#define PRS_MEMBER		0
#define PRS_ROOT		1
#define PRS_ISOLATED		2
#define PRS_INVALID_ROOT	-1
#define PRS_INVALID_ISOLATED	-2
/* …（同文件无关代码省略）… */
struct tmpmasks {
	cpumask_var_t addmask, delmask;	/* For partition root */
	cpumask_var_t new_cpus;		/* For update_cpumasks_hier() */
};
/* …（同文件无关代码省略）… */
static inline bool is_partition_valid(const struct cpuset *cs)
{
	return cs->partition_root_state > 0;
}
/* …（同文件无关代码省略）… */
static inline bool is_partition_invalid(const struct cpuset *cs)
{
	return cs->partition_root_state < 0;
}
/* …（同文件无关代码省略）… */
static inline bool cs_is_member(const struct cpuset *cs)
{
	return cs->partition_root_state == PRS_MEMBER;
}
/* …（同文件无关代码省略）… */
static inline void notify_partition_change(struct cpuset *cs, int old_prs)
{
	if (old_prs == cs->partition_root_state)
		return;
	cgroup_file_notify(&cs->partition_file);

	/* Reset prs_err if not invalid */
	if (is_partition_valid(cs))
		WRITE_ONCE(cs->prs_err, PERR_NONE);
}
/* …（同文件无关代码省略）… */
void lockdep_assert_cpuset_lock_held(void)
{
	lockdep_assert_held(&cpuset_mutex);
}
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
static inline bool partition_is_populated(struct cpuset *cs,
					  struct cpuset *excluded_child)
{
	struct cpuset *cp;
	struct cgroup_subsys_state *pos_css;

	/*
	 * We cannot call cs_is_populated(cs) directly, as
	 * nr_populated_domain_children may include populated
	 * csets from descendants that are partitions.
	 */
	if (cgroup_has_tasks(cs->css.cgroup))
		return true;

	rcu_read_lock();
	cpuset_for_each_descendant_pre(cp, pos_css, cs) {
		if (cp == cs)
			continue;

		if (cp == excluded_child || is_partition_valid(cp)) {
			pos_css = css_rightmost_descendant(pos_css);
			continue;
		}

		if (cpuset_is_populated(cp)) {
			rcu_read_unlock();
			return true;
		}
	}
	rcu_read_unlock();
	return false;
}
/* …（同文件无关代码省略）… */
static inline struct cpumask *user_xcpus(struct cpuset *cs)
{
	return cpumask_empty(cs->exclusive_cpus) ? cs->cpus_allowed
						 : cs->exclusive_cpus;
}
/* …（同文件无关代码省略）… */
static inline bool xcpus_empty(struct cpuset *cs)
{
	return cpumask_empty(cs->cpus_allowed) &&
	       cpumask_empty(cs->exclusive_cpus);
}
/* …（同文件无关代码省略）… */
static inline bool cpusets_are_exclusive(struct cpuset *cs1, struct cpuset *cs2)
{
	struct cpumask *xcpus1 = user_xcpus(cs1);
	struct cpumask *xcpus2 = user_xcpus(cs2);

	if (cpumask_intersects(xcpus1, xcpus2))
		return false;
	return true;
}
/* …（同文件无关代码省略）… */
void cpuset_update_tasks_cpumask(struct cpuset *cs, struct cpumask *new_cpus)
{
	struct css_task_iter it;
	struct task_struct *task;
	bool top_cs = cs == &top_cpuset;

	css_task_iter_start(&cs->css, 0, &it);
	while ((task = css_task_iter_next(&it))) {
		const struct cpumask *possible_mask = task_cpu_possible_mask(task);

		if (top_cs) {
			/*
			 * PF_KTHREAD tasks are handled by housekeeping.
			 * PF_NO_SETAFFINITY tasks are ignored.
			 */
			if (task->flags & (PF_KTHREAD | PF_NO_SETAFFINITY))
				continue;
			cpumask_andnot(new_cpus, possible_mask, subpartitions_cpus);
		} else {
			cpumask_and(new_cpus, possible_mask, cs->effective_cpus);
		}
		set_cpus_allowed_ptr(task, new_cpus);
	}
	css_task_iter_end(&it);
}
/* …（同文件无关代码省略）… */
static void update_partition_sd_lb(struct cpuset *cs, int old_prs)
{
	int new_prs = cs->partition_root_state;
	bool rebuild_domains = (new_prs > 0) || (old_prs > 0);
	bool new_lb;

	/*
	 * If cs is not a valid partition root, the load balance state
	 * will follow its parent.
	 */
	if (new_prs > 0) {
		new_lb = (new_prs != PRS_ISOLATED);
	} else {
		new_lb = is_sched_load_balance(parent_cs(cs));
	}
	if (new_lb != !!is_sched_load_balance(cs)) {
		rebuild_domains = true;
		assign_bit(CS_SCHED_LOAD_BALANCE, &cs->flags, new_lb);
	}

	if (rebuild_domains)
		cpuset_force_rebuild();
}
/* …（同文件无关代码省略）… */
static bool tasks_nocpu_error(struct cpuset *parent, struct cpuset *cs,
			      struct cpumask *xcpus)
{
	/*
	 * A populated partition (cs or parent) can't have empty effective_cpus
	 */
	return (cpumask_subset(parent->effective_cpus, xcpus) &&
		partition_is_populated(parent, cs)) ||
	       (!cpumask_intersects(xcpus, cpu_active_mask) &&
		partition_is_populated(cs, NULL));
}
/* …（同文件无关代码省略）… */
static void reset_partition_data(struct cpuset *cs)
{
	struct cpuset *parent = parent_cs(cs);

	if (!cpuset_v2())
		return;

	lockdep_assert_held(&callback_lock);

	if (cpumask_empty(cs->exclusive_cpus))
		cpumask_clear(cs->effective_xcpus);

	if (!cpumask_and(cs->effective_cpus, parent->effective_cpus, cs->cpus_allowed))
		cpumask_copy(cs->effective_cpus, parent->effective_cpus);
}
/* …（同文件无关代码省略）… */
static inline bool is_remote_partition(struct cpuset *cs)
{
	return cs->remote_partition;
}
/* …（同文件无关代码省略）… */
}

/*
 * isolated_cpus_update - Update the isolated_cpus mask
 * @old_prs: old partition_root_state
 * @new_prs: new partition_root_state
 * @xcpus: exclusive CPUs with state change
 */
static void isolated_cpus_update(int old_prs, int new_prs, struct cpumask *xcpus)
{
	bool updated = false;
	int cpu;

	WARN_ON_ONCE(old_prs == new_prs);
	lockdep_assert_held(&callback_lock);
	lockdep_assert_held(&cpuset_mutex);
	for_each_cpu(cpu, xcpus)
/* …（同文件无关代码省略）… */
		update_housekeeping = true;
}

/*
 * isolated_cpus_release - Remove the relevant CPUs in @xcpus from isolated_cpus
 * @cs: the current cpuset
 * @xcpus: the set of effective xcpus to be considered for removal
 *
 * This function is called when an isolated partition is invalidated or when
 * some of its effective xcpus are to be removed from isolated_cpus due to
 * changes in parent cpuset. The given @xcpus must be modifiable as the function
 * will update its value to determine the right set of CPUs to be removed.
 */
static void isolated_cpus_release(struct cpuset *cs, struct cpumask *xcpus)
{
	struct cpuset *ancestor = parent_cs(cs);
	struct cpuset *iancestor = NULL;

	/*
	 * Search the furthest isolated partition root ancestor available
	 * until top_cpuset or a remote partition root is hit.
	 */
	for (; ancestor; ancestor = parent_cs(ancestor)) {
		if (ancestor->partition_root_state == PRS_ISOLATED)
			iancestor = ancestor;
		if (is_remote_partition(ancestor))
			break;
	}

	/*
	 * If a furthest isolated partition acnestor is found, use it to
	 * determine which of the CPUs in @xcpus should be removed from
	 * isolated_cpus. Otherwise, all of them will be removed.
	 */
	if (iancestor && !cpumask_andnot(xcpus, xcpus, iancestor->effective_xcpus))
		return; /* No CPU should be removed from isolated_cpus */

	spin_lock_irq(&callback_lock);
	isolated_cpus_update(PRS_ISOLATED, PRS_ROOT, xcpus);
	spin_unlock_irq(&callback_lock);
}

/*
 * partition_xcpus_add - Add new exclusive CPUs to partition
 * @new_prs: new partition_root_state
/* …（同文件无关代码省略）… */
static void partition_xcpus_add(int new_prs, struct cpuset *parent,
				struct cpumask *xcpus)
{
	WARN_ON_ONCE(new_prs < 0);
	lockdep_assert_held(&callback_lock);
	if (!parent)
		parent = &top_cpuset;


	if (parent == &top_cpuset)
		cpumask_or(subpartitions_cpus, subpartitions_cpus, xcpus);

	if (new_prs != parent->partition_root_state)
		isolated_cpus_update(parent->partition_root_state, new_prs,
				     xcpus);

	cpumask_andnot(parent->effective_cpus, parent->effective_cpus, xcpus);
}
/* …（同文件无关代码省略）… */
static void partition_xcpus_del(int old_prs, struct cpuset *parent,
				struct cpumask *xcpus)
{
	WARN_ON_ONCE(old_prs < 0);
	lockdep_assert_held(&callback_lock);
	if (!parent)
		parent = &top_cpuset;

	if (parent == &top_cpuset)
		cpumask_andnot(subpartitions_cpus, subpartitions_cpus, xcpus);

	if (old_prs != parent->partition_root_state)
		isolated_cpus_update(old_prs, parent->partition_root_state,
				     xcpus);

	cpumask_or(parent->effective_cpus, parent->effective_cpus, xcpus);
	cpumask_and(parent->effective_cpus, parent->effective_cpus, cpu_active_mask);
}
/* …（同文件无关代码省略）… */
static bool prstate_housekeeping_conflict(int new_prs, int parent_prs,
					  struct cpumask *new_cpus,
					  struct cpumask *del_cpus)
{
	cpumask_var_t full_hk_cpus;
	int res;

	WARN_ON_ONCE(parent_prs <= 0);
	if (!new_prs)
		return false;

	/*
	 * For a currently invalid partition root state, do the check assuming
	 * that it may be converted back to valid.
	 *
	 * There are code paths where an invalid partition may be reverted back
	 * to a valid partition but we still need to run the housekeeping check
	 * before the conversion happens to make sure the switch is valid. We
	 * can either do the negative to positive PRS switch individually in all
	 * these code paths or do it here which I think will make the code a bit
	 * easier to read.
	 */
	if (new_prs < 0)
		new_prs = -new_prs;

	if (housekeeping_enabled(HK_TYPE_DOMAIN_BOOT) && (new_prs == PRS_ROOT) &&
	    !cpumask_subset(new_cpus, housekeeping_cpumask(HK_TYPE_DOMAIN_BOOT)))
		return true;

	if (del_cpus && cpumask_empty(del_cpus))
		del_cpus = NULL;

	/*
	 * Exit 2nd check early and return false if
	 * 1) there is no boot time isolated nohz_full CPU
	 * 2) parent and child has the same partition state
	 * 3) new_prs is PRS_ROOT, parent_prs is PRS_ISOLATED and no CPU will
	 *    be returned back to the parent.
	 */
	if (!housekeeping_enabled(HK_TYPE_KERNEL_NOISE) ||
	    (new_prs == parent_prs) || (!del_cpus && (new_prs == PRS_ROOT)))
		return false;

	/*
	 * Now make sure that @new_cpus contains new CPUs to be isolated and
	 * @del_cpus contains isolated CPUs to be un-isolated.
	 * With the guards above, @new_cpus can't be NULL after the swap.
	 */
	if (parent_prs == PRS_ISOLATED)
		swap(new_cpus, del_cpus);

	if (!alloc_cpumask_var(&full_hk_cpus, GFP_KERNEL))
		return true;

	/*
	 * Check if del_cpus may contain a CPU that can become an active
	 * housekeeping CPU again.
	 */
	if (del_cpus) {
		cpumask_and(full_hk_cpus, housekeeping_cpumask(HK_TYPE_KERNEL_NOISE),
			    housekeeping_cpumask(HK_TYPE_DOMAIN_BOOT));
		if (cpumask_first_and_and(del_cpus, full_hk_cpus,
					  cpu_active_mask) < nr_cpu_ids) {
			res = false;
			goto out;
		}
	}

	cpumask_and(full_hk_cpus, housekeeping_cpumask(HK_TYPE_KERNEL_NOISE),
		    housekeeping_cpumask(HK_TYPE_DOMAIN));
	cpumask_andnot(full_hk_cpus, full_hk_cpus, isolated_cpus);
	cpumask_and(full_hk_cpus, full_hk_cpus, cpu_active_mask);
	res = !cpumask_weight_andnot(full_hk_cpus, new_cpus);
out:
	free_cpumask_var(full_hk_cpus);
	return res;
}

/*
 * cpuset_update_sd_hk_unlock - Rebuild sched domains, update HK & unlock
 *
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
static void compute_partition_effective_cpumask(struct cpuset *cs,
						struct cpumask *new_ecpus)
{
	struct cgroup_subsys_state *css;
	struct cpuset *child;
	bool populated = partition_is_populated(cs, NULL);

	/*
	 * Check child partition roots to see if they should be invalidated
	 * when all the effective_cpus will be used up and cs is populated.
	 * It is better to sacrifice a child partition root than invalidating
	 * the whole subtree.
	 *
	 * Return early if the effective_cpus is already empty.
	 */
	compute_excpus(cs, new_ecpus);
	if (!cpumask_and(new_ecpus, new_ecpus, cpu_active_mask))
		return;

	rcu_read_lock();
	cpuset_for_each_child(child, css, cs) {
		if (!is_partition_valid(child))
			continue;

		/*
		 * There shouldn't be a remote partition underneath another
		 * partition root.
		 */
		WARN_ON_ONCE(is_remote_partition(child));
		if (populated && cpumask_subset(new_ecpus, child->effective_xcpus)) {
			int old_prs = child->partition_root_state;
			int parent_prs = cs->partition_root_state;

			/*
			 * Invalidate child partition and update isolated_cpus
			 * if necessary.
			 */
			spin_lock_irq(&callback_lock);
			WRITE_ONCE(child->prs_err, PERR_NOCPUS);
			if ((old_prs == PRS_ISOLATED) && (parent_prs == PRS_ROOT))
				isolated_cpus_update(PRS_ISOLATED, PRS_ROOT,
						     child->effective_xcpus);
			child->partition_root_state = -old_prs;
			spin_unlock_irq(&callback_lock);
			notify_partition_change(child, old_prs);
			continue;
		}
		cpumask_andnot(new_ecpus, new_ecpus,
			       child->effective_xcpus);
	}
	rcu_read_unlock();
}
/* …（同文件无关代码省略）… */
static void compute_effective_cpumask(struct cpumask *new_cpus,
				      struct cpuset *cs, struct cpuset *parent)
{
	bool has_cpus;

	if (is_remote_partition(cs) ||
	    (is_partition_valid(cs) && is_partition_valid(parent)))
		return compute_partition_effective_cpumask(cs, new_cpus);

	has_cpus = cpumask_and(new_cpus, cs->cpus_allowed, parent->effective_cpus);
	if (!has_cpus && is_in_v2_mode())
		cpumask_copy(new_cpus, parent->effective_cpus);
}
/* …（同文件无关代码省略）… */
static void remote_partition_disable(struct cpuset *cs, struct tmpmasks *tmp)
{
	WARN_ON_ONCE(!is_remote_partition(cs));
	/*
	 * When a CPU is offlined, top_cpuset may end up with no available CPUs,
	 * which should clear subpartitions_cpus. We should not emit a warning for this
	 * scenario: the hierarchy is updated from top to bottom, so subpartitions_cpus
	 * may already be cleared when disabling the partition.
	 */
	WARN_ON_ONCE(!cpumask_subset(cs->effective_xcpus, subpartitions_cpus) &&
		     !cpumask_empty(subpartitions_cpus));

	spin_lock_irq(&callback_lock);
	cs->remote_partition = false;
	partition_xcpus_del(cs->partition_root_state, NULL, cs->effective_xcpus);
	if (cs->prs_err)
		cs->partition_root_state = -cs->partition_root_state;
	else
		cs->partition_root_state = PRS_MEMBER;

	/* effective_xcpus may need to be changed */
	compute_excpus(cs, cs->effective_xcpus);
	reset_partition_data(cs);
	spin_unlock_irq(&callback_lock);
	cpuset_force_rebuild();

	/*
	 * Propagate changes in top_cpuset's effective_cpus down the hierarchy.
	 */
	cpuset_update_tasks_cpumask(&top_cpuset, tmp->new_cpus);
	update_sibling_cpumasks(&top_cpuset, NULL, tmp);
}
/* …（同文件无关代码省略）… */
static void remote_cpus_update(struct cpuset *cs, struct cpumask *xcpus,
			       struct cpumask *excpus, struct tmpmasks *tmp)
{
	bool adding, deleting;
	int prs = cs->partition_root_state;

	if (WARN_ON_ONCE(!is_remote_partition(cs)))
		return;

	WARN_ON_ONCE(!cpumask_subset(cs->effective_xcpus, subpartitions_cpus));

	if (cpumask_empty(excpus)) {
		WRITE_ONCE(cs->prs_err, PERR_CPUSEMPTY);
		goto invalidate;
	}

	adding   = cpumask_andnot(tmp->addmask, excpus, cs->effective_xcpus);
	deleting = cpumask_andnot(tmp->delmask, cs->effective_xcpus, excpus);

	/*
	 * Additions of remote CPUs is only allowed if those CPUs are
	 * not allocated to other partitions and there are effective_cpus
	 * left in the top cpuset.
	 */
	if (adding) {
		WARN_ON_ONCE(cpumask_intersects(tmp->addmask, subpartitions_cpus));
		if (!capable(CAP_SYS_ADMIN))
			WRITE_ONCE(cs->prs_err, PERR_ACCESS);
		else if (cpumask_intersects(tmp->addmask, subpartitions_cpus) ||
			 cpumask_subset(top_cpuset.effective_cpus, tmp->addmask))
			WRITE_ONCE(cs->prs_err, PERR_NOCPUS);
		else if (prstate_housekeeping_conflict(prs, PRS_ROOT,
						       excpus, tmp->delmask))
			WRITE_ONCE(cs->prs_err, PERR_HKEEPING);
		if (cs->prs_err)
			goto invalidate;
	}

	cs->remote_partition_updating = true;
	spin_lock_irq(&callback_lock);
	if (adding)
		partition_xcpus_add(prs, NULL, tmp->addmask);
	if (deleting)
		partition_xcpus_del(prs, NULL, tmp->delmask);
	/*
	 * Need to update effective_xcpus and exclusive_cpus now as
	 * update_sibling_cpumasks() below may iterate back to the same cs.
	 */
	cpumask_copy(cs->effective_xcpus, excpus);
	if (xcpus)
		cpumask_copy(cs->exclusive_cpus, xcpus);
	spin_unlock_irq(&callback_lock);
	if (adding || deleting)
		cpuset_force_rebuild();

	/*
	 * Propagate changes in top_cpuset's effective_cpus down the hierarchy.
	 */
	cpuset_update_tasks_cpumask(&top_cpuset, tmp->new_cpus);
	update_sibling_cpumasks(&top_cpuset, NULL, tmp);
	cs->remote_partition_updating = false;
	return;

invalidate:
	remote_partition_disable(cs, tmp);
}
/* …（同文件无关代码省略）… */
static int update_parent_effective_cpumask(struct cpuset *cs, int cmd,
					   struct cpumask *newmask,
					   struct tmpmasks *tmp)
{
	struct cpuset *parent = parent_cs(cs);
	int adding;	/* Adding cpus to parent's effective_cpus	*/
	int deleting;	/* Deleting cpus from parent's effective_cpus	*/
	int old_prs, new_prs;
	int part_error = PERR_NONE;	/* Partition error? */
	int parent_prs = parent->partition_root_state;
	bool nocpu;

	lockdep_assert_cpuset_lock_held();
	WARN_ON_ONCE(is_remote_partition(cs));	/* For local partition only */

	/*
	 * new_prs will only be changed for the partcmd_update and
	 * partcmd_invalidate commands.
	 */
	adding = deleting = false;
	old_prs = new_prs = cs->partition_root_state;

	if (cmd == partcmd_invalidate) {
		if (is_partition_invalid(cs))
			return 0;

		/*
		 * Make the current partition invalid.
		 */
		if (is_partition_valid(parent))
			adding = cpumask_and(tmp->addmask,
					     cs->effective_xcpus,
					     parent->effective_xcpus);
		if (old_prs > 0)
			new_prs = -old_prs;

		goto write_error;
	}

	/*
	 * The parent must be a partition root.
	 * The new cpumask, if present, or the current cpus_allowed must
	 * not be empty.
	 */
	if (!is_partition_valid(parent)) {
		return is_partition_invalid(parent)
		       ? PERR_INVPARENT : PERR_NOTPART;
	}
	if (!newmask && xcpus_empty(cs))
		return PERR_CPUSEMPTY;

	/*
	 * Except for partcmd_disable and non-NULL newmask, use delmask
	 * to compute the current effective xcpus value and use it for
	 * tasks_nocpu_error check.
	 */
	if ((cmd != partcmd_disable) && !newmask) {
		compute_excpus(cs, tmp->delmask);
		nocpu = tasks_nocpu_error(parent, cs, tmp->delmask);
	}

	if ((cmd == partcmd_enable) || (cmd == partcmd_enablei)) {
		new_prs = (cmd == partcmd_enable) ? PRS_ROOT : PRS_ISOLATED;

		/*
		 * Enabling partition root is not allowed if its
		 * effective_xcpus is empty.
		 */
		if (cpumask_empty(tmp->delmask))
			return PERR_INVCPUS;

		if (prstate_housekeeping_conflict(new_prs, parent_prs, tmp->delmask, NULL))
			return PERR_HKEEPING;

		if (nocpu)
			return PERR_NOCPUS;

		/*
		 * This function will only be called when all the preliminary
		 * checks have passed. At this point, the following condition
		 * should hold.
		 *
		 * (cs->effective_xcpus & cpu_active_mask) ⊆ parent->effective_cpus
		 *
		 * Warn if it is not the case.
		 */
		cpumask_and(tmp->new_cpus, tmp->delmask, cpu_active_mask);
		WARN_ON_ONCE(!cpumask_subset(tmp->new_cpus, parent->effective_cpus));
		deleting = true;
	} else if (cmd == partcmd_disable) {
		/*
		 * May need to add cpus back to parent's effective_cpus
		 * (and maybe removed from subpartitions_cpus/isolated_cpus)
		 * for valid partition root.
		 */
		if (is_partition_valid(cs)) {
			cpumask_copy(tmp->addmask, cs->effective_xcpus);
			adding = true;
		}
		new_prs = PRS_MEMBER;
	} else if (newmask) {
		/*
		 * Empty cpumask is not allowed
		 */
		if (cpumask_empty(newmask)) {
			part_error = PERR_CPUSEMPTY;
			goto write_error;
		}
		if ((parent_prs == PRS_ISOLATED) && (old_prs == -PRS_ROOT)) {
			part_error = PERR_ISOPARENT;
			goto write_error;
		}

		/* Check newmask whether cpus are available for parent/cs */
		nocpu = tasks_nocpu_error(parent, cs, newmask);

		/*
		 * partcmd_update with newmask:
		 *
		 * Compute add/delete mask to/from effective_cpus
		 *
		 * For valid partition:
		 *   addmask = effective_xcpus & ~newmask
		 *			      & parent->effective_xcpus
		 *   delmask = newmask & ~effective_xcpus
		 *		       & parent->effective_xcpus
		 *
		 * For invalid partition:
		 *   delmask = newmask & parent->effective_xcpus
		 *   The partition may become valid soon.
		 */
		if (is_partition_invalid(cs)) {
			adding = false;
			deleting = cpumask_and(tmp->delmask,
					newmask, parent->effective_xcpus);
		} else {
			cpumask_andnot(tmp->addmask, cs->effective_xcpus, newmask);
			adding = cpumask_and(tmp->addmask, tmp->addmask,
					     parent->effective_xcpus);

			cpumask_andnot(tmp->delmask, newmask, cs->effective_xcpus);
			deleting = cpumask_and(tmp->delmask, tmp->delmask,
					       parent->effective_xcpus);
		}

		/*
		 * The new CPUs to be removed from parent's effective CPUs
		 * must be present.
		 */
		if (deleting) {
			cpumask_and(tmp->new_cpus, tmp->delmask, cpu_active_mask);
			WARN_ON_ONCE(!cpumask_subset(tmp->new_cpus, parent->effective_cpus));
		}

		/*
		 * Make partition invalid if parent's effective_cpus could
		 * become empty and there are tasks in the parent.
		 */
		if (nocpu && (!adding ||
		    !cpumask_intersects(tmp->addmask, cpu_active_mask))) {
			part_error = PERR_NOCPUS;
			deleting = false;
			adding = cpumask_and(tmp->addmask,
					     cs->effective_xcpus, parent->effective_xcpus);
		}
	} else {
		/*
		 * partcmd_update w/o newmask
		 *
		 * delmask = newly computed effective xcpus value
		 *
		 * This can be called from:
		 * 1) update_cpumasks_hier()
		 * 2) cpuset_hotplug_update_tasks()
		 *
		 * Check to see if it can be transitioned from valid to
		 * invalid partition or vice versa.
		 *
		 * A partition error happens when parent has tasks and all
		 * its effective CPUs will have to be distributed out.
		 *
		 * For an isolated partition with a shrunk root partition parent
		 * we may need to return some or all of its isolated CPUs.
		 */
		nocpu |= cpumask_empty(tmp->delmask);
		if ((parent_prs == PRS_ROOT) && (old_prs == PRS_ISOLATED)) {
			cpumask_clear(tmp->addmask);
			if (nocpu)
				cpumask_copy(tmp->addmask, cs->effective_xcpus);
			else if (!cpumask_subset(cs->effective_xcpus, parent->effective_xcpus))
				cpumask_andnot(tmp->addmask, cs->effective_xcpus,
							     parent->effective_xcpus);
			if (!cpumask_empty(tmp->addmask))
				isolated_cpus_release(cs, tmp->addmask);
		}
		if (nocpu) {
			part_error = PERR_NOCPUS;
			if (is_partition_valid(cs))
				adding = cpumask_and(tmp->addmask,
						     cs->effective_xcpus,
						     parent->effective_xcpus);
		} else if ((parent_prs == PRS_ISOLATED) &&
			   ((old_prs == PRS_ROOT) || (old_prs == -PRS_ROOT))) {
			part_error = PERR_ISOPARENT;
			if (old_prs == PRS_ROOT) {
				cpumask_copy(tmp->addmask, cs->effective_xcpus);
				adding = true;
			}
		} else if (is_partition_invalid(cs)) {
			struct cgroup_subsys_state *css;
			struct cpuset *child;
			bool exclusive = true;

			/*
			 * Convert invalid partition to valid has to pass the
			 * cpu exclusivity test when cpuset.cpus.exclusive is
			 * empty as well as the housekeeping check.
			 */
			if (prstate_housekeeping_conflict(cs->partition_root_state,
							  parent_prs, tmp->delmask, NULL)) {
				part_error = PERR_HKEEPING;
				goto write_error;
			}
			if (cpumask_empty(cs->exclusive_cpus)) {
				rcu_read_lock();
				cpuset_for_each_child(child, css, parent) {
					if (child == cs)
						continue;
					if (!cpusets_are_exclusive(cs, child)) {
						exclusive = false;
						break;
					}
				}
				rcu_read_unlock();
			}
			if (exclusive)
				deleting = !cpumask_empty(tmp->delmask);
			else
				part_error = PERR_NOTEXCL;
		}
		/*
		 * When both cs and parent remain valid, parent's
		 * compute_partition_effective_cpumask() call would have removed
		 * cs's effective_xcpus from the parent's effective_cpus. So
		 * there is no need to adjust parent's effective_cpus here.
		 */
	}

write_error:
	if (part_error)
		WRITE_ONCE(cs->prs_err, part_error);

	if (cmd == partcmd_update) {
		/*
		 * Check for possible transition between valid and invalid
		 * partition root.
		 */
		switch (cs->partition_root_state) {
		case PRS_ROOT:
		case PRS_ISOLATED:
			if (part_error)
				new_prs = -old_prs;
			break;
		case PRS_INVALID_ROOT:
		case PRS_INVALID_ISOLATED:
			if (!part_error)
				new_prs = -old_prs;
			break;
		}
	}

	if (!adding && !deleting && (new_prs == old_prs))
		return 0;

	/*
	 * Change the parent's effective_cpus & effective_xcpus (top cpuset
	 * only).
	 *
	 * Newly added CPUs will be removed from effective_cpus and
	 * newly deleted ones will be added back to effective_cpus.
	 */
	spin_lock_irq(&callback_lock);
	if (old_prs != new_prs)
		cs->partition_root_state = new_prs;

	/*
	 * Adding to parent's effective_cpus means deletion CPUs from cs
	 * and vice versa.
	 */
	if (adding)
		partition_xcpus_del(old_prs, parent, tmp->addmask);
	if (deleting)
		partition_xcpus_add(new_prs, parent, tmp->delmask);

	spin_unlock_irq(&callback_lock);

	if (adding || deleting) {
		cpuset_update_tasks_cpumask(parent, tmp->addmask);
		update_sibling_cpumasks(parent, cs, tmp);
	}

	/*
	 * For partcmd_update without newmask, it is being called from
	 * cpuset_handle_hotplug(). Update the load balance flag and
	 * scheduling domain accordingly.
	 */
	if ((cmd == partcmd_update) && !newmask)
		update_partition_sd_lb(cs, old_prs);

	notify_partition_change(cs, old_prs);
	return 0;
}
/* …（同文件无关代码省略）… */
static void update_cpumasks_hier(struct cpuset *cs, struct tmpmasks *tmp,
				 bool force)
{
	struct cpuset *cp;
	struct cgroup_subsys_state *pos_css;
	int old_prs, new_prs;

	rcu_read_lock();
	cpuset_for_each_descendant_pre(cp, pos_css, cs) {
		struct cpuset *parent = parent_cs(cp);
		bool remote = is_remote_partition(cp);
		bool update_parent = false;

		old_prs = new_prs = cp->partition_root_state;

		/*
		 * For a child remote partition with remote_partition_updating
		 * set, we can skip the whole subtree as the this function will
		 * be called eventually. We can also skip the subtree if the
		 * effective_xcpus isn't going to be changed. Otherwise, we
		 * will have to call remote_cpus_update().
		 *
		 * remote_cpus_update() will reuse tmp->new_cpus only after
		 * its value is being processed.
		 */
		if (remote && (cp != cs)) {
			compute_excpus(cp, tmp->new_cpus);
			if (cp->remote_partition_updating ||
			    cpumask_equal(cp->effective_xcpus, tmp->new_cpus)) {
				pos_css = css_rightmost_descendant(pos_css);
				continue;
			}
			rcu_read_unlock();
			remote_cpus_update(cp, NULL, tmp->new_cpus, tmp);
			rcu_read_lock();

			/* Remote partition may be invalidated */
			new_prs = cp->partition_root_state;
			remote = (new_prs == old_prs);
		}

		compute_effective_cpumask(tmp->new_cpus, cp, parent);

		if (remote)
			goto get_css;	/* Ready to update cpuset data */

		/*
		 * A partition with no effective_cpus is allowed as long as
		 * there is no task associated with it. Call
		 * update_parent_effective_cpumask() to check it.
		 */
		if (is_partition_valid(cp) && cpumask_empty(tmp->new_cpus)) {
			update_parent = true;
			goto update_parent_effective;
		}

		/*
		 * Skip the whole subtree if
		 * 1) the cpumask remains the same,
		 * 2) has no partition root state,
		 * 3) force flag not set, and
		 * 4) for v2 load balance state same as its parent.
		 */
		if (!cp->partition_root_state && !force &&
		    cpumask_equal(tmp->new_cpus, cp->effective_cpus) &&
		    (!cpuset_v2() ||
		    (is_sched_load_balance(parent) == is_sched_load_balance(cp)))) {
			pos_css = css_rightmost_descendant(pos_css);
			continue;
		}

update_parent_effective:
		/*
		 * update_parent_effective_cpumask() should have been called
		 * for cs already in update_cpumask(). We should also call
		 * cpuset_update_tasks_cpumask() again for tasks in the parent
		 * cpuset if the parent's effective_cpus changes.
		 */
		if ((cp != cs) && old_prs) {
			int parent_prs = parent->partition_root_state;

			switch (parent_prs) {
			case PRS_ROOT:
			case PRS_ISOLATED:
				update_parent = true;
				break;

			default:
				/*
				 * When parent is not a valid partition root,
				 * child partition roots become invalid too.
				 *
				 * For a to be invalidated isolated partition,
				 * call return_isolated_cpus() to remove the
				 * relevant CPUs from isolated_cpus.
				 */
				if (is_partition_valid(cp))
					new_prs = -old_prs;

				if (old_prs == PRS_ISOLATED) {
					cpumask_copy(tmp->delmask, cp->effective_xcpus);
					isolated_cpus_release(cp, tmp->delmask);
				}
				WRITE_ONCE(cp->prs_err,
					   is_partition_invalid(parent)
					   ? PERR_INVPARENT : PERR_NOTPART);
				break;
			}
		}
get_css:
		if (!css_tryget_online(&cp->css))
			continue;
		rcu_read_unlock();

		if (update_parent) {
			update_parent_effective_cpumask(cp, partcmd_update, NULL, tmp);
			/*
			 * Capture current partition_root_state as it may have
			 * changed. Recompute effective cpumask if changed.
			 */
			new_prs = cp->partition_root_state;
			if (new_prs != old_prs) {
				compute_effective_cpumask(tmp->new_cpus, cp, parent);
			}
		}

		spin_lock_irq(&callback_lock);
		cpumask_copy(cp->effective_cpus, tmp->new_cpus);
		cp->partition_root_state = new_prs;
		/*
		 * Need to compute effective_xcpus if either exclusive_cpus
		 * is non-empty or it is a valid partition root.
		 */
		if ((new_prs > 0) || !cpumask_empty(cp->exclusive_cpus))
			compute_excpus(cp, cp->effective_xcpus);
		if (new_prs <= 0)
			reset_partition_data(cp);
		spin_unlock_irq(&callback_lock);

		notify_partition_change(cp, old_prs);

		WARN_ON(!is_in_v2_mode() &&
			!cpumask_equal(cp->cpus_allowed, cp->effective_cpus));

		cpuset_update_tasks_cpumask(cp, tmp->new_cpus);

		/*
		 * On default hierarchy, inherit the CS_SCHED_LOAD_BALANCE
		 * from parent if current cpuset isn't a valid partition root
		 * and their load balance states differ.
		 */
		if (cpuset_v2() && !is_partition_valid(cp) &&
		    (is_sched_load_balance(parent) != is_sched_load_balance(cp))) {
			assign_bit(CS_SCHED_LOAD_BALANCE, &cp->flags,
				   is_sched_load_balance(parent));
		}

		/*
		 * On legacy hierarchy, if the effective cpumask of any non-
		 * empty cpuset is changed, we need to rebuild sched domains.
		 * On default hierarchy, the cpuset needs to be a partition
		 * root as well.
		 */
		if (!cpumask_empty(cp->cpus_allowed) &&
		    is_sched_load_balance(cp) &&
		   (!cpuset_v2() || is_partition_valid(cp)))
			cpuset_force_rebuild();

		rcu_read_lock();
		css_put(&cp->css);
	}
	rcu_read_unlock();
}
/* …（同文件无关代码省略）… */
static void update_sibling_cpumasks(struct cpuset *parent, struct cpuset *cs,
				    struct tmpmasks *tmp)
{
	struct cpuset *sibling;
	struct cgroup_subsys_state *pos_css;

	lockdep_assert_cpuset_lock_held();

	/*
	 * Check all its siblings and call update_cpumasks_hier()
	 * if their effective_cpus will need to be changed.
	 *
	 * It is possible a change in parent's effective_cpus
	 * due to a change in a child partition's effective_xcpus will impact
	 * its siblings even if they do not inherit parent's effective_cpus
	 * directly. It should not impact valid partition.
	 *
	 * The update_cpumasks_hier() function may sleep. So we have to
	 * release the RCU read lock before calling it.
	 */
	rcu_read_lock();
	cpuset_for_each_child(sibling, pos_css, parent) {
		if (sibling == cs || is_partition_valid(sibling))
			continue;

		compute_effective_cpumask(tmp->new_cpus, sibling,
					  parent);
		if (cpumask_equal(tmp->new_cpus, sibling->effective_cpus))
			continue;

		if (!css_tryget_online(&sibling->css))
			continue;

		rcu_read_unlock();
		update_cpumasks_hier(sibling, tmp, false);
		rcu_read_lock();
		css_put(&sibling->css);
	}
	rcu_read_unlock();
}
/* …（同文件无关代码省略）… */
	return 0;
}

/**
 * validate_partition - Validate a cpuset partition configuration
 * @cs: The cpuset to validate
 * @trialcs: The trial cpuset containing proposed configuration changes
 *
 * If any validation check fails, the appropriate error code is set in the
 * cpuset's prs_err field.
 *
 * Return: PRS error code (0 if valid, non-zero error code if invalid)
 */
static enum prs_errcode validate_partition(struct cpuset *cs, struct cpuset *trialcs)
{
	struct cpuset *parent = parent_cs(cs);

	if (cs_is_member(trialcs))
		return PERR_NONE;

	if (cpumask_empty(trialcs->effective_xcpus))
		return PERR_INVCPUS;

	if (tasks_nocpu_error(parent, cs, trialcs->effective_xcpus))
		return PERR_NOCPUS;

	return PERR_NONE;
}

/**
 * partition_cpus_change - Handle partition state changes due to CPU mask updates
 * @cs: The target cpuset being modified
/* …（同文件无关代码省略）… */
				  struct tmpmasks *tmp)
{
	enum prs_errcode prs_err;
	int parent_prs = is_remote_partition(cs)
		       ? PRS_ROOT : parent_cs(cs)->partition_root_state;

	if (cs_is_member(cs))
		return;

	prs_err = validate_partition(cs, trialcs);
	if (!prs_err && (parent_prs > 0) &&
	    !cpumask_empty(trialcs->effective_xcpus)) {
		int prs = cs->partition_root_state;

		/*
		 * Check for housekeeping conflict by passing the whole
		 * effective_xcpus as well as those CPUs that are to be
		 * removed from cs as delmask.
		 */
		if (prs > 0)
			cpumask_andnot(tmp->delmask, cs->effective_xcpus,
				       trialcs->effective_xcpus);
		else
			cpumask_clear(tmp->delmask);

		if (prstate_housekeeping_conflict(prs, parent_prs,
						  trialcs->effective_xcpus, tmp->delmask))
			prs_err = PERR_HKEEPING;
	}

	if (prs_err) {
		WRITE_ONCE(cs->prs_err, prs_err);
		trialcs->prs_err = prs_err;
/* …（同文件无关代码省略）… */
	spin_lock_irq(&callback_lock);
	cpumask_copy(cs->cpus_allowed, trialcs->cpus_allowed);
	cpumask_copy(cs->effective_xcpus, trialcs->effective_xcpus);
	if ((old_prs > 0) && !is_partition_valid(cs))
		reset_partition_data(cs);
	spin_unlock_irq(&callback_lock);

	/* effective_cpus/effective_xcpus will be updated here */
/* …（同文件无关代码省略）… */
	spin_lock_irq(&callback_lock);
	cpumask_copy(cs->exclusive_cpus, trialcs->exclusive_cpus);
	cpumask_copy(cs->effective_xcpus, trialcs->effective_xcpus);
	if ((old_prs > 0) && !is_partition_valid(cs))
		reset_partition_data(cs);
	spin_unlock_irq(&callback_lock);

	/*
/* …（同文件无关代码省略）… */
						  cs->effective_xcpus, NULL))
			err = PERR_HKEEPING;
		else if (parent_prs == PRS_ISOLATED)
			err = PERR_ISOPARENT;

		if (!err)
			isolcpus_updated = true;
		else
			disable_partition = true;
	} else {
		/*
		 * Switching back to member is always allowed even if it
/* …（同文件无关代码省略）… */
	if (!is_partition_valid(cs))
		reset_partition_data(cs);
	else if (isolcpus_updated)
		isolated_cpus_update(old_prs, new_prs, cs->effective_xcpus);
	spin_unlock_irq(&callback_lock);

	/* Force update if partition is disabled & update effective_xcpus */
/* …（同文件无关代码省略）… */
	rcu_read_unlock();

	if (cs == oldcs) {
		if (cs == &top_cpuset)
			return;

		set_cpus_allowed_ptr(task, current->cpus_ptr);
		task->mems_allowed = current->mems_allowed;
		return;
	}

/* …（同文件无关代码省略）… */
void cpuset_force_rebuild(void)
{
	force_sd_rebuild = true;
}
/* …（同文件无关代码省略）… */
		partcmd = partcmd_invalidate;
	/*
	 * On the other hand, an invalid partition root may be transitioned
	 * back to a regular one with a non-empty effective xcpus.
	 */
	else if (is_partition_valid(parent) && is_partition_invalid(cs) &&
		 !cpumask_empty(cs->effective_xcpus))
		partcmd = partcmd_update;

	if (partcmd >= 0) {
