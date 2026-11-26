// AUTO-DRAFT from torvalds/linux PR #f441d24349a3fb7f0b0f8f76f05686e448fd0b10
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
static inline bool __sched_core_less(const struct task_struct *a,
				     const struct task_struct *b)
{
	if (a->core_cookie < b->core_cookie)
		return true;

	if (a->core_cookie > b->core_cookie)
		return false;

	/* flip prio, so high prio is leftmost */
	if (prio_less(b, a, !!task_rq(a)->core->core_forceidle_count))
		return true;

	return false;
}
/* …（同文件无关代码省略）… */
#define __node_2_sc(node) rb_entry((node), struct task_struct, core_node)
/* …（同文件无关代码省略）… */
static inline bool rb_sched_core_less(struct rb_node *a, const struct rb_node *b)
{
	return __sched_core_less(__node_2_sc(a), __node_2_sc(b));
}
/* …（同文件无关代码省略）… */
void sched_core_enqueue(struct rq *rq, struct task_struct *p)
{
	if (p->se.sched_delayed)
		return;

	rq->core->core_task_seq++;

	if (!p->core_cookie)
		return;

	rb_add(&p->core_node, &rq->core_tree, rb_sched_core_less);
}
/* …（同文件无关代码省略）… */
static void update_rq_clock_task(struct rq *rq, s64 delta)
{
/*
 * In theory, the compile should just see 0 here, and optimize out the call
 * to sched_rt_avg_update. But I don't trust it...
 */
	s64 __maybe_unused steal = 0, irq_delta = 0;

#ifdef CONFIG_IRQ_TIME_ACCOUNTING
	if (irqtime_enabled()) {
		irq_delta = irq_time_read(cpu_of(rq)) - rq->prev_irq_time;

		/*
		 * Since irq_time is only updated on {soft,}irq_exit, we might run into
		 * this case when a previous update_rq_clock() happened inside a
		 * {soft,}IRQ region.
		 *
		 * When this happens, we stop ->clock_task and only update the
		 * prev_irq_time stamp to account for the part that fit, so that a next
		 * update will consume the rest. This ensures ->clock_task is
		 * monotonic.
		 *
		 * It does however cause some slight miss-attribution of {soft,}IRQ
		 * time, a more accurate solution would be to update the irq_time using
		 * the current rq->clock timestamp, except that would require using
		 * atomic ops.
		 */
		if (irq_delta > delta)
			irq_delta = delta;

		rq->prev_irq_time += irq_delta;
		delta -= irq_delta;
		delayacct_irq(rq->curr, irq_delta);
	}
#endif
#ifdef CONFIG_PARAVIRT_TIME_ACCOUNTING
	if (static_branch_unlikely(&paravirt_steal_rq_enabled)) {
		u64 prev_steal;

		steal = prev_steal = paravirt_steal_clock(cpu_of(rq));
		steal -= rq->prev_steal_time_rq;

		if (unlikely(steal > delta))
			steal = delta;

		rq->prev_steal_time_rq = prev_steal;
		delta -= steal;
	}
#endif

	rq->clock_task += delta;

#ifdef CONFIG_HAVE_SCHED_AVG_IRQ
	if ((irq_delta + steal) && sched_feat(NONTASK_CAPACITY))
		update_irq_load_avg(rq, irq_delta + steal);
#endif
	update_rq_clock_pelt(rq, delta);
}
/* …（同文件无关代码省略）… */
void update_rq_clock(struct rq *rq)
{
	s64 delta;
	u64 clock;

	lockdep_assert_rq_held(rq);

	if (rq->clock_update_flags & RQCF_ACT_SKIP)
		return;

	if (sched_feat(WARN_DOUBLE_CLOCK))
		WARN_ON_ONCE(rq->clock_update_flags & RQCF_UPDATED);
	rq->clock_update_flags |= RQCF_UPDATED;

	clock = sched_clock_cpu(cpu_of(rq));
	scx_rq_clock_update(rq, clock);

	delta = clock - rq->clock;
	if (delta < 0)
		return;
	rq->clock += delta;

	update_rq_clock_task(rq, delta);
}
/* …（同文件无关代码省略）… */
#define fetch_or(ptr, mask)						\
	({								\
		typeof(ptr) _ptr = (ptr);				\
		typeof(mask) _mask = (mask);				\
		typeof(*_ptr) _val = *_ptr;				\
									\
		do {							\
		} while (!try_cmpxchg(_ptr, &_val, _val | _mask));	\
	_val;								\
})
/* …（同文件无关代码省略）… */
static inline bool set_nr_and_not_polling(struct thread_info *ti, int tif)
{
	return !(fetch_or(&ti->flags, 1 << tif) & _TIF_POLLING_NRFLAG);
}
/* …（同文件无关代码省略）… */
static void __resched_curr(struct rq *rq, int tif)
{
	struct task_struct *curr = rq->curr;
	struct thread_info *cti = task_thread_info(curr);
	bool need_ipi;
	int cpu;

	lockdep_assert_rq_held(rq);

	/*
	 * Always immediately preempt the idle task; no point in delaying doing
	 * actual work.
	 */
	if (is_idle_task(curr) && tif == TIF_NEED_RESCHED_LAZY)
		tif = TIF_NEED_RESCHED;

	if (cti->flags & ((1 << tif) | _TIF_NEED_RESCHED))
		return;

	cpu = cpu_of(rq);

	if (cpu == smp_processor_id()) {
		set_ti_thread_flag(cti, tif);
		if (tif == TIF_NEED_RESCHED)
			set_preempt_need_resched();
		trace_sched_set_need_resched_tp(curr, cpu, tif);
		return;
	}

	need_ipi = set_nr_and_not_polling(cti, tif);
	trace_sched_set_need_resched_tp(curr, cpu, tif);
	if (need_ipi) {
		if (tif == TIF_NEED_RESCHED)
			smp_send_reschedule(cpu);
	} else {
		trace_sched_wake_idle_without_ipi(cpu);
	}
}
/* …（同文件无关代码省略）… */
void resched_curr(struct rq *rq)
{
	__resched_curr(rq, TIF_NEED_RESCHED);
}
/* …（同文件无关代码省略）… */
static inline void uclamp_idle_reset(struct rq *rq, enum uclamp_id clamp_id,
				     unsigned int clamp_value)
{
	/* Reset max-clamp retention only on idle exit */
	if (!(rq->uclamp_flags & UCLAMP_FLAG_IDLE))
		return;

	uclamp_rq_set(rq, clamp_id, clamp_value);
}
/* …（同文件无关代码省略）… */
static inline struct uclamp_se
uclamp_eff_get(struct task_struct *p, enum uclamp_id clamp_id)
{
	struct uclamp_se uc_req = uclamp_tg_restrict(p, clamp_id);
	struct uclamp_se uc_max = uclamp_default[clamp_id];

	/* System default restrictions always apply */
	if (unlikely(uc_req.value > uc_max.value))
		return uc_max;

	return uc_req;
}
/* …（同文件无关代码省略）… */
static inline void uclamp_rq_inc_id(struct rq *rq, struct task_struct *p,
				    enum uclamp_id clamp_id)
{
	struct uclamp_rq *uc_rq = &rq->uclamp[clamp_id];
	struct uclamp_se *uc_se = &p->uclamp[clamp_id];
	struct uclamp_bucket *bucket;

	lockdep_assert_rq_held(rq);

	/* Update task effective clamp */
	p->uclamp[clamp_id] = uclamp_eff_get(p, clamp_id);

	bucket = &uc_rq->bucket[uc_se->bucket_id];
	bucket->tasks++;
	uc_se->active = true;

	uclamp_idle_reset(rq, clamp_id, uc_se->value);

	/*
	 * Local max aggregation: rq buckets always track the max
	 * "requested" clamp value of its RUNNABLE tasks.
	 */
	if (bucket->tasks == 1 || uc_se->value > bucket->value)
		bucket->value = uc_se->value;

	if (uc_se->value > uclamp_rq_get(rq, clamp_id))
		uclamp_rq_set(rq, clamp_id, uc_se->value);
}
/* …（同文件无关代码省略）… */
static inline void uclamp_rq_inc(struct rq *rq, struct task_struct *p, int flags)
{
	enum uclamp_id clamp_id;

	/*
	 * Avoid any overhead until uclamp is actually used by the userspace.
	 *
	 * The condition is constructed such that a NOP is generated when
	 * sched_uclamp_used is disabled.
	 */
	if (!uclamp_is_used())
		return;

	if (unlikely(!p->sched_class->uclamp_enabled))
		return;

	/* Only inc the delayed task which being woken up. */
	if (p->se.sched_delayed && !(flags & ENQUEUE_DELAYED))
		return;

	for_each_clamp_id(clamp_id)
		uclamp_rq_inc_id(rq, p, clamp_id);

	/* Reset clamp idle holding when there is one RUNNABLE task */
	if (rq->uclamp_flags & UCLAMP_FLAG_IDLE)
		rq->uclamp_flags &= ~UCLAMP_FLAG_IDLE;
}
/* …（同文件无关代码省略）… */
	for_each_clamp_id(clamp_id) {
		uclamp_se_set(&p->uclamp_req[clamp_id],
			      uclamp_none(clamp_id), false);
	}
/* …（同文件无关代码省略）… */
void enqueue_task(struct rq *rq, struct task_struct *p, int flags)
{
	if (!(flags & ENQUEUE_NOCLOCK))
		update_rq_clock(rq);

	/*
	 * Can be before ->enqueue_task() because uclamp considers the
	 * ENQUEUE_DELAYED task before its ->sched_delayed gets cleared
	 * in ->enqueue_task().
	 */
	uclamp_rq_inc(rq, p, flags);

	p->sched_class->enqueue_task(rq, p, flags);

	psi_enqueue(p, flags);

	if (!(flags & ENQUEUE_RESTORE))
		sched_info_enqueue(rq, p);

	if (sched_core_enabled(rq))
		sched_core_enqueue(rq, p);
}
/* …（同文件无关代码省略）… */
void activate_task(struct rq *rq, struct task_struct *p, int flags)
{
	if (task_on_rq_migrating(p))
		flags |= ENQUEUE_MIGRATED;

	enqueue_task(rq, p, flags);

	WRITE_ONCE(p->on_rq, TASK_ON_RQ_QUEUED);
	ASSERT_EXCLUSIVE_WRITER(p->on_rq);
}
/* …（同文件无关代码省略）… */
void wakeup_preempt(struct rq *rq, struct task_struct *p, int flags)
{
	struct task_struct *donor = rq->donor;

	if (p->sched_class == rq->next_class) {
		rq->next_class->wakeup_preempt(rq, p, flags);

	} else if (sched_class_above(p->sched_class, rq->next_class)) {
		rq->next_class->wakeup_preempt(rq, p, flags);
		resched_curr(rq);
		rq->next_class = p->sched_class;
	}

	/*
	 * A queue event has occurred, and we're going to schedule.  In
	 * this case, we can save a useless back to back clock update.
	 */
	if (task_on_rq_queued(donor) && test_tsk_need_resched(rq->curr))
		rq_clock_skip_update(rq);
}
/* …（同文件无关代码省略）… */
static inline bool task_can_migrate_to_preferred(struct task_struct *p, int cpu)
{
	/* No need to migrate from a preferred CPU */
	if (cpu_preferred(cpu))
		return false;

	/* Only FAIR tasks honor preferred CPU state */
	if (unlikely(p->sched_class != &fair_sched_class))
		return false;

	/* Ignore preferred state if task affinity is changing */
	if (unlikely(!cpumask_test_cpu(task_cpu(p), p->cpus_ptr)))
		return false;

	return cpumask_intersects_and(p->cpus_ptr, cpu_preferred_mask,
				      task_cpu_possible_mask(p));
}
/* …（同文件无关代码省略）… */
static inline bool is_cpu_allowed(struct task_struct *p, int cpu)
{
	/* When not in the task's cpumask, no point in looking further. */
	if (!task_allowed_on_cpu(p, cpu))
		return false;

	/* migrate_disabled() must be allowed to finish. */
	if (is_migration_disabled(p))
		return cpu_online(cpu);

	/* Non kernel threads are not allowed during either online or offline. */
	if (!(p->flags & PF_KTHREAD)) {
		/* Try to use preferred CPU if task's affinity allows */
		if (task_can_migrate_to_preferred(p, cpu))
			return false;
		return cpu_active(cpu);
	}

	/* KTHREAD_IS_PER_CPU is always allowed. */
	if (kthread_is_per_cpu(p))
		return cpu_online(cpu);

	/* Regular kernel threads don't get to stay during offline. */
	if (cpu_dying(cpu))
		return false;

	/* Try to keep unbound kthreads on a preferred CPU if possible. */
	if (task_can_migrate_to_preferred(p, cpu))
		return false;

	/* Otherwise, they are allowed to run on online CPU. */
	return cpu_online(cpu);
}
/* …（同文件无关代码省略）… */
static void
do_set_cpus_allowed(struct task_struct *p, struct affinity_context *ctx)
{
	scoped_guard (sched_change, p, DEQUEUE_SAVE)
		p->sched_class->set_cpus_allowed(p, ctx);
}
/* …（同文件无关代码省略）… */
void set_cpus_allowed_force(struct task_struct *p, const struct cpumask *new_mask)
{
	struct affinity_context ac = {
		.new_mask  = new_mask,
		.user_mask = NULL,
		.flags     = SCA_USER,	/* clear the user requested mask */
	};
	union cpumask_rcuhead {
		cpumask_t cpumask;
		struct rcu_head rcu;
	};

	scoped_guard (__task_rq_lock, p)
		do_set_cpus_allowed(p, &ac);

	/*
	 * Because this is called with p->pi_lock held, it is not possible
	 * to use kfree() here (when PREEMPT_RT=y), therefore punt to using
	 * kfree_rcu().
	 */
	kfree_rcu((union cpumask_rcuhead *)ac.user_mask, rcu);
}
/* …（同文件无关代码省略）… */
static int select_fallback_rq(int cpu, struct task_struct *p)
{
	int nid = cpu_to_node(cpu);
	const struct cpumask *nodemask = NULL;
	enum { cpuset, possible, fail } state = cpuset;
	int dest_cpu;

	/*
	 * If the node that the CPU is on has been offlined, cpu_to_node()
	 * will return -1. There is no CPU on the node, and we should
	 * select the CPU on the other node.
	 */
	if (nid != -1) {
		nodemask = cpumask_of_node(nid);

		/* Look for allowed, online CPU in same node. */
		for_each_cpu(dest_cpu, nodemask) {
			if (is_cpu_allowed(p, dest_cpu))
				return dest_cpu;
		}
	}

	for (;;) {
		/* Any allowed, online CPU? */
		for_each_cpu(dest_cpu, p->cpus_ptr) {
			if (!is_cpu_allowed(p, dest_cpu))
				continue;

			goto out;
		}

		/* No more Mr. Nice Guy. */
		switch (state) {
		case cpuset:
			if (cpuset_cpus_allowed_fallback(p)) {
				state = possible;
				break;
			}
			fallthrough;
		case possible:
			set_cpus_allowed_force(p, task_cpu_fallback_mask(p));
			state = fail;
			break;
		case fail:
			BUG();
			break;
		}
	}

out:
	if (state != cpuset) {
		/*
		 * Don't tell them about moving exiting tasks or
		 * kernel threads (both mm NULL), since they never
		 * leave kernel.
		 */
		if (p->mm && printk_ratelimit()) {
			printk_deferred("process %d (%s) no longer affine to cpu%d\n",
					task_pid_nr(p), p->comm, cpu);
		}
	}

	return dest_cpu;
}
/* …（同文件无关代码省略）… */
static inline
int select_task_rq(struct task_struct *p, int cpu, int *wake_flags)
{
	lockdep_assert_held(&p->pi_lock);

	if (p->nr_cpus_allowed > 1 && !is_migration_disabled(p)) {
		cpu = p->sched_class->select_task_rq(p, cpu, *wake_flags);
		*wake_flags |= WF_RQ_SELECTED;
	} else {
		cpu = cpumask_any(p->cpus_ptr);
	}

	/*
	 * In order not to call set_task_cpu() on a blocking task we need
	 * to rely on ttwu() to place the task on a valid ->cpus_ptr
	 * CPU.
	 *
	 * Since this is common to all placement strategies, this lives here.
	 *
	 * [ this allows ->select_task() to simply return task_cpu(p) and
	 *   not worry about this generic constraint ]
	 */
	if (unlikely(!is_cpu_allowed(p, cpu)))
		cpu = select_fallback_rq(task_cpu(p), p);

	return cpu;
}
/* …（同文件无关代码省略）… */
	scoped_guard(raw_spinlock, &p->blocked_lock) {
		/* Task is waking up; clear any blocked_on relationship */
		__clear_task_blocked_on(p, NULL);

		/* If already current, don't need to return migrate */
		if (task_current(rq, p))
			return false;
	}
/* …（同文件无关代码省略）… */
	struct rq_flags rf;
	struct rq *rq;
	int wake_flags = WF_FORK;

	raw_spin_lock_irqsave(&p->pi_lock, rf.flags);
	WRITE_ONCE(p->__state, TASK_RUNNING);
/* …（同文件无关代码省略）… */
	 */
	p->recent_used_cpu = task_cpu(p);
	__set_task_cpu(p, select_task_rq(p, task_cpu(p), &wake_flags));
	rq = __task_rq_lock(p, &rf);
	update_rq_clock(rq);
	post_init_entity_util_avg(p);

	activate_task(rq, p, ENQUEUE_NOCLOCK | ENQUEUE_INITIAL);
	trace_sched_wakeup_new(p);
	wakeup_preempt(rq, p, wake_flags);
	if (p->sched_class->task_woken) {
