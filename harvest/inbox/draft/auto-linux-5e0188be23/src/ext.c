// AUTO-DRAFT from torvalds/linux PR #a9daf327f3ff90625204876981f88cfe2b6f6fff
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static bool dsq_is_rq_owned(struct scx_dispatch_q *dsq)
{
	switch (dsq->id) {
	case SCX_DSQ_LOCAL:
	case SCX_DSQ_REJECT:
	case SCX_DSQ_RESCUE:
		return true;
	default:
		return false;
	}
}
/* …（同文件无关代码省略）… */
__printf(5, 6) bool __scx_exit(struct scx_sched *sch,
			       enum scx_exit_kind kind, s64 exit_code,
			       s32 exit_cpu, const char *fmt, ...)
{
	va_list args;
	bool ret;

	va_start(args, fmt);
	ret = scx_vexit(sch, kind, exit_code, exit_cpu, fmt, args);
	va_end(args);

	return ret;
}
/* …（同文件无关代码省略）… */
static bool u32_before(u32 a, u32 b)
{
	return (s32)(a - b) < 0;
}
/* …（同文件无关代码省略）… */
bool scx_is_descendant(struct scx_sched *sch, struct scx_sched *ancestor)
{
	if (sch->level < ancestor->level)
		return false;
	return sch->ancestors[ancestor->level] == ancestor;
}
/* …（同文件无关代码省略）… */
static struct scx_dispatch_q *find_global_dsq(struct scx_sched *sch, s32 cpu)
{
	return &sch->pnode[cpu_to_node(cpu)]->global_dsq;
}
/* …（同文件无关代码省略）… */
static struct scx_dispatch_q *find_user_dsq(struct scx_sched *sch, u64 dsq_id)
{
	return rhashtable_lookup(&sch->dsq_hash, &dsq_id, dsq_hash_params);
}
/* …（同文件无关代码省略）… */
static struct scx_dispatch_q *bypass_enq_target_dsq(struct scx_sched *sch, struct rq *rq,
						    struct task_struct *p, u64 *enq_flags)
{
	struct scx_sched *host = scx_exec_sched(sch, cpu_of(rq));
	struct scx_dispatch_q *dsq;

	/*
	 * If @sch is a sub-sched which is bypassing, its tasks should go into
	 * the bypass DSQs of the nearest ancestor which is not bypassing. The
	 * not-bypassing ancestor is responsible for scheduling all tasks from
	 * bypassing sub-trees. If all ancestors including root are bypassing,
	 * all tasks should go to the root's bypass DSQs.
	 *
	 * Whenever a sched starts bypassing, all runnable tasks in its subtree
	 * are re-enqueued after scx_bypassing() is turned on, guaranteeing that
	 * all tasks are transferred to the right DSQs.
	 *
	 * A cpu's dispatch only descends into a host holding %SCX_CAP_BASE on
	 * it, so the bypass DSQ of a host without BASE here is never consumed.
	 * Send @p to rescue on this cpu instead, resolving against @host.
	 * scx_dispatch_enqueue() resolves a local DSQ again, against the
	 * bypassing @sch. %SCX_ENQ_IGNORE_CAPS makes that second resolution
	 * admit @p.
	 *
	 * A migration-disabled task can't leave this cpu, and a task with a
	 * migration pending must run here until the migration moves it. Either
	 * would wait on the unconsumed bypass DSQ until the host regains BASE.
	 * Resolve them too when rescue is disabled. The resolution admits both
	 * to the local DSQ, as it does any insert of such a task.
	 */
	if (unlikely(scx_missing_caps(host, cpu_of(rq), SCX_CAP_BASE)) &&
	    (scx_rescue_enabled() || is_migration_disabled(p) || p->migration_pending)) {
		*enq_flags |= SCX_ENQ_RESCUE;
		dsq = scx_resolve_local_dsq(host, rq, p, enq_flags);
		*enq_flags |= SCX_ENQ_IGNORE_CAPS;
	} else {
		dsq = scx_bypass_dsq(host, cpu_of(rq));
	}
	return dsq;
}
/* …（同文件无关代码省略）… */
static bool rq_is_open(struct rq *rq, u64 enq_flags)
{
	lockdep_assert_rq_held(rq);

	/*
	 * A higher-priority class task is either running or in the process of
	 * waking up on @rq.
	 */
	if (sched_class_above(rq->next_class, &ext_sched_class))
		return false;

	/*
	 * @rq is either in transition to or in idle and there is no
	 * higher-priority class task waking up on it.
	 */
	if (sched_class_above(&ext_sched_class, rq->next_class))
		return true;

	/*
	 * @rq is either picking, in transition to, or running an SCX task.
	 */

	/*
	 * If we're in the dispatch path holding rq lock, $donor may or may not
	 * be ready depending on whether the on-going dispatch decides to extend
	 * $donor's slice. We say yes here and resolve it at the end of dispatch.
	 * See dispatch_one().
	 */
	if (rq->scx.flags & SCX_RQ_IN_DISPATCH)
		return true;

	/*
	 * The preemption flags clear $donor's slice if on SCX and kick dispatch,
	 * so allow it to avoid spuriously triggering reenq on a combined
	 * PREEMPT|IMMED insertion.
	 */
	if (enq_flags & (SCX_ENQ_PREEMPT | SCX_ENQ_PREEMPT_LAZY)) {
		struct task_struct *donor = rq->donor;

		/*
		 * A protected slice refuses the preemption and the cpu stays
		 * occupied. See rq_owned_post_enq().
		 */
		return donor->sched_class != &ext_sched_class ||
			likely(!(donor->scx.flags & SCX_TASK_PROTECTED));
	}

	/*
	 * @rq is either in transition to or running an SCX task and can't go
	 * idle without another SCX dispatch cycle.
	 */
	return false;
}
/* …（同文件无关代码省略）… */
static void scx_rq_lock_drop(struct rq *rq)
{
	lockdep_assert_rq_held(rq);
#ifdef CONFIG_SCHED_CORE
	if (sched_core_enabled(rq))
		rq->scx.lock_drop_seq++;
#endif
}
/* …（同文件无关代码省略）… */
static void switch_rq_lock(struct rq *from, struct rq *to)
{
	bool tracked = scx_locked_rq() == from;

	if (tracked)
		update_locked_rq(NULL);
	scx_rq_lock_drop(from);
	raw_spin_rq_unlock(from);
	raw_spin_rq_lock(to);
	if (tracked)
		update_locked_rq(to);
}
/* …（同文件无关代码省略）… */
static struct scx_cmask *scx_fill_cmask_scratch(struct scx_sched *sch,
						const struct cpumask *cpumask)
{
	struct scx_cmask *kern_va = *this_cpu_ptr(sch->set_cmask_scratch);
	struct scx_cmask_ref ref;

	scx_cmask_ref_init_kern(sch, kern_va, 0, num_possible_cpus(), &ref);
	scx_cmask_ref_from_cpumask(&ref, cpumask);
	return kern_va;
}
/* …（同文件无关代码省略）… */
static inline void scx_call_op_set_cpumask(struct scx_sched *sch, struct rq *rq,
					   struct task_struct *task,
					   const struct cpumask *cpumask)
{
	if (scx_is_cid_type())
		SCX_CALL_CID_OP_TASK(sch, set_cmask, rq, task,
				     scx_fill_cmask_scratch(sch, cpumask));
	else
		SCX_CALL_OP_TASK(sch, set_cpumask, rq, task, cpumask);
}
/* …（同文件无关代码省略）… */
	/* iterate in the reverse dispatch order */
	SCX_DSQ_ITER_REV		= 1U << 16,

	__SCX_DSQ_ITER_HAS_REENQ	= 1U << 29,
	__SCX_DSQ_ITER_HAS_SLICE	= 1U << 30,
	__SCX_DSQ_ITER_HAS_VTIME	= 1U << 31,

	__SCX_DSQ_ITER_USER_FLAGS	= SCX_DSQ_ITER_REV,
	__SCX_DSQ_ITER_ALL_FLAGS	= __SCX_DSQ_ITER_USER_FLAGS |
					  __SCX_DSQ_ITER_HAS_REENQ |
					  __SCX_DSQ_ITER_HAS_SLICE |
					  __SCX_DSQ_ITER_HAS_VTIME,
/* …（同文件无关代码省略）… */
static struct task_struct *nldsq_next_task(struct scx_dispatch_q *dsq,
					   struct task_struct *cur, bool rev)
{
	struct list_head *list_node;
	struct scx_dsq_list_node *dsq_lnode;

	lockdep_assert_held(&dsq->lock);

	if (cur)
		list_node = &cur->scx.dsq_list.node;
	else
		list_node = &dsq->list;

	/* find the next task, need to skip BPF iteration cursors */
	do {
		if (rev)
			list_node = list_node->prev;
		else
			list_node = list_node->next;

		if (list_node == &dsq->list)
			return NULL;

		dsq_lnode = container_of(list_node, struct scx_dsq_list_node,
					 node);
	} while (dsq_lnode->flags & SCX_DSQ_LNODE_ITER_CURSOR);

	return container_of(dsq_lnode, struct task_struct, scx.dsq_list);
}
/* …（同文件无关代码省略）… */
static struct task_struct *nldsq_cursor_next_task(struct scx_dsq_list_node *cursor,
						  struct scx_dispatch_q *dsq)
{
	bool rev = cursor->flags & SCX_DSQ_ITER_REV;
	struct task_struct *p;

	lockdep_assert_held(&dsq->lock);
	BUG_ON(!(cursor->flags & SCX_DSQ_LNODE_ITER_CURSOR));

	if (list_empty(&cursor->node))
		p = NULL;
	else
		p = container_of(cursor, struct task_struct, scx.dsq_list);

	/* skip cursors and tasks that were queued after @cursor init */
	do {
		p = nldsq_next_task(dsq, p, rev);
	} while (p && unlikely(u32_before(cursor->priv, p->scx.dsq_seq)));

	if (p) {
		if (rev)
			list_move_tail(&cursor->node, &p->scx.dsq_list.node);
		else
			list_move(&cursor->node, &p->scx.dsq_list.node);
	} else {
		list_del_init(&cursor->node);
	}

	return p;
}
/* …（同文件无关代码省略）… */
static bool nldsq_cursor_lost_task(struct scx_dsq_list_node *cursor,
				   struct rq *rq, struct scx_dispatch_q *dsq,
				   struct task_struct *p)
{
	lockdep_assert_rq_held(rq);
	lockdep_assert_held(&dsq->lock);

	/*
	 * @p could have already left $src_dsq, got re-enqueud, or be in the
	 * process of being consumed by someone else.
	 */
	if (unlikely(p->scx.dsq != dsq ||
		     u32_before(cursor->priv, p->scx.dsq_seq) ||
		     p->scx.holding_cpu >= 0))
		return true;

	/* if @p has stayed on @dsq, its rq couldn't have changed */
	if (WARN_ON_ONCE(rq != task_rq(p)))
		return true;

	return false;
}
/* …（同文件无关代码省略）… */
struct bpf_iter_scx_dsq_kern {
	struct scx_dsq_list_node	cursor;
	struct scx_dispatch_q		*dsq;
	u64				slice;
	u64				vtime;
} __attribute__((aligned(8)));
/* …（同文件无关代码省略）… */
struct bpf_iter_scx_dsq {
	u64				__opaque[6];
} __attribute__((aligned(8)));
/* …（同文件无关代码省略）… */
u32 scx_get_task_state(const struct task_struct *p)
{
	return p->scx.flags & SCX_TASK_STATE_MASK;
}
/* …（同文件无关代码省略）… */
void scx_set_task_state(struct task_struct *p, u32 state)
{
	u32 prev_state = scx_get_task_state(p);
	bool warn = false;

	switch (state) {
	case SCX_TASK_NONE:
		warn = prev_state == SCX_TASK_DEAD;
		break;
	case SCX_TASK_INIT_BEGIN:
		warn = prev_state != SCX_TASK_NONE;
		break;
	case SCX_TASK_INIT:
		warn = prev_state != SCX_TASK_INIT_BEGIN;
		p->scx.flags |= SCX_TASK_RESET_RUNNABLE_AT;
		break;
	case SCX_TASK_READY:
		warn = !(prev_state == SCX_TASK_INIT ||
			 prev_state == SCX_TASK_ENABLED);
		break;
	case SCX_TASK_ENABLED:
		warn = prev_state != SCX_TASK_READY;
		break;
	case SCX_TASK_DEAD:
		warn = !(prev_state == SCX_TASK_NONE ||
			 prev_state == SCX_TASK_INIT_BEGIN);
		break;
	default:
		WARN_ONCE(1, "sched_ext: Invalid task state %d -> %d for %s[%d]",
			  prev_state, state, p->comm, p->pid);
		return;
	}

	WARN_ONCE(warn, "sched_ext: Invalid task state transition 0x%x -> 0x%x for %s[%d]",
		  prev_state, state, p->comm, p->pid);

	p->scx.flags &= ~SCX_TASK_STATE_MASK;
	p->scx.flags |= state;
}
/* …（同文件无关代码省略）… */
static void wait_ops_state(struct task_struct *p, unsigned long opss)
{
	do {
		cpu_relax();
	} while (atomic_long_read_acquire(&p->scx.ops_state) == opss);
}
/* …（同文件无关代码省略）… */
static inline bool __cpu_valid(s32 cpu)
{
	return likely(cpu >= 0 && cpu < nr_cpu_ids && cpu_possible(cpu));
}
/* …（同文件无关代码省略）… */
bool scx_cpu_valid(struct scx_sched *sch, s32 cpu, const char *where)
{
	if (__cpu_valid(cpu)) {
		return true;
	} else {
		scx_error(sch, "invalid CPU %d%s%s", cpu, where ? " " : "", where ?: "");
		return false;
	}
}
/* …（同文件无关代码省略）… */
static void clear_task_slice_oob(struct task_struct *p)
{
	if (unlikely(atomic64_read(&p->scx.slice_oob)))
		atomic64_set(&p->scx.slice_oob, 0);
}
/* …（同文件无关代码省略）… */
static bool set_task_slice_keep_oob(struct task_struct *p, u64 slice)
{
	lockdep_assert_rq_held(task_rq(p));

	if (unlikely(p->scx.flags & SCX_TASK_PROTECTED))
		return false;

	p->scx.slice = slice;
	return true;
}
/* …（同文件无关代码省略）… */
bool scx_set_task_slice(struct task_struct *p, u64 slice)
{
	if (!set_task_slice_keep_oob(p, slice))
		return false;
	clear_task_slice_oob(p);
	return true;
}
/* …（同文件无关代码省略）… */
static void apply_task_slice_oob(struct rq *rq, struct task_struct *p)
{
	u64 oob, dur, slice;

	lockdep_assert_rq_held(rq);

	if (likely(!atomic64_read(&p->scx.slice_oob)))
		return;

	oob = atomic64_xchg(&p->scx.slice_oob, 0);
	if (unlikely(!oob))
		return;

	/* the issuing scheduler no longer owns @p, drop the request */
	if (unlikely(((oob >> SCX_SLICE_OOB_ID_SHIFT) & SCX_SLICE_OOB_ID_MASK) !=
		     (scx_task_sched(p)->id & SCX_SLICE_OOB_ID_MASK)))
		return;

	dur = oob & SCX_SLICE_OOB_DUR_MASK;
	slice = dur == SCX_SLICE_OOB_DUR_MASK ? SCX_SLICE_INF : dur;

	if (slice > p->scx.slice &&
	    unlikely(scx_task_has_kernel_slice(p) ||
		     scx_missing_caps(scx_task_sched(p), cpu_of(rq), SCX_CAP_BASE))) {
		__scx_add_event(scx_task_sched(p), SCX_EV_SLICE_DENIED, 1);
		return;
	}

	if (unlikely(!set_task_slice_keep_oob(p, slice)))
		__scx_add_event(scx_task_sched(p), SCX_EV_SLICE_DENIED, 1);
}
/* …（同文件无关代码省略）… */
static void apply_slice_vtime(struct task_struct *p, u64 slice, u64 vtime, u64 enq_flags)
{
	if (slice) {
		p->scx.slice = slice;
		/*
		 * An explicit slice supersedes a pending oob request. A carried
		 * default refill is not an explicit request and must keep it.
		 */
		if (!(enq_flags & SCX_ENQ_SLICE_DFL))
			clear_task_slice_oob(p);
	} else if (!p->scx.slice) {
		p->scx.slice = 1;
	}

	if (enq_flags & SCX_ENQ_DSQ_PRIQ)
		p->scx.dsq_vtime = vtime;
}
/* …（同文件无关代码省略）… */
static void refill_task_slice_dfl(struct scx_sched *sch, struct task_struct *p)
{
	if (scx_task_has_kernel_slice(p))
		return;

	/*
	 * A default refill is not an explicit request, so it must not drop a
	 * pending out-of-band one, which is applied when @p next runs.
	 */
	set_task_slice_keep_oob(p, READ_ONCE(sch->slice_dfl));
	__scx_add_event(sch, SCX_EV_REFILL_SLICE_DFL, 1);
}
/* …（同文件无关代码省略）… */
static void expire_task_slice(struct task_struct *p)
{
	/*
	 * Unlike the refill above, an expiry supersedes a pending out-of-band
	 * request. A kept request would be applied at the next
	 * update_curr_scx() and undo the expiry.
	 */
	if (!scx_task_has_kernel_slice(p))
		scx_set_task_slice(p, 0);
}
/* …（同文件无关代码省略）… */
static void update_curr_scx(struct rq *rq)
{
	struct task_struct *donor;
	s64 delta_exec;

	/*
	 * update_curr_scx() is selected through rq->donor->sched_class, not
	 * rq->curr->sched_class, so @donor is always an EXT task here. If an EXT
	 * owner executes for a FAIR donor, FAIR's update_curr() runs instead.
	 */
	donor = rq->donor;

	/* apply even on 0 delta_exec, callers may still act on the slice */
	apply_task_slice_oob(rq, donor);

	delta_exec = update_curr_common(rq);
	if (unlikely(delta_exec <= 0))
		return;

	if (donor->scx.slice != SCX_SLICE_INF)
		donor->scx.slice -= min_t(u64, donor->scx.slice, delta_exec);

	if (unlikely(donor == scx_rescuee(rq)))
		scx_rescue_charge(rq, delta_exec);

	dl_server_update(&rq->ext_server, delta_exec);
}
/* …（同文件无关代码省略）… */
static void scx_resched_curr_lazy(struct rq *rq)
{
	resched_curr_lazy(rq);

	if (rq->scx.flags & SCX_RQ_CAN_STOP_TICK) {
		rq->scx.flags &= ~SCX_RQ_CAN_STOP_TICK;
		if (rq->clock_update_flags < RQCF_UPDATED)
			update_rq_clock(rq);
		update_other_load_avgs(rq);
		sched_update_tick_dependency(rq);
	}
}
/* …（同文件无关代码省略）… */
static void scx_dispatch_enqueue(struct scx_sched *sch, struct rq *rq,
				 struct scx_dispatch_q *dsq, struct task_struct *p,
				 u64 slice, u64 vtime, u64 enq_flags)
{
	bool is_rq_owned = dsq_is_rq_owned(dsq);

	if (dsq->id == SCX_DSQ_LOCAL)
		dsq = scx_resolve_local_dsq(sch, rq, p, &enq_flags);

	WARN_ON_ONCE(p->scx.dsq || !list_empty(&p->scx.dsq_list.node));
	WARN_ON_ONCE((p->scx.dsq_flags & SCX_TASK_DSQ_ON_PRIQ) ||
		     !RB_EMPTY_NODE(&p->scx.dsq_priq));

	if (!is_rq_owned) {
		raw_spin_lock_nested(&dsq->lock,
			(enq_flags & SCX_ENQ_NESTED) ? SINGLE_DEPTH_NESTING : 0);

		if (unlikely(dsq->id == SCX_DSQ_INVALID)) {
			scx_error(sch, "attempting to dispatch to a destroyed dsq");
			/* fall back to the global dsq */
			raw_spin_unlock(&dsq->lock);
			dsq = find_global_dsq(sch, task_cpu(p));
			raw_spin_lock(&dsq->lock);
		}
	}

	if (unlikely((dsq->id & SCX_DSQ_FLAG_BUILTIN) &&
		     (enq_flags & SCX_ENQ_DSQ_PRIQ))) {
		/*
		 * SCX_DSQ_LOCAL and SCX_DSQ_GLOBAL DSQs always consume from
		 * their FIFO queues. To avoid confusion and accidentally
		 * starving vtime-dispatched tasks by FIFO-dispatched tasks, we
		 * disallow any internal DSQ from doing vtime ordering of
		 * tasks.
		 */
		scx_error(sch, "cannot use vtime ordering for built-in DSQs");
		enq_flags &= ~SCX_ENQ_DSQ_PRIQ;
	}

	/*
	 * @dsq is locked and @enq_flags is sanitized. Commit the carried slice
	 * and vtime before the PRIQ insertion below reads the new dsq_vtime.
	 */
	if (enq_flags & SCX_ENQ_APPLY_SLICE)
		apply_slice_vtime(p, slice, vtime, enq_flags);

	if (enq_flags & SCX_ENQ_DSQ_PRIQ) {
		struct rb_node *rbp;

		/*
		 * A PRIQ DSQ shouldn't be using FIFO enqueueing. As tasks are
		 * linked to both the rbtree and list on PRIQs, this can only be
		 * tested easily when adding the first task.
		 */
		if (unlikely(RB_EMPTY_ROOT(&dsq->priq) &&
			     nldsq_next_task(dsq, NULL, false)))
			scx_error(sch, "DSQ ID 0x%016llx already had FIFO-enqueued tasks",
				  dsq->id);

		p->scx.dsq_flags |= SCX_TASK_DSQ_ON_PRIQ;
		rb_add(&p->scx.dsq_priq, &dsq->priq, scx_dsq_priq_less);

		/*
		 * Find the previous task and insert after it on the list so
		 * that @dsq->list is vtime ordered.
		 */
		rbp = rb_prev(&p->scx.dsq_priq);
		if (rbp) {
			struct task_struct *prev =
				container_of(rbp, struct task_struct,
					     scx.dsq_priq);
			list_add(&p->scx.dsq_list.node, &prev->scx.dsq_list.node);
			/* first task unchanged - no update needed */
		} else {
			list_add(&p->scx.dsq_list.node, &dsq->list);
			/* not builtin and new task is at head - use fastpath */
			rcu_assign_pointer(dsq->first_task, p);
		}
	} else {
		/* a FIFO DSQ shouldn't be using PRIQ enqueuing */
		if (unlikely(!RB_EMPTY_ROOT(&dsq->priq)))
			scx_error(sch, "DSQ ID 0x%016llx already had PRIQ-enqueued tasks",
				  dsq->id);

		if (enq_flags & (SCX_ENQ_HEAD | SCX_ENQ_PREEMPT | SCX_ENQ_PREEMPT_LAZY)) {
			/* new task inserted at head - use fastpath */
			if (dsq_insert_head(dsq, p) && !(dsq->id & SCX_DSQ_FLAG_BUILTIN))
				rcu_assign_pointer(dsq->first_task, p);
		} else {
			/*
			 * dsq->list can contain parked BPF iterator cursors, so
			 * list_empty() here isn't a reliable proxy for "no real
			 * task in the DSQ". Test dsq->first_task directly.
			 */
			list_add_tail(&p->scx.dsq_list.node, &dsq->list);
			if (!dsq->first_task && !(dsq->id & SCX_DSQ_FLAG_BUILTIN))
				rcu_assign_pointer(dsq->first_task, p);
		}
	}

	/* seq records the order tasks are queued, used by BPF DSQ iterator */
	WRITE_ONCE(dsq->seq, dsq->seq + 1);
	p->scx.dsq_seq = dsq->seq;

	dsq_inc_nr(dsq, p, enq_flags);
	p->scx.dsq = dsq;

	/*
	 * Update custody and call ops.dequeue() before clearing ops_state:
	 * once ops_state is cleared, waiters in ops_dequeue() can proceed
	 * and dequeue_task_scx() will RMW p->scx.flags. If we clear
	 * ops_state first, both sides would modify p->scx.flags
	 * concurrently in a non-atomic way.
	 */
	if (is_rq_owned) {
		rq_owned_post_enq(sch, rq, dsq, p, enq_flags);
	} else {
		bool call_dequeue = false;

		/*
		 * Global and bypass DSQs are terminal - the task leaves the
		 * scheduler's custody, so ops.dequeue() fires. It can run
		 * without @p's rq lock (finish_dispatch() passes the dispatch
		 * rq); that's safe because dequeue_task_scx() waits on
		 * SCX_OPSS_DISPATCHING (see the ops_state note above) and so
		 * can't race it. A non-terminal DSQ keeps the task in custody.
		 * The custody transfer happens under @dsq->lock so that later
		 * consumers see the flag clear; the callback runs after
		 * @dsq->lock is dropped because it may lock a DSQ itself.
		 */
		if (dsq->id == SCX_DSQ_GLOBAL || dsq->id == SCX_DSQ_BYPASS)
			call_dequeue = task_leave_custody(p);
		else
			p->scx.flags |= SCX_TASK_IN_CUSTODY;

		raw_spin_unlock(&dsq->lock);

		if (call_dequeue && SCX_HAS_OP(sch, dequeue))
			SCX_CALL_OP_TASK(sch, dequeue, rq, p, 0);
	}

	/*
	 * We're transitioning out of QUEUEING or DISPATCHING. store_release to
	 * match waiters' load_acquire.
	 */
	if (enq_flags & SCX_ENQ_CLEAR_OPSS)
		atomic_long_set_release(&p->scx.ops_state, SCX_OPSS_NONE);
}
/* …（同文件无关代码省略）… */
void scx_task_unlink_from_dsq(struct task_struct *p, struct scx_dispatch_q *dsq)
{
	WARN_ON_ONCE(list_empty(&p->scx.dsq_list.node));

	if (p->scx.dsq_flags & SCX_TASK_DSQ_ON_PRIQ) {
		rb_erase(&p->scx.dsq_priq, &dsq->priq);
		RB_CLEAR_NODE(&p->scx.dsq_priq);
		p->scx.dsq_flags &= ~SCX_TASK_DSQ_ON_PRIQ;
	}
	p->scx.dsq_flags &= ~SCX_TASK_DSQ_REENQ;

	list_del_init(&p->scx.dsq_list.node);
	dsq_dec_nr(dsq, p);

	if (!(dsq->id & SCX_DSQ_FLAG_BUILTIN) && rcu_access_pointer(dsq->first_task) == p) {
		struct task_struct *first_task;

		first_task = nldsq_next_task(dsq, NULL, false);
		rcu_assign_pointer(dsq->first_task, first_task);
	}
}
/* …（同文件无关代码省略）… */
void scx_dispatch_dequeue(struct rq *rq, struct task_struct *p)
{
	struct scx_dispatch_q *dsq = p->scx.dsq;
	bool is_rq_owned = dsq && dsq_is_rq_owned(dsq);

	lockdep_assert_rq_held(rq);

	if (!dsq) {
		/*
		 * If !dsq && on-list, @p is on @rq's ddsp_deferred_locals.
		 * Unlinking is all that's needed to cancel.
		 */
		if (unlikely(!list_empty(&p->scx.dsq_list.node)))
			list_del_init(&p->scx.dsq_list.node);

		/*
		 * When dispatch_to_local_dsq() or remote consumption moves a
		 * task to a local DSQ, the task isn't associated with any DSQ
		 * but @p->scx.holding_cpu may be set. Clearing holding_cpu
		 * tells dispatch_to_local_dsq() that it lost to a dequeue.
		 */
		if (p->scx.holding_cpu >= 0)
			p->scx.holding_cpu = -1;

		return;
	}

	if (!is_rq_owned)
		raw_spin_lock(&dsq->lock);

	/*
	 * Now that we hold @dsq->lock, @p->holding_cpu and @p->scx.dsq_* can't
	 * change underneath us.
	*/
	if (p->scx.holding_cpu < 0) {
		/* @p must still be on @dsq, dequeue */
		scx_task_unlink_from_dsq(p, dsq);
	} else {
		/*
		 * We're racing against unlink_dsq_and_switch_rq_lock(),
		 * which already removed @p from @dsq and set
		 * @p->scx.holding_cpu. Clear holding_cpu to tell
		 * unlink_dsq_and_switch_rq_lock() that it lost the race.
		 */
		WARN_ON_ONCE(!list_empty(&p->scx.dsq_list.node));
		p->scx.holding_cpu = -1;
	}
	p->scx.dsq = NULL;

	if (!is_rq_owned)
		raw_spin_unlock(&dsq->lock);
}
/* …（同文件无关代码省略）… */
static void dispatch_dequeue_locked(struct task_struct *p,
				    struct scx_dispatch_q *dsq)
{
	lockdep_assert_rq_held(task_rq(p));
	lockdep_assert_held(&dsq->lock);

	scx_task_unlink_from_dsq(p, dsq);
	p->scx.dsq = NULL;
}
/* …（同文件无关代码省略）… */
static struct scx_dispatch_q *find_dsq_for_dispatch(struct scx_sched *sch,
						    struct rq *rq, u64 dsq_id,
						    s32 tcpu)
{
	struct scx_dispatch_q *dsq;

	if (dsq_id == SCX_DSQ_LOCAL)
		return &rq->scx.local_dsq;

	if ((dsq_id & SCX_DSQ_LOCAL_ON) == SCX_DSQ_LOCAL_ON) {
		s32 cpu = scx_cpu_ret(sch, dsq_id & SCX_DSQ_LOCAL_CPU_MASK);

		if (!scx_cpu_valid(sch, cpu, "in SCX_DSQ_LOCAL_ON dispatch verdict"))
			return find_global_dsq(sch, tcpu);

		return &cpu_rq(cpu)->scx.local_dsq;
	}

	if (dsq_id == SCX_DSQ_GLOBAL)
		dsq = find_global_dsq(sch, tcpu);
	else
		dsq = find_user_dsq(sch, dsq_id);

	/*
	 * Built-in DSQs are never inserted into dsq_hash, so REJECT and RESCUE
	 * hit the error below. They cannot be reached with an ID.
	 */
	if (unlikely(!dsq)) {
		scx_error(sch, "non-existent DSQ 0x%llx", dsq_id);
		return find_global_dsq(sch, tcpu);
	}

	return dsq;
}
/* …（同文件无关代码省略）… */
static inline void clear_direct_dispatch(struct task_struct *p)
{
	p->scx.ddsp_dsq_id = SCX_DSQ_INVALID;
	p->scx.ddsp_enq_flags = 0;
}
/* …（同文件无关代码省略）… */
static void direct_dispatch(struct scx_sched *sch, struct task_struct *p,
			    u64 enq_flags)
{
	struct rq *rq = task_rq(p);
	struct scx_dispatch_q *dsq =
		find_dsq_for_dispatch(sch, rq, p->scx.ddsp_dsq_id, task_cpu(p));
	u64 ddsp_enq_flags, slice, vtime;

	p->scx.ddsp_enq_flags |= enq_flags;

	/*
	 * We are in the enqueue path with @rq locked and pinned, and thus can't
	 * double lock a remote rq and enqueue to its local DSQ. For
	 * DSQ_LOCAL_ON verdicts targeting the local DSQ of a remote CPU, defer
	 * the enqueue so that it's executed when @rq can be unlocked.
	 */
	if (dsq->id == SCX_DSQ_LOCAL && dsq != &rq->scx.local_dsq) {
		unsigned long opss;

		opss = atomic_long_read(&p->scx.ops_state) & SCX_OPSS_STATE_MASK;

		switch (opss & SCX_OPSS_STATE_MASK) {
		case SCX_OPSS_NONE:
			break;
		case SCX_OPSS_QUEUEING:
			/*
			 * As @p was never passed to the BPF side, _release is
			 * not strictly necessary. Still do it for consistency.
			 */
			atomic_long_set_release(&p->scx.ops_state, SCX_OPSS_NONE);
			break;
		default:
			WARN_ONCE(true, "sched_ext: %s[%d] has invalid ops state 0x%lx in direct_dispatch()",
				  p->comm, p->pid, opss);
			atomic_long_set_release(&p->scx.ops_state, SCX_OPSS_NONE);
			break;
		}

		WARN_ON_ONCE(p->scx.dsq || !list_empty(&p->scx.dsq_list.node));
		list_add_tail(&p->scx.dsq_list.node,
			      &rq->scx.ddsp_deferred_locals);
		schedule_deferred_locked(rq);
		return;
	}

	ddsp_enq_flags = p->scx.ddsp_enq_flags;
	slice = p->scx.ddsp_slice;
	vtime = p->scx.ddsp_vtime;
	clear_direct_dispatch(p);

	scx_dispatch_enqueue(sch, rq, dsq, p, slice, vtime,
			     ddsp_enq_flags | SCX_ENQ_APPLY_SLICE | SCX_ENQ_CLEAR_OPSS);
}
/* …（同文件无关代码省略）… */
bool scx_rq_online(struct rq *rq)
{
	/*
	 * Test both cpu_active() and %SCX_RQ_ONLINE. %SCX_RQ_ONLINE follows the
	 * core's rq online state. cpu_active() test guarantees that, if this
	 * function returns %true, %SCX_RQ_ONLINE will stay set until the
	 * current scheduling operation is complete even if we aren't locking
	 * @rq.
	 */
	return likely((rq->scx.flags & SCX_RQ_ONLINE) && cpu_active(cpu_of(rq)));
}
/* …（同文件无关代码省略）… */
void scx_do_enqueue_task(struct rq *rq, struct task_struct *p, u64 enq_flags,
			 int sticky_cpu)
{
	enum {
		ENQ_ACTION_NONE,
		ENQ_ACTION_DIRECT,
		ENQ_ACTION_LOCAL_NOREFILL,
		ENQ_ACTION_ENQUEUE,
	} action = ENQ_ACTION_NONE;
	struct scx_sched *sch = scx_task_sched(p);
	struct task_struct **ddsp_taskp;
	struct scx_dispatch_q *dsq;
	unsigned long qseq;

	WARN_ON_ONCE(!(p->scx.flags & SCX_TASK_QUEUED));

	/* internal movements - rq migration / RESTORE */
	if (sticky_cpu == cpu_of(rq))
		goto local_norefill;

	/*
	 * Clear persistent TASK_IMMED for fresh enqueues, see dsq_inc_nr().
	 * Note that exiting and migration-disabled tasks that skip
	 * ops.enqueue() below will lose IMMED protection unless
	 * %SCX_OPS_ENQ_EXITING / %SCX_OPS_ENQ_MIGRATION_DISABLED are set.
	 */
	p->scx.flags &= ~SCX_TASK_IMMED;

	/*
	 * If !scx_rq_online(), we already told the BPF scheduler that the CPU
	 * is offline and are just running the hotplug path. Don't bother the
	 * BPF scheduler.
	 */
	if (!scx_rq_online(rq))
		goto local;

	if (scx_bypassing(sch, cpu_of(rq))) {
		__scx_add_event(sch, SCX_EV_BYPASS_DISPATCH, 1);
		goto bypass;
	}

	/*
	 * A task reenqueued too many times without running means the scheduler
	 * keeps re-deciding a placement it can't honor, e.g. re-inserting to a
	 * cid it lacks caps on. Eject the owning scheduler and strand the task
	 * to be picked up during sched exit.
	 *
	 * Bypass guarantees that every runnable task keeps running, and the
	 * disable relies on that to take sleeping locks. The kernel places the
	 * tasks of a bypassing scheduler and those on an offline rq, and
	 * counting their reenqueues could strand a task under bypass and
	 * deadlock the disable on a lock the task holds, so both tests stay
	 * above the count.
	 */
	if (enq_flags & SCX_ENQ_REENQ) {
		if (++p->scx.reenq_cnt > 1)
			__scx_add_event(sch, SCX_EV_REENQ_REPEAT, 1);

		if (unlikely(p->scx.reenq_cnt > SCX_REENQ_MAX_REPEAT)) {
			__scx_exit(sch, SCX_EXIT_ERROR_REENQ, 0, cpu_of(rq),
				   "%s[%d] reenqueued %u times without running",
				   p->comm, p->pid, p->scx.reenq_cnt);
			goto out;
		}
	}

	if (p->scx.ddsp_dsq_id != SCX_DSQ_INVALID)
		goto direct;

	/*
	 * A full wakeup enqueue happens before is_blocked is cleared. Don't
	 * report it as another blocked-donor admission.
	 */
	if ((sch->ops.flags & SCX_OPS_ENQ_BLOCKED) && p->is_blocked &&
	    !(enq_flags & SCX_ENQ_WAKEUP)) {
		enq_flags |= SCX_ENQ_BLOCKED;
	} else {
		/* see %SCX_OPS_ENQ_EXITING */
		if (unlikely(p->flags & PF_EXITING) &&
		    !(sch->ops.flags & SCX_OPS_ENQ_EXITING)) {
			__scx_add_event(sch, SCX_EV_ENQ_SKIP_EXITING, 1);
			enq_flags |= SCX_ENQ_RESCUE;	/* avoid looping on cap rejection */
			goto local;
		}

		/* see %SCX_OPS_ENQ_MIGRATION_DISABLED */
		if (!(sch->ops.flags & SCX_OPS_ENQ_MIGRATION_DISABLED) &&
		    is_migration_disabled(p)) {
			__scx_add_event(sch, SCX_EV_ENQ_SKIP_MIGRATION_DISABLED, 1);
			goto local;
		}
	}

	if (unlikely(!SCX_HAS_OP(sch, enqueue)))
		goto global;

	/*
	 * DSQ bypass didn't trigger, enqueue on the BPF scheduler. Wrap the
	 * per-task qseq counter where the QSEQ field wraps and skip 0, which is
	 * what scx_bpf_dsq_insert() records for a task in NONE or DISPATCHING.
	 */
	p->scx.ops_qseq = ((p->scx.ops_qseq + 1) &
			   (SCX_OPSS_QSEQ_MASK >> SCX_OPSS_QSEQ_SHIFT)) ?: 1;
	qseq = (unsigned long)p->scx.ops_qseq << SCX_OPSS_QSEQ_SHIFT;

	WARN_ON_ONCE(atomic_long_read(&p->scx.ops_state) != SCX_OPSS_NONE);
	atomic_long_set(&p->scx.ops_state, SCX_OPSS_QUEUEING | qseq);

	ddsp_taskp = this_cpu_ptr(&direct_dispatch_task);
	WARN_ON_ONCE(*ddsp_taskp);
	*ddsp_taskp = p;

	SCX_CALL_OP_TASK(sch, enqueue, rq, p, enq_flags);

	*ddsp_taskp = NULL;
	if (p->scx.ddsp_dsq_id != SCX_DSQ_INVALID)
		goto direct;

	/*
	 * Task is now in BPF scheduler's custody. Set %SCX_TASK_IN_CUSTODY
	 * so ops.dequeue() is called when it leaves custody.
	 */
	p->scx.flags |= SCX_TASK_IN_CUSTODY;

	/*
	 * If not directly dispatched, QUEUEING isn't clear yet and dispatch or
	 * dequeue may be waiting. The store_release matches their load_acquire.
	 */
	atomic_long_set_release(&p->scx.ops_state, SCX_OPSS_QUEUED | qseq);
	goto out;

direct:
	action = ENQ_ACTION_DIRECT;
	goto out;
local_norefill:
	action = ENQ_ACTION_LOCAL_NOREFILL;
	goto out;
local:
	dsq = &rq->scx.local_dsq;
	goto enqueue;
global:
	dsq = find_global_dsq(sch, task_cpu(p));
	goto enqueue;
bypass:
	dsq = bypass_enq_target_dsq(sch, rq, p, &enq_flags);
	goto enqueue;

enqueue:
	action = ENQ_ACTION_ENQUEUE;
out:
	/* The reason is input to ops.enqueue(), not to the resulting placement. */
	p->scx.flags &= ~SCX_TASK_REENQ_REASON_MASK;

	switch (action) {
	case ENQ_ACTION_DIRECT:
		direct_dispatch(sch, p, enq_flags);
		break;
	case ENQ_ACTION_LOCAL_NOREFILL:
		scx_dispatch_enqueue(sch, rq, &rq->scx.local_dsq, p, 0, 0,
				     enq_flags);
		break;
	case ENQ_ACTION_ENQUEUE:
		refill_task_slice_dfl(sch, p);
		clear_direct_dispatch(p);
		scx_dispatch_enqueue(sch, rq, dsq, p, 0, 0, enq_flags);
		break;
	case ENQ_ACTION_NONE:
		break;
	}
}
/* …（同文件无关代码省略）… */
	}

	/*
	 * list_add_tail() must be used. scx_bypass() and rq_offline_scx()
	 * depend on tasks being appended to the runnable_list.
	 */
	list_add_tail(&p->scx.runnable_node, &rq->scx.runnable_list);
/* …（同文件无关代码省略）… */
static void move_remote_task_to_local_dsq(struct scx_sched *sch,
					  struct task_struct *p, u64 enq_flags,
					  struct rq *src_rq, struct rq *dst_rq)
{
	lockdep_assert_rq_held(src_rq);

	/*
	 * Set sticky_cpu before deactivate_task() to properly mark the
	 * beginning of an SCX-internal migration.
	 */
	p->scx.sticky_cpu = cpu_of(dst_rq);
	deactivate_task(src_rq, p, 0);
	set_task_cpu(p, cpu_of(dst_rq));

	switch_rq_lock(src_rq, dst_rq);

	/*
	 * activate_task() below truncates enq_flags to 32 bits and re-derives
	 * @p's owner, dropping our scx flags and the placing @sch. We own @rq,
	 * so stash both across the call. The enqueue reads them back, keeping
	 * the scx flags and checking caps against the placer, not the owner.
	 */
	WARN_ON_ONCE(!cpumask_test_cpu(cpu_of(dst_rq), p->cpus_ptr));
	WARN_ON_ONCE(dst_rq->scx.remote_activate_enq_flags ||
		     dst_rq->scx.remote_activate_sch);
	dst_rq->scx.remote_activate_enq_flags = enq_flags;
	dst_rq->scx.remote_activate_sch = sch;
	activate_task(dst_rq, p, 0);
	dst_rq->scx.remote_activate_enq_flags = 0;
	dst_rq->scx.remote_activate_sch = NULL;
}
/* …（同文件无关代码省略）… */
static bool task_can_run_on_remote_rq(struct scx_sched *sch,
				      struct task_struct *p, struct rq *rq,
				      bool enforce)
{
	s32 cpu = cpu_of(rq);

	if (enforce)
		lockdep_assert_rq_held(task_rq(p));

	WARN_ON_ONCE(task_cpu(p) == cpu);

	/*
	 * proxy_set_task_cpu() preserves wake_cpu when moving a donor's scheduling
	 * context to its lock owner's rq. For example, if BPF places donor D on
	 * CPU0 while its lock owner runs on CPU1, proxy execution moves D to CPU1
	 * but leaves D->wake_cpu pointing to CPU0. If D is later put on a shared
	 * DSQ, CPU0 could otherwise consume it and move it back, only for proxy
	 * execution to return it to CPU1 again.
	 *
	 * When task_cpu() == wake_cpu, the donor has not been proxy-migrated and
	 * BPF may still choose its placement. An actively donating task is
	 * rejected separately by task_proxy_running_or_donating().
	 */
	if (sched_proxy_exec() && p->is_blocked && task_cpu(p) != p->wake_cpu)
		return false;

	/*
	 * If @p has migration disabled, @p->cpus_ptr is updated to contain only
	 * the pinned CPU in migrate_disable_switch() while @p is being switched
	 * out. However, put_prev_task_scx() is called before @p->cpus_ptr is
	 * updated and thus another CPU may see @p on a DSQ inbetween leading to
	 * @p passing the below task_allowed_on_cpu() check while migration is
	 * disabled.
	 *
	 * Test the migration disabled state first as the race window is narrow
	 * and the BPF scheduler failing to check migration disabled state can
	 * easily be masked if task_allowed_on_cpu() is done first.
	 */
	if (unlikely(is_migration_disabled(p))) {
		if (enforce)
			scx_error(sch, "SCX_DSQ_LOCAL[_ON] cannot move migration disabled %s[%d] from CPU %d to %d",
				  p->comm, p->pid, task_cpu(p), cpu);
		return false;
	}

	/*
	 * We don't require the BPF scheduler to avoid dispatching to offline
	 * CPUs mostly for convenience but also because CPUs can go offline
	 * between scx_bpf_dsq_insert() calls and here. Trigger error iff the
	 * picked CPU is outside the allowed mask.
	 */
	if (!task_allowed_on_cpu(p, cpu)) {
		if (enforce)
			scx_error(sch, "SCX_DSQ_LOCAL[_ON] target CPU %d not allowed for %s[%d]",
				  cpu, p->comm, p->pid);
		return false;
	}

	if (!scx_rq_online(rq)) {
		if (enforce)
			__scx_add_event(sch, SCX_EV_DISPATCH_LOCAL_DSQ_OFFLINE, 1);
		return false;
	}

	return true;
}
/* …（同文件无关代码省略）… */
static bool task_proxy_unsafe_to_move(struct task_struct *p)
{
	if (!sched_proxy_exec())
		return false;

	return task_proxy_running_or_donating(p) || is_migration_disabled(p);
}
/* …（同文件无关代码省略）… */
static void scx_proxy_reject_task(struct scx_sched *sch, struct rq *rq,
				  struct task_struct *p, u64 enq_flags)
{
	lockdep_assert_rq_held(rq);
	WARN_ON_ONCE(p->scx.flags & SCX_TASK_REENQ_REASON_MASK);

	p->scx.flags |= SCX_TASK_REENQ_PROXY;
	scx_divert_strip_flags(p, &enq_flags);

	scx_dispatch_enqueue(sch, rq, &rq->scx.reject_dsq, p, 0, 0, enq_flags);
}
/* …（同文件无关代码省略）… */
static void dispatch_to_local_dsq(struct scx_sched *sch, struct rq *rq,
				  struct scx_dispatch_q *dst_dsq, struct task_struct *p,
				  u64 slice, u64 vtime, u64 enq_flags)
{
	struct rq *src_rq = task_rq(p);
	struct rq *dst_rq = container_of(dst_dsq, struct rq, scx.local_dsq);
	struct rq *locked_rq = rq;

	/*
	 * We're synchronized against dequeue through DISPATCHING. As @p can't
	 * be dequeued, its task_rq and cpus_allowed are stable too.
	 *
	 * If dispatching to @rq that @p is already on, no lock dancing needed.
	 */
	if (rq == src_rq && rq == dst_rq) {
		scx_dispatch_enqueue(sch, rq, dst_dsq, p, slice, vtime,
				     enq_flags | SCX_ENQ_APPLY_SLICE | SCX_ENQ_CLEAR_OPSS);
		return;
	}

	/*
	 * @p is on a possibly remote @src_rq which we need to lock to move the
	 * task. If dequeue is in progress, it'd be locking @src_rq and waiting
	 * on DISPATCHING, so we can't grab @src_rq lock while holding
	 * DISPATCHING.
	 *
	 * As DISPATCHING guarantees that @p is wholly ours, we can pretend that
	 * we're moving from a DSQ and use the same mechanism - mark the task
	 * under transfer with holding_cpu, release DISPATCHING and then follow
	 * the same protocol. See unlink_dsq_and_switch_rq_lock().
	 */
	p->scx.holding_cpu = raw_smp_processor_id();

	/* store_release ensures that dequeue sees the above */
	atomic_long_set_release(&p->scx.ops_state, SCX_OPSS_NONE);

	/* switch to @src_rq lock */
	if (locked_rq != src_rq) {
		switch_rq_lock(locked_rq, src_rq);
		locked_rq = src_rq;
	}

	/* task_rq couldn't have changed if we're still the holding cpu */
	if (likely(p->scx.holding_cpu == raw_smp_processor_id()) &&
	    !WARN_ON_ONCE(src_rq != task_rq(p))) {
		bool fallback = false;

		/* the handshake is over, see unlink_dsq_and_switch_rq_lock() */
		p->scx.holding_cpu = -1;

		/*
		 * If @p is staying on the same rq, there's no need to go
		 * through the full deactivate/activate cycle. Optimize by
		 * abbreviating move_remote_task_to_local_dsq().
		 */
		if (src_rq == dst_rq) {
			scx_dispatch_enqueue(sch, dst_rq, &dst_rq->scx.local_dsq, p,
					     slice, vtime, enq_flags | SCX_ENQ_APPLY_SLICE);
		} else if (unlikely(task_proxy_unsafe_to_move(p))) {
			fallback = true;
			scx_proxy_reject_task(sch, src_rq, p, enq_flags);
		} else if (unlikely(!task_can_run_on_remote_rq(sch, p, dst_rq, true))) {
			fallback = true;
			scx_dispatch_enqueue(sch, src_rq, find_global_dsq(sch, task_cpu(p)),
					     p, slice, vtime,
					     enq_flags | SCX_ENQ_APPLY_SLICE |
					     SCX_ENQ_GDSQ_FALLBACK);
		} else {
			apply_slice_vtime(p, slice, vtime, enq_flags);
			move_remote_task_to_local_dsq(sch, p, enq_flags, src_rq, dst_rq);
			/* task has been moved to dst_rq, which is now locked */
			locked_rq = dst_rq;
		}

		/* if the destination CPU is idle, wake it up */
		if (!fallback && sched_class_above(p->sched_class,
						      dst_rq->donor->sched_class))
			resched_curr(dst_rq);
	}

	/* switch back to @rq lock */
	if (locked_rq != rq)
		switch_rq_lock(locked_rq, rq);
}
/* …（同文件无关代码省略）… */
		rq->scx.cpu_released = false;
	}

	if (prev->sched_class == &ext_sched_class) {
		update_curr_scx(rq);

/* …（同文件无关代码省略）… */
	}

	verdict = scx_dispatch_sched(root_sch, rq, prev, false);
	if (verdict != SCX_DSP_NONE)
		goto has_tasks;

/* …（同文件无关代码省略）… */
static void task_woken_scx(struct rq *rq, struct task_struct *p)
{
	run_deferred(rq);
}
/* …（同文件无关代码省略）… */
static void set_cpus_allowed_scx(struct task_struct *p,
				 struct affinity_context *ac)
{
	struct scx_sched *sch = scx_task_sched(p);

	set_cpus_allowed_common(p, ac);

	if (task_dead_and_done(p))
		return;

	/*
	 * The effective cpumask is stored in @p->cpus_ptr which may temporarily
	 * differ from the configured one in @p->cpus_mask. Always tell the bpf
	 * scheduler the effective one.
	 *
	 * Fine-grained memory write control is enforced by BPF making the const
	 * designation pointless. Cast it away when calling the operation.
	 *
	 * The cid form receives the initial mask when the task is enabled and
	 * hears about changes only afterwards, see struct scx_enable_args.
	 */
	if (SCX_HAS_OP(sch, set_cpumask) &&
	    (!scx_is_cid_type() || scx_get_task_state(p) == SCX_TASK_ENABLED))
		scx_call_op_set_cpumask(sch, task_rq(p), p, (struct cpumask *)p->cpus_ptr);
}

static void handle_hotplug(struct rq *rq, bool online)
{
	struct scx_sched *sch = scx_root_protected();
	s32 cpu = cpu_of(rq);
	s32 cpu_or_cid = cpu;

	atomic_long_inc(&scx_hotplug_seq);
	scx_update_frozen_cpus(cpu, online);

/* …（同文件无关代码省略）… */
	 * stable here. Note that we can't depend on scx_enabled() test as the
	 * hotplug ops need to be enabled before __scx_enabled is set.
	 */
	if (unlikely(!sch))
		return;

	if (scx_enabled())
		scx_idle_update_selcpu_topology(&sch->ops);
/* …（同文件无关代码省略）… */
			 "cpu %d going %s, exiting scheduler", cpu,
			 online ? "online" : "offline");

	if (online)
		scx_online_ecaps(rq);
}

void scx_rq_activate(struct rq *rq)
{
	handle_hotplug(rq, true);
}
/* …（同文件无关代码省略）… */
	handle_hotplug(rq, false);
}

static void rq_online_scx(struct rq *rq)
{
	rq->scx.flags |= SCX_RQ_ONLINE;
}

static void rq_offline_scx(struct rq *rq)
{
	struct task_struct *p, *n;

	rq->scx.flags &= ~SCX_RQ_ONLINE;

	/* sched domain rebuilds call rq_offline with the CPU staying alive */
	if (cpu_active(cpu_of(rq)))
		return;

	scx_rescue_flush(rq);

	/*
	 * An offline CPU no longer calls ops.dispatch(). Re-enqueue its tasks
	 * onto the local DSQ so that they run here and balance_push() moves
	 * them off.
	 */
	list_for_each_entry_safe_reverse(p, n, &rq->scx.runnable_list, scx.runnable_node) {
		if (p->scx.dsq == &rq->scx.local_dsq)
			continue;
		guard(sched_change)(p, DEQUEUE_SAVE | DEQUEUE_MOVE | DEQUEUE_NOCLOCK);
	}
}

static bool check_rq_for_timeouts(struct rq *rq)
{
	struct scx_sched *sch;
/* …（同文件无关代码省略）… */
static void task_tick_scx(struct rq *rq, struct task_struct *donor, int queued)
{
	struct scx_sched *sch = scx_task_sched(donor);

	update_curr_scx(rq);

	/*
	 * While disabling, always resched as we can't trust the slice
	 * management.
	 */
	if (scx_bypassing(sch, cpu_of(rq)))
		expire_task_slice(donor);
	else if (SCX_HAS_OP(sch, tick))
		SCX_CALL_OP_TASK(sch, tick, rq, donor);

	if (!donor->scx.slice) {
		/* the slice can't be trusted while bypassing */
		if (READ_ONCE(donor->scx.lazy_resched) &&
		    !scx_bypassing(sch, cpu_of(rq)))
			scx_resched_curr_lazy(rq);
		else
			resched_curr(rq);
	}
}
/* …（同文件无关代码省略）… */
static void __scx_enable_task(struct scx_sched *sch, struct task_struct *p)
{
	struct rq *rq = task_rq(p);
	u32 weight;

	lockdep_assert_rq_held(rq);

	/*
	 * Verify the task is not in BPF scheduler's custody. If flag
	 * transitions are consistent, the flag should always be clear
	 * here.
	 */
	WARN_ON_ONCE(p->scx.flags & SCX_TASK_IN_CUSTODY);

	/*
	 * Set the weight before calling ops.enable() so that the scheduler
	 * doesn't see a stale value if they inspect the task struct.
	 */
	if (task_has_idle_policy(p))
		weight = WEIGHT_IDLEPRIO;
	else
		weight = sched_prio_to_weight[p->static_prio - MAX_RT_PRIO];

	p->scx.weight = sched_weight_to_cgroup(weight);
	p->scx.lazy_resched = sch->ops.flags & SCX_OPS_LAZY_RESCHED;

	if (SCX_HAS_OP(sch, enable)) {
		if (scx_is_cid_type()) {
			struct scx_cmask *cmask = scx_fill_cmask_scratch(sch, p->cpus_ptr);
			struct scx_enable_args args = {
				.cmask_arena_addr = scx_kaddr_to_arena(sch, cmask),
			};

			SCX_CALL_CID_OP_TASK(sch, enable, rq, p, &args);
		} else {
			SCX_CALL_OP_TASK(sch, enable, rq, p);
		}
	}

	/*
	 * The initial mask also goes out through set_cmask() so a scheduler can
	 * track affinity there alone, and before set_weight() so that the mask
	 * is in place when weight-dependent state is derived, see struct
	 * scx_enable_args.
	 */
	if (scx_is_cid_type() && SCX_HAS_OP(sch, set_cpumask))
		scx_call_op_set_cpumask(sch, rq, p, p->cpus_ptr);

	if (SCX_HAS_OP(sch, set_weight))
		SCX_CALL_OP_TASK(sch, set_weight, rq, p, p->scx.weight);
}
/* …（同文件无关代码省略）… */
void scx_enable_task(struct scx_sched *sch, struct task_struct *p)
{
	__scx_enable_task(sch, p);
	scx_set_task_state(p, SCX_TASK_ENABLED);
}
/* …（同文件无关代码省略）… */
static void scx_disable_task(struct scx_sched *sch, struct task_struct *p)
{
	struct rq *rq = task_rq(p);

	lockdep_assert_rq_held(rq);
	WARN_ON_ONCE(scx_get_task_state(p) != SCX_TASK_ENABLED);

	clear_direct_dispatch(p);

	if (SCX_HAS_OP(sch, disable))
		SCX_CALL_OP_TASK(sch, disable, rq, p);
	scx_set_task_state(p, SCX_TASK_READY);

	/*
	 * Verify the task is not in BPF scheduler's custody. If flag
	 * transitions are consistent, the flag should always be clear
	 * here.
	 */
	WARN_ON_ONCE(p->scx.flags & SCX_TASK_IN_CUSTODY);
}
/* …（同文件无关代码省略）… */
		args.cancelled = true;
		break;
	case SCX_TASK_READY:
		break;
	case SCX_TASK_ENABLED:
		scx_disable_task(sch, p);
/* …（同文件无关代码省略）… */
static bool task_dead_and_done(struct task_struct *p)
{
	struct rq *rq = task_rq(p);

	lockdep_assert_rq_held(rq);

	/*
	 * In do_task_dead(), a dying task sets %TASK_DEAD with preemption
	 * disabled and __schedule(). If @p has %TASK_DEAD set and off CPU, @p
	 * won't ever run again.
	 */
	return unlikely(READ_ONCE(p->__state) == TASK_DEAD) &&
		!task_on_cpu(rq, p);
}
/* …（同文件无关代码省略）… */
static void switching_to_scx(struct rq *rq, struct task_struct *p)
{
	struct scx_sched *sch = scx_task_sched(p);

	if (task_dead_and_done(p))
		return;

	scx_enable_task(sch, p);

	/*
	 * set_cpus_allowed_scx() is not called while @p is associated with a
	 * different scheduler class. Keep the BPF scheduler up-to-date. The cid
	 * form gets its mask from scx_enable_task().
	 */
	if (!scx_is_cid_type() && SCX_HAS_OP(sch, set_cpumask))
		scx_call_op_set_cpumask(sch, rq, p, (struct cpumask *)p->cpus_ptr);
}
/* …（同文件无关代码省略）… */
static void process_ddsp_deferred_locals(struct rq *rq)
{
	struct task_struct *p;

	lockdep_assert_rq_held(rq);

	/*
	 * Now that @rq can be unlocked, execute the deferred enqueueing of
	 * tasks directly dispatched to the local DSQs of other CPUs. See
	 * direct_dispatch(). Keep popping from the head instead of using
	 * list_for_each_entry_safe() as dispatch_local_dsq() may unlock @rq
	 * temporarily.
	 */
	while ((p = list_first_entry_or_null(&rq->scx.ddsp_deferred_locals,
				struct task_struct, scx.dsq_list.node))) {
		struct scx_sched *sch = scx_task_sched(p);
		struct scx_dispatch_q *dsq;
		u64 dsq_id = p->scx.ddsp_dsq_id;
		u64 enq_flags = p->scx.ddsp_enq_flags;
		u64 slice = p->scx.ddsp_slice;
		u64 vtime = p->scx.ddsp_vtime;

		list_del_init(&p->scx.dsq_list.node);
		clear_direct_dispatch(p);

		dsq = find_dsq_for_dispatch(sch, rq, dsq_id, task_cpu(p));
		if (!WARN_ON_ONCE(dsq->id != SCX_DSQ_LOCAL))
			dispatch_to_local_dsq(sch, rq, dsq, p, slice, vtime, enq_flags);
	}
}
/* …（同文件无关代码省略）… */
static bool local_task_should_reenq(struct rq *rq, struct task_struct *p,
				    u64 *reenq_flags, u32 *reason)
{
	bool first;

	first = !(*reenq_flags & SCX_REENQ_TSR_NOT_FIRST);
	*reenq_flags |= SCX_REENQ_TSR_NOT_FIRST;

	if (unlikely((p->scx.flags & SCX_TASK_PROTECTED) || scx_task_has_kernel_slice(p)))
		return false;

	*reason = SCX_TASK_REENQ_KFUNC;

	if ((p->scx.flags & SCX_TASK_IMMED) &&
	    (!first || !(*reenq_flags & SCX_REENQ_TSR_RQ_OPEN))) {
		__scx_add_event(scx_task_sched(p), SCX_EV_REENQ_IMMED, 1);
		*reason = SCX_TASK_REENQ_IMMED;
		return true;
	}

	if ((*reenq_flags & SCX_REENQ_CAP_REVOKE) &&
	    scx_task_reenq_on_cap_revoke(rq, p)) {
		*reason = SCX_TASK_REENQ_CAP;
		return true;
	}

	return *reenq_flags & SCX_REENQ_ANY;
}
/* …（同文件无关代码省略）… */
void scx_wait_dispatching(struct task_struct *p)
{
	if (unlikely(atomic_long_read_acquire(&p->scx.ops_state) == SCX_OPSS_DISPATCHING))
		wait_ops_state(p, SCX_OPSS_DISPATCHING);
}
/* …（同文件无关代码省略）… */
static u32 reenq_local(struct scx_sched *sch, struct rq *rq, u64 reenq_flags)
{
	LIST_HEAD(tasks);
	u32 nr_enqueued = 0;
	struct task_struct *p, *n;

	lockdep_assert_rq_held(rq);

	if (WARN_ON_ONCE(reenq_flags & __SCX_REENQ_TSR_MASK))
		reenq_flags &= ~__SCX_REENQ_TSR_MASK;
	if (rq_is_open(rq, 0))
		reenq_flags |= SCX_REENQ_TSR_RQ_OPEN;

	/*
	 * The BPF scheduler may choose to dispatch tasks back to
	 * @rq->scx.local_dsq. Move all candidate tasks off to a private list
	 * first to avoid processing the same tasks repeatedly.
	 */
	list_for_each_entry_safe(p, n, &rq->scx.local_dsq.list,
				 scx.dsq_list.node) {
		struct scx_sched *task_sch = scx_task_sched(p);
		u32 reason;

		/*
		 * If @p is being migrated, @p's current CPU may not agree with
		 * its allowed CPUs and the migration_cpu_stop is about to
		 * deactivate and re-activate @p anyway. Skip re-enqueueing.
		 *
		 * While racing sched property changes may also dequeue and
		 * re-enqueue a migrating task while its current CPU and allowed
		 * CPUs disagree, they use %ENQUEUE_RESTORE which is bypassed to
		 * the current local DSQ for running tasks and thus are not
		 * visible to the BPF scheduler.
		 */
		if (p->migration_pending)
			continue;

		if (!scx_is_descendant(task_sch, sch))
			continue;

		if (!local_task_should_reenq(rq, p, &reenq_flags, &reason))
			continue;

		scx_wait_dispatching(p);
		scx_dispatch_dequeue(rq, p);

		WARN_ON_ONCE(p->scx.flags & SCX_TASK_REENQ_REASON_MASK);
		p->scx.flags |= reason;

		list_add_tail(&p->scx.dsq_list.node, &tasks);
	}

	list_for_each_entry_safe(p, n, &tasks, scx.dsq_list.node) {
		list_del_init(&p->scx.dsq_list.node);

		scx_do_enqueue_task(rq, p, SCX_ENQ_REENQ, -1);

		nr_enqueued++;
	}

	/*
	 * The revoke that scheduled this scan may have raced the pick: donor
	 * may be a now-capless task, either one that kept running or one
	 * promoted off the local DSQ between the ecaps sync and this scan.
	 * Zero the slice to evict it. The enqueue gate blocks new capless
	 * inserts, so no later pick can slip through after the scan.
	 */
	if ((reenq_flags & SCX_REENQ_CAP_REVOKE) &&
	    rq->donor->sched_class == &ext_sched_class &&
	    scx_task_reenq_on_cap_revoke(rq, rq->donor)) {
		expire_task_slice(rq->donor);
		resched_curr(rq);
	}

	return nr_enqueued;
}
/* …（同文件无关代码省略）… */
static void process_deferred_reenq_locals(struct rq *rq)
{
	lockdep_assert_rq_held(rq);

	/*
	 * A task can be re-queued within this loop when a reenqueued task
	 * bounces straight back to the local DSQ. That recursion is bounded by
	 * the per-task reenqueue cap in scx_do_enqueue_task().
	 */
	while (true) {
		struct scx_sched *sch;
		u64 reenq_flags;

		scoped_guard (raw_spinlock, &rq->scx.deferred_reenq_lock) {
			struct scx_deferred_reenq_local *drl =
				list_first_entry_or_null(&rq->scx.deferred_reenq_locals,
							 struct scx_deferred_reenq_local,
							 node);
			struct scx_sched_pcpu *sch_pcpu;

			if (!drl)
				return;

			sch_pcpu = container_of(drl, struct scx_sched_pcpu,
						deferred_reenq_local);
			sch = sch_pcpu->sch;

			reenq_flags = drl->flags;
			WRITE_ONCE(drl->flags, 0);
			list_del_init(&drl->node);
		}

		/* see schedule_dsq_reenq() */
		smp_mb();

		reenq_local(sch, rq, reenq_flags);
	}
}
/* …（同文件无关代码省略）… */
static bool user_task_should_reenq(struct task_struct *p, u64 reenq_flags, u32 *reason)
{
	*reason = SCX_TASK_REENQ_KFUNC;
	if (reenq_flags & SCX_REENQ_ANY)
		return true;
	return (reenq_flags & SCX_REENQ_MARKED) && (p->scx.dsq_flags & SCX_TASK_DSQ_REENQ);
}
/* …（同文件无关代码省略）… */
static void reenq_user(struct rq *rq, struct scx_dispatch_q *dsq, u64 reenq_flags)
{
	struct rq *locked_rq = rq;
	struct scx_sched *sch = dsq->sched;
	struct scx_dsq_list_node cursor = INIT_DSQ_LIST_CURSOR(cursor, dsq, 0);
	struct task_struct *p;
	s32 nr_visited = 0;

	lockdep_assert_rq_held(rq);

	raw_spin_lock(&dsq->lock);

	while (likely(!READ_ONCE(sch->bypass_depth))) {
		struct rq *task_rq;
		u32 reason;

		/*
		 * Count skipped tasks too so that a long walk doesn't hold the
		 * locks throughout. The cursor keeps the position.
		 */
		if (!(++nr_visited % SCX_TASK_ITER_BATCH)) {
			raw_spin_unlock(&dsq->lock);
			if (locked_rq) {
				scx_rq_lock_drop(locked_rq);
				raw_spin_rq_unlock(locked_rq);
				locked_rq = NULL;
			}
			cpu_relax();
			raw_spin_lock(&dsq->lock);
		}

		p = nldsq_cursor_next_task(&cursor, dsq);
		if (!p)
			break;

		if (!user_task_should_reenq(p, reenq_flags, &reason))
			continue;

		task_rq = task_rq(p);

		if (locked_rq != task_rq) {
			if (locked_rq) {
				scx_rq_lock_drop(locked_rq);
				raw_spin_rq_unlock(locked_rq);
			}
			if (unlikely(!raw_spin_rq_trylock(task_rq))) {
				raw_spin_unlock(&dsq->lock);
				raw_spin_rq_lock(task_rq);
				raw_spin_lock(&dsq->lock);
			}
			locked_rq = task_rq;

			/* did we lose @p while switching locks? */
			if (nldsq_cursor_lost_task(&cursor, task_rq, dsq, p))
				continue;
		}

		/* @p is on @dsq, its rq and @dsq are locked */
		scx_wait_dispatching(p);
		dispatch_dequeue_locked(p, dsq);
		raw_spin_unlock(&dsq->lock);

		WARN_ON_ONCE(p->scx.flags & SCX_TASK_REENQ_REASON_MASK);
		p->scx.flags |= reason;

		scx_do_enqueue_task(task_rq, p, SCX_ENQ_REENQ, -1);

		raw_spin_lock(&dsq->lock);
	}

	list_del_init(&cursor.node);
	raw_spin_unlock(&dsq->lock);

	if (locked_rq != rq) {
		if (locked_rq) {
			scx_rq_lock_drop(locked_rq);
			raw_spin_rq_unlock(locked_rq);
		}
		raw_spin_rq_lock(rq);
	}
}
/* …（同文件无关代码省略）… */
static void process_deferred_reenq_users(struct rq *rq)
{
	lockdep_assert_rq_held(rq);

	while (true) {
		struct scx_dispatch_q *dsq;
		u64 dsq_id, reenq_flags;

		scoped_guard (raw_spinlock, &rq->scx.deferred_reenq_lock) {
			struct scx_deferred_reenq_user *dru =
				list_first_entry_or_null(&rq->scx.deferred_reenq_users,
							 struct scx_deferred_reenq_user,
							 node);
			struct scx_dsq_pcpu *dsq_pcpu;

			if (!dru)
				return;

			dsq_pcpu = container_of(dru, struct scx_dsq_pcpu,
						deferred_reenq_user);
			dsq = dsq_pcpu->dsq;
			reenq_flags = dru->flags;
			WRITE_ONCE(dru->flags, 0);
			list_del_init(&dru->node);
		}

		/* see schedule_dsq_reenq() */
		smp_mb();

		/* destroy_dsq() may have raced and invalidated @dsq, nothing to reenq */
		dsq_id = READ_ONCE(dsq->id);
		if (unlikely(dsq_id == SCX_DSQ_INVALID))
			continue;

		BUG_ON(dsq_id & SCX_DSQ_FLAG_BUILTIN);
		reenq_user(rq, dsq, reenq_flags);
	}
}
/* …（同文件无关代码省略）… */
static void scx_reenq_reject(struct rq *rq)
{
	LIST_HEAD(tasks);
	struct task_struct *p, *n;

	lockdep_assert_rq_held(rq);

	if (list_empty(&rq->scx.reject_dsq.list))
		return;

	/*
	 * Move ready tasks to a private list so a task re-rejected by
	 * scx_do_enqueue_task() below isn't revisited this round.
	 */
	list_for_each_entry_safe(p, n, &rq->scx.reject_dsq.list, scx.dsq_list.node) {
		u32 reason = p->scx.flags & SCX_TASK_REENQ_REASON_MASK;

		WARN_ON_ONCE(!reason);

		if (sched_proxy_exec() && reason == SCX_TASK_REENQ_PROXY) {
			/* Affinity machinery will dequeue and reactivate @p. */
			if (p->migration_pending)
				continue;

			if (task_on_cpu(rq, p) || task_current_donor(rq, p)) {
				rq->scx.flags |= SCX_RQ_PROXY_RETRY;
				continue;
			}
		} else {
			WARN_ON_ONCE(p->migration_pending);
		}

		scx_wait_dispatching(p);
		scx_dispatch_dequeue(rq, p);

		list_add_tail(&p->scx.dsq_list.node, &tasks);
	}

	list_for_each_entry_safe(p, n, &tasks, scx.dsq_list.node) {
		list_del_init(&p->scx.dsq_list.node);

		scx_do_enqueue_task(rq, p, SCX_ENQ_REENQ, -1);
	}
}
/* …（同文件无关代码省略）… */
static void run_deferred(struct rq *rq)
{
	process_ddsp_deferred_locals(rq);

	if (!list_empty(&rq->scx.deferred_reenq_locals))
		process_deferred_reenq_locals(rq);

	if (!list_empty(&rq->scx.deferred_reenq_users))
		process_deferred_reenq_users(rq);

	scx_reenq_reject(rq);
}
/* …（同文件无关代码省略）… */
	.task_woken		= task_woken_scx,
	.set_cpus_allowed	= set_cpus_allowed_scx,

	.rq_online		= rq_online_scx,
	.rq_offline		= rq_offline_scx,

	.task_tick		= task_tick_scx,

	.switching_to		= switching_to_scx,
/* …（同文件无关代码省略）… */
{
	int kind;

	do {
		irq_work_sync(&sch->disable_irq_work);
		kthread_flush_work(&sch->disable_work);
		kind = atomic_read(&sch->exit_kind);
	} while (kind != SCX_EXIT_NONE && kind != SCX_EXIT_DONE);
}

/* …（同文件无关代码省略）… */
__bpf_kfunc struct task_struct *bpf_iter_scx_dsq_next(struct bpf_iter_scx_dsq *it)
{
	struct bpf_iter_scx_dsq_kern *kit = (void *)it;

	if (!kit->dsq)
		return NULL;

	guard(raw_spinlock_irqsave)(&kit->dsq->lock);

	return nldsq_cursor_next_task(&kit->cursor, kit->dsq);
}

/**
/* …（同文件无关代码省略）… */
/**
 * scx_bpf_dsq_reenq - Re-enqueue tasks on a DSQ
 * @dsq_id: DSQ to re-enqueue
 * @reenq_flags: %SCX_RENQ_*
 * @aux: implicit BPF argument to access bpf_prog_aux hidden from BPF progs
 *
 * Iterate over all of the tasks currently enqueued on the DSQ identified by
/* …（同文件无关代码省略）… */
 * - Local DSQs (%SCX_DSQ_LOCAL or %SCX_DSQ_LOCAL_ON | $cpu)
 * - User DSQs
 *
 * Re-enqueues are performed asynchronously. Can be called from anywhere. A
 * re-enqueue of an inactive cpu's local DSQ is ignored.
 *
 * %SCX_DSQ_LOCAL resolves to the local DSQ of the rq the current scheduler
 * operation is locked to - e.g. the rq being dispatched for in ops.dispatch() -
/* …（同文件无关代码省略）… */
 * scx_bpf_reenqueue_local___v2 - Re-enqueue tasks on a local DSQ
 * @aux: implicit BPF argument to access bpf_prog_aux hidden from BPF progs
 *
 * Iterate over all of the tasks currently enqueued on the local DSQ of the
 * caller's CPU, and re-enqueue them in the BPF scheduler. Can be called from
 * anywhere.
 *
 * This is now a special case of scx_bpf_dsq_reenq() and may be removed in the
 * future.
/* …（同文件无关代码省略）… */
static s32 scx_cpuperf_set(struct scx_sched *sch, s32 cpu, u32 perf)
{
	struct rq *rq, *locked_rq;
	struct rq_flags rf;
	s32 ret;

	if (unlikely(perf > SCX_CPUPERF_ONE)) {
		scx_error(sch, "Invalid cpuperf target %u for CPU %d", perf, cpu);
		return -EINVAL;
	}

	if (!scx_cpu_valid(sch, cpu, NULL))
		return -EINVAL;

	rq = cpu_rq(cpu);
	locked_rq = scx_locked_rq();

	/*
	 * When called with an rq lock held, restrict the operation to the
	 * corresponding CPU to prevent ABBA deadlocks.
	 */
	if (locked_rq && rq != locked_rq) {
		scx_error(sch, "Invalid target CPU %d", cpu);
		return -EINVAL;
	}

	/*
	 * If no rq lock is held, allow to operate on any CPU by acquiring
	 * the corresponding rq lock.
	 */
	if (!locked_rq) {
		rq_lock_irqsave(rq, &rf);
		update_rq_clock(rq);
	}

	/*
	 * ecaps updates are folded under the rq lock, making this test
	 * authoritative: a write can never land after a revoke has taken
	 * effect on @cpu.
	 */
	if (likely(!scx_missing_caps(sch, cpu, SCX_CAP_PERF))) {
		rq->scx.cpuperf_target = perf;
		cpufreq_update_util(rq, 0);
		ret = 0;
	} else {
		__scx_add_event(sch, SCX_EV_SUB_CIDPERF_DENIED, 1);
		ret = -EACCES;
	}

	if (!locked_rq)
		rq_unlock_irqrestore(rq, &rf);

	return ret;
}
/* …（同文件无关代码省略）… */
__bpf_kfunc s32 scx_bpf_cidperf_set(s32 cid, u32 perf,
				    const struct bpf_prog_aux *aux)
{
	struct scx_sched *sch;
	s32 cpu;

	guard(rcu)();

	sch = scx_prog_sched(aux);
	if (unlikely(!sch))
		return -ENODEV;

	if (!scx_kf_allowed_ctx(sch))
		return -EDEADLK;

	cpu = scx_cid_to_cpu(sch, cid);
	if (cpu < 0)
		return cpu;

	return scx_cpuperf_set(sch, cpu, perf);
}
/* …（同文件无关代码省略）… */
 * when scx_locked_rq() returns NULL. Taking it while the context already holds
 * an rq lock, the same one or another cpu's, can deadlock.
 *
 * Before returning, scx_bpf_sub_grant() and scx_bpf_sub_revoke() invoke
 * ops.sub_caps_updated() of the sub-schedulers whose caps changed, so those ops
 * run in the tracing program's context. No SCX op expects to run from an
/* …（同文件无关代码省略）… */
BTF_ID_FLAGS(func, scx_bpf_cidperf_set, KF_IMPLICIT_ARGS)
BTF_ID_FLAGS(func, scx_bpf_sub_grant, KF_IMPLICIT_ARGS)
BTF_ID_FLAGS(func, scx_bpf_sub_revoke, KF_IMPLICIT_ARGS)
BTF_KFUNCS_END(scx_kfunc_ids_no_tracing)

/*
