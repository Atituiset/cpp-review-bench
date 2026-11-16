// AUTO-DRAFT from torvalds/linux PR #4b15991b3be43e9581aca489ec30a351310bb49d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
struct freezer {
	struct cgroup_subsys_state	css;
	unsigned int			state;
};
/* …（同文件无关代码省略）… */
static inline struct freezer *css_freezer(struct cgroup_subsys_state *css)
{
	return css ? container_of(css, struct freezer, css) : NULL;
}
/* …（同文件无关代码省略）… */
static struct freezer *parent_freezer(struct freezer *freezer)
{
	return css_freezer(freezer->css.parent);
}
/* …（同文件无关代码省略）… */
static struct cgroup_subsys_state *
freezer_css_alloc(struct cgroup_subsys_state *parent_css)
{
	struct freezer *freezer;

	freezer = kzalloc_obj(struct freezer);
	if (!freezer)
		return ERR_PTR(-ENOMEM);

	return &freezer->css;
}
/* …（同文件无关代码省略）… */
static int freezer_css_online(struct cgroup_subsys_state *css)
{
	struct freezer *freezer = css_freezer(css);
	struct freezer *parent = parent_freezer(freezer);

	cpus_read_lock();
	mutex_lock(&freezer_mutex);

	freezer->state |= CGROUP_FREEZER_ONLINE;

	if (parent && (parent->state & CGROUP_FREEZING)) {
		freezer->state |= CGROUP_FREEZING_PARENT | CGROUP_FROZEN;
		static_branch_inc_cpuslocked(&freezer_active);
	}

	mutex_unlock(&freezer_mutex);
	cpus_read_unlock();
	return 0;
}
/* …（同文件无关代码省略）… */
static void freezer_css_offline(struct cgroup_subsys_state *css)
{
	struct freezer *freezer = css_freezer(css);

	cpus_read_lock();
	mutex_lock(&freezer_mutex);

	if (freezer->state & CGROUP_FREEZING)
		static_branch_dec_cpuslocked(&freezer_active);

	freezer->state = 0;

	mutex_unlock(&freezer_mutex);
	cpus_read_unlock();
}
/* …（同文件无关代码省略）… */
static void freezer_css_free(struct cgroup_subsys_state *css)
{
	kfree(css_freezer(css));
}
/* …（同文件无关代码省略）… */
static void freezer_attach(struct cgroup_taskset *tset)
{
	struct task_struct *task;
	struct cgroup_subsys_state *new_css;

	mutex_lock(&freezer_mutex);

	/*
	 * Make the new tasks conform to the current state of @new_css.
	 * For simplicity, when migrating any task to a FROZEN cgroup, we
	 * revert it to FREEZING and let update_if_frozen() determine the
	 * correct state later.
	 *
	 * Tasks in @tset are on @new_css but may not conform to its
	 * current state before executing the following - !frozen tasks may
	 * be visible in a FROZEN cgroup and frozen tasks in a THAWED one.
	 */
	cgroup_taskset_for_each(task, new_css, tset) {
		struct freezer *freezer = css_freezer(new_css);

		if (!(freezer->state & CGROUP_FREEZING)) {
			__thaw_task(task);
		} else {
			/* clear FROZEN and propagate upwards */
			while (freezer && (freezer->state & CGROUP_FROZEN)) {
				freezer->state &= ~CGROUP_FROZEN;
				freezer = parent_freezer(freezer);
			}
			freeze_task(task);
		}
	}

	mutex_unlock(&freezer_mutex);
}
/* …（同文件无关代码省略）… */
	rcu_read_unlock();

	/* are all tasks frozen? */
	css_task_iter_start(css, 0, &it);

	while ((task = css_task_iter_next(&it))) {
		if (freezing(task) && !frozen(task))
/* …（同文件无关代码省略）… */
	struct css_task_iter it;
	struct task_struct *task;

	css_task_iter_start(&freezer->css, 0, &it);
	while ((task = css_task_iter_next(&it)))
		freeze_task(task);
	css_task_iter_end(&it);
/* …（同文件无关代码省略）… */
	struct css_task_iter it;
	struct task_struct *task;

	css_task_iter_start(&freezer->css, 0, &it);
	while ((task = css_task_iter_next(&it)))
		__thaw_task(task);
	css_task_iter_end(&it);
/* …（同文件无关代码省略）… */
	 * CGROUP_FREEZING_PARENT.
	 */
	mutex_lock(&freezer_mutex);
	rcu_read_lock();
	css_for_each_descendant_pre(pos, &freezer->css) {
		struct freezer *pos_f = css_freezer(pos);
/* …（同文件无关代码省略）… */
		css_put(pos);
	}
	rcu_read_unlock();
	mutex_unlock(&freezer_mutex);
	cpus_read_unlock();
}

static ssize_t freezer_write(struct kernfs_open_file *of,
			     char *buf, size_t nbytes, loff_t off)
{
/* …（同文件无关代码省略）… */
struct cgroup_subsys freezer_cgrp_subsys = {
	.css_alloc	= freezer_css_alloc,
	.css_online	= freezer_css_online,
	.css_offline	= freezer_css_offline,
	.css_free	= freezer_css_free,
	.attach		= freezer_attach,
