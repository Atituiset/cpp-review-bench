// AUTO-DRAFT from torvalds/linux PR #762122d75e50e4919ef77afc2dffdc4931c5be87
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
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
static struct scx_dispatch_q *bypass_enq_target_dsq(struct scx_sched *sch, s32 cpu)
{
#ifdef CONFIG_EXT_SUB_SCHED
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
	 */
	while (scx_parent(sch) && scx_bypassing(sch, cpu))
		sch = scx_parent(sch);
#endif	/* CONFIG_EXT_SUB_SCHED */

	return scx_bypass_dsq(sch, cpu);
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
	 * If we're in the dispatch path holding rq lock, $curr may or may not
	 * be ready depending on whether the on-going dispatch decides to extend
	 * $curr's slice. We say yes here and resolve it at the end of dispatch.
	 * See dispatch_one().
	 */
	if (rq->scx.flags & SCX_RQ_IN_DISPATCH)
		return true;

	/*
	 * %SCX_ENQ_PREEMPT clears $curr's slice if on SCX and kicks dispatch,
	 * so allow it to avoid spuriously triggering reenq on a combined
	 * PREEMPT|IMMED insertion.
	 */
	if (enq_flags & SCX_ENQ_PREEMPT) {
		struct task_struct *curr = rq->curr;

		/*
		 * A protected slice refuses the preemption and the cpu stays
		 * occupied. See rq_owned_post_enq().
		 */
		return curr->sched_class != &ext_sched_class ||
			likely(!(curr->scx.flags & SCX_TASK_PROTECTED));
	}

	/*
	 * @rq is either in transition to or running an SCX task and can't go
	 * idle without another SCX dispatch cycle.
	 */
	return false;
}
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
static void schedule_deferred(struct rq *rq)
{
	/*
	 * This is the fallback when schedule_deferred_locked() can't use
	 * the cheaper balance callback or wakeup hook paths (the target
	 * CPU is not in dispatch or wakeup). Currently, this is primarily
	 * hit by reenqueue operations targeting a remote CPU.
	 *
	 * Queue on the target CPU. The deferred work can run from any CPU
	 * correctly - the _locked() path already processes remote rqs from
	 * the calling CPU - but targeting the owning CPU allows IPI delivery
	 * without waiting for the calling CPU to re-enable IRQs and is
	 * cheaper as the reenqueue runs locally.
	 */
	irq_work_queue_on(&rq->scx.deferred_irq_work, cpu_of(rq));
}
/* …（同文件无关代码省略）… */
static void schedule_deferred_locked(struct rq *rq)
{
	lockdep_assert_rq_held(rq);

	/*
	 * If in the middle of waking up a task, task_woken_scx() will be called
	 * afterwards which will then run the deferred actions, no need to
	 * schedule anything.
	 */
	if (rq->scx.flags & SCX_RQ_IN_WAKEUP)
		return;

	/* Don't do anything if there already is a deferred operation. */
	if (rq->scx.flags & SCX_RQ_BAL_CB_PENDING)
		return;

	/*
	 * If in dispatch, the balance callbacks will be called before rq lock
	 * is released. Schedule one.
	 *
	 *
	 * We can't directly insert the callback into the
	 * rq's list: The call can drop its lock and make the pending balance
	 * callback visible to unrelated code paths that call rq_pin_lock().
	 *
	 * Just let dispatch_one() know that it must do it itself.
	 */
	if (rq->scx.flags & SCX_RQ_IN_DISPATCH) {
		rq->scx.flags |= SCX_RQ_BAL_CB_PENDING;
		return;
	}

	/*
	 * No scheduler hooks available. Use the generic irq_work path. The
	 * above WAKEUP and DISPATCH paths should cover most of the cases and
	 * the time to IRQ re-enable shouldn't be long.
	 */
	schedule_deferred(rq);
}
/* …（同文件无关代码省略）… */
static void clear_task_slice_oob(struct task_struct *p)
{
	if (unlikely(atomic64_read(&p->scx.slice_oob)))
		atomic64_set(&p->scx.slice_oob, 0);
}
/* …（同文件无关代码省略）… */
static bool dsq_insert_head(struct scx_dispatch_q *dsq, struct task_struct *p)
{
	struct list_head *pos = &dsq->list;
	struct scx_dsq_list_node *node;

	/*
	 * Only rq-owned DSQs can hold protected tasks and the associated rq
	 * lock keeps their flags stable.
	 */
	if (!dsq_is_rq_owned(dsq)) {
		list_add(&p->scx.dsq_list.node, &dsq->list);
		return true;
	}

	list_for_each_entry(node, &dsq->list, node) {
		struct task_struct *q;

		if (WARN_ON_ONCE(node->flags & SCX_DSQ_LNODE_ITER_CURSOR))
			continue;

		q = container_of(node, struct task_struct, scx.dsq_list);
		if (!(q->scx.flags & SCX_TASK_PROTECTED))
			break;

		pos = &node->node;
	}

	list_add(&p->scx.dsq_list.node, pos);

	return pos == &dsq->list;
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
static bool scx_dsq_priq_less(struct rb_node *node_a,
			      const struct rb_node *node_b)
{
	const struct task_struct *a =
		container_of(node_a, struct task_struct, scx.dsq_priq);
	const struct task_struct *b =
		container_of(node_b, struct task_struct, scx.dsq_priq);

	return time_before64(a->scx.dsq_vtime, b->scx.dsq_vtime);
}
/* …（同文件无关代码省略）… */
static void dsq_inc_nr(struct scx_dispatch_q *dsq, struct task_struct *p, u64 enq_flags)
{
	/* scx_bpf_dsq_nr_queued() reads ->nr without locking, use WRITE_ONCE() */
	WRITE_ONCE(dsq->nr, dsq->nr + 1);

	/*
	 * Once @p reaches a local DSQ, it can only leave it by being dispatched
	 * to the CPU or dequeued. In both cases, the only way @p can go back to
	 * the BPF sched is through enqueueing. If being inserted into a local
	 * DSQ with IMMED, persist the state until the next enqueueing event in
	 * scx_do_enqueue_task() so that we can maintain IMMED protection
	 * through e.g. SAVE/RESTORE cycles and slice extensions.
	 */
	if (enq_flags & SCX_ENQ_IMMED) {
		if (unlikely(dsq->id != SCX_DSQ_LOCAL)) {
			WARN_ON_ONCE(!(enq_flags & SCX_ENQ_GDSQ_FALLBACK));
			return;
		}
		p->scx.flags |= SCX_TASK_IMMED;
	}

	if (p->scx.flags & SCX_TASK_IMMED) {
		struct rq *rq = container_of(dsq, struct rq, scx.local_dsq);

		if (WARN_ON_ONCE(dsq->id != SCX_DSQ_LOCAL))
			return;

		rq->scx.nr_immed++;

		/*
		 * If @rq already had other tasks or the current task is not
		 * done yet, @p can't go on the CPU immediately. Re-enqueue.
		 */
		if (unlikely(dsq->nr > 1 || !rq_is_open(rq, enq_flags)))
			scx_schedule_reenq_local(rq, 0);
	}
}
/* …（同文件无关代码省略）… */
static void refill_task_slice_dfl(struct scx_sched *sch, struct task_struct *p)
{
	/*
	 * A default refill is not an explicit request, so it must not drop a
	 * pending out-of-band one, which is applied when @p next runs.
	 */
	set_task_slice_keep_oob(p, READ_ONCE(sch->slice_dfl));
	__scx_add_event(sch, SCX_EV_REFILL_SLICE_DFL, 1);
}
/* …（同文件无关代码省略）… */
static inline bool task_scx_migrating(struct task_struct *p)
{
	/*
	 * We only need to check sticky_cpu: it is set to the destination
	 * CPU in move_remote_task_to_local_dsq() before deactivate_task()
	 * and cleared when the task is enqueued on the destination, so it
	 * is only non-negative during an internal SCX migration.
	 */
	return p->scx.sticky_cpu >= 0;
}
/* …（同文件无关代码省略）… */
static bool task_leave_custody(struct task_struct *p)
{
	if (!(p->scx.flags & SCX_TASK_IN_CUSTODY) || task_scx_migrating(p))
		return false;

	p->scx.flags &= ~SCX_TASK_IN_CUSTODY;
	return true;
}
/* …（同文件无关代码省略）… */
static void rq_owned_post_enq(struct scx_sched *sch, struct rq *rq,
			      struct scx_dispatch_q *dsq, struct task_struct *p,
			      u64 enq_flags)
{
	if (task_leave_custody(p) && SCX_HAS_OP(sch, dequeue))
		SCX_CALL_OP_TASK(sch, dequeue, rq, p, 0);

	/*
	 * Only local inserts get the wakeup treatment below. Rejects kick the
	 * deferred reenq and rescue parks are paced by the rescue timer.
	 */
	if (unlikely(dsq->id != SCX_DSQ_LOCAL)) {
		if (dsq->id == SCX_DSQ_REJECT)
			schedule_deferred_locked(rq);
		return;
	}

	/*
	 * Note that @rq's lock may be dropped between this enqueue and @p
	 * actually getting on CPU. This gives higher-class tasks (e.g. RT)
	 * an opportunity to wake up on @rq and prevent @p from running.
	 * Here are some concrete examples:
	 *
	 * Example 1:
	 *
	 * We dispatch two tasks from a single ops.dispatch():
	 * - First, a local task to this CPU's local DSQ;
	 * - Second, a local/remote task to a remote CPU's local DSQ.
	 * We must drop the local rq lock in order to finish the second
	 * dispatch. In that time, an RT task can wake up on the local rq.
	 *
	 * Example 2:
	 *
	 * We dispatch a local/remote task to a remote CPU's local DSQ.
	 * We must drop the remote rq lock before the dispatched task can run,
	 * which gives an RT task an opportunity to wake up on the remote rq.
	 *
	 * Both examples work the same if we replace dispatching with moving
	 * the tasks from a user-created DSQ.
	 *
	 * We must detect these wakeups so that we can re-enqueue IMMED tasks
	 * from @rq's local DSQ. scx_wakeup_preempt() serves exactly this
	 * purpose, but for it to be invoked, we must ensure that we bump
	 * @rq->next_class to &ext_sched_class if it's currently idle.
	 *
	 * wakeup_preempt() does the bumping, and since we only invoke it if
	 * @rq->next_class is below &ext_sched_class, it will also
	 * resched_curr(rq).
	 */
	if (sched_class_above(p->sched_class, rq->next_class))
		wakeup_preempt(rq, p, 0);

	/*
	 * If @rq is in dispatch, the CPU is already vacant and looking for the
	 * next task to run. No need to preempt or trigger resched after moving
	 * @p into its local DSQ.
	 * Note that the wakeup_preempt() above may have already triggered
	 * a resched if @rq->next_class was idle. It's harmless, since
	 * need_resched is cleared immediately after task pick.
	 */
	if (rq->scx.flags & SCX_RQ_IN_DISPATCH)
		return;

	if ((enq_flags & SCX_ENQ_PREEMPT) && p != rq->curr &&
	    rq->curr->sched_class == &ext_sched_class) {
		if (likely(scx_set_task_slice(rq->curr, 0)))
			resched_curr(rq);
		else
			__scx_add_event(sch, SCX_EV_SLICE_DENIED, 1);
	}
}
/* …（同文件无关代码省略）… */
static void scx_dispatch_enqueue(struct scx_sched *sch, struct rq *rq,
				 struct scx_dispatch_q *dsq, struct task_struct *p,
				 u64 slice, u64 vtime, u64 enq_flags)
{
	bool is_rq_owned = false;

	if (dsq->id == SCX_DSQ_LOCAL) {
		dsq = scx_resolve_local_dsq(sch, rq, p, &enq_flags);
		is_rq_owned = true;
	}

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

		if (enq_flags & (SCX_ENQ_HEAD | SCX_ENQ_PREEMPT)) {
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
	 * Test both cpu_active() and %SCX_RQ_ONLINE. %SCX_RQ_ONLINE indicates
	 * the online state as seen from the BPF scheduler. cpu_active() test
	 * guarantees that, if this function returns %true, %SCX_RQ_ONLINE will
	 * stay set until the current scheduling operation is complete even if
	 * we aren't locking @rq.
	 */
	return likely((rq->scx.flags & SCX_RQ_ONLINE) && cpu_active(cpu_of(rq)));
}
/* …（同文件无关代码省略）… */
void scx_do_enqueue_task(struct rq *rq, struct task_struct *p, u64 enq_flags,
			 int sticky_cpu)
{
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
	 * A task reenqueued too many times without running means the scheduler
	 * keeps re-deciding a placement it can't honor, e.g. re-inserting to a
	 * cid it lacks caps on. Eject the owning scheduler and strand the task
	 * to be picked up during sched exit.
	 */
	if (enq_flags & SCX_ENQ_REENQ) {
		if (++p->scx.reenq_cnt > 1)
			__scx_add_event(sch, SCX_EV_REENQ_REPEAT, 1);

		if (unlikely(p->scx.reenq_cnt > SCX_REENQ_MAX_REPEAT)) {
			__scx_exit(sch, SCX_EXIT_ERROR_REENQ, 0, cpu_of(rq),
				   "%s[%d] reenqueued %u times without running",
				   p->comm, p->pid, p->scx.reenq_cnt);
			return;
		}
	}

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

	if (p->scx.ddsp_dsq_id != SCX_DSQ_INVALID)
		goto direct;

	/* see %SCX_OPS_ENQ_EXITING */
	if (!(sch->ops.flags & SCX_OPS_ENQ_EXITING) &&
	    unlikely(p->flags & PF_EXITING)) {
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

	if (unlikely(!SCX_HAS_OP(sch, enqueue)))
		goto global;

	/* DSQ bypass didn't trigger, enqueue on the BPF scheduler */
	qseq = rq->scx.ops_qseq++ << SCX_OPSS_QSEQ_SHIFT;

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
	return;

direct:
	direct_dispatch(sch, p, enq_flags);
	return;
local_norefill:
	scx_dispatch_enqueue(sch, rq, &rq->scx.local_dsq, p, 0, 0, enq_flags);
	return;
local:
	dsq = &rq->scx.local_dsq;
	goto enqueue;
global:
	dsq = find_global_dsq(sch, task_cpu(p));
	goto enqueue;
bypass:
	dsq = bypass_enq_target_dsq(sch, task_cpu(p));
	goto enqueue;

enqueue:
	refill_task_slice_dfl(sch, p);
	clear_direct_dispatch(p);
	scx_dispatch_enqueue(sch, rq, dsq, p, 0, 0, enq_flags);
}
/* …（同文件无关代码省略）… */
	}

	/*
	 * list_add_tail() must be used. scx_bypass() depends on tasks being
	 * appended to the runnable_list.
	 */
	list_add_tail(&p->scx.runnable_node, &rq->scx.runnable_list);

/* …（同文件无关代码省略）… */
	int sticky_cpu = p->scx.sticky_cpu;
	u64 enq_flags = core_enq_flags | rq->scx.remote_activate_enq_flags;

	/*
	 * SCX_RQ_IN_WAKEUP promises a task_woken_scx() call once this enqueue
	 * returns. Only the core's wakeup path delivers one. The flags stashed
/* …（同文件无关代码省略）… */
		dl_server_start(&rq->ext_server);

	scx_do_enqueue_task(rq, p, enq_flags, sticky_cpu);

	if (sticky_cpu >= 0)
		p->scx.sticky_cpu = -1;
out:
	rq->scx.flags &= ~SCX_RQ_IN_WAKEUP;

/* …（同文件无关代码省略）… */

static void rq_offline_scx(struct rq *rq)
{
	rq->scx.flags &= ~SCX_RQ_ONLINE;
	scx_rescue_flush(rq);
}

static bool check_rq_for_timeouts(struct rq *rq)
{
	struct scx_sched *sch;
	struct task_struct *p;
	struct rq_flags rf;
	bool timed_out = false;

	rq_lock_irqsave(rq, &rf);
	sch = rcu_dereference_bh(scx_root);
	if (unlikely(!sch))
		goto out_unlock;

	list_for_each_entry(p, &rq->scx.runnable_list, scx.runnable_node) {
		struct scx_sched *sch = scx_task_sched(p);
		unsigned long last_runnable = p->scx.runnable_at;

		if (unlikely(time_after(jiffies,
					last_runnable + READ_ONCE(sch->watchdog_timeout)))) {
			struct scx_dispatch_q *dsq = READ_ONCE(p->scx.dsq);
			u32 dur_ms = jiffies_to_msecs(jiffies - last_runnable);

			/*
			 * A task can be stuck on a DSQ that a sched other than
			 * its owner is responsible for draining, e.g. an
			 * ancestor's bypass DSQ while the owner is bypassing.
			 * Blame the drainer. The local DSQ is consumed by the
			 * cpu itself and keeps blame on the owner.
			 */
			if (dsq && dsq->sched && dsq->id != SCX_DSQ_LOCAL)
				sch = dsq->sched;

			__scx_exit(sch, SCX_EXIT_ERROR_STALL, 0, cpu_of(rq),
				   "%s[%d] failed to run for %u.%03us",
				   p->comm, p->pid, dur_ms / 1000,
				   dur_ms % 1000);
			timed_out = true;
			break;
		}
	}
out_unlock:
	rq_unlock_irqrestore(rq, &rf);
	return timed_out;
}
/* …（同文件无关代码省略）… */
	scx_for_each_descendant_pre(pos, sch) {
		if (pos == sch)
			continue;
		if (bypass)
			inc_bypass_depth(pos);
		else
			dec_bypass_depth(pos);
	}
/* …（同文件无关代码省略）… */
static const char *scx_exit_reason(enum scx_exit_kind kind)
{
	switch (kind) {
	case SCX_EXIT_UNREG:
		return "unregistered from user space";
	case SCX_EXIT_UNREG_BPF:
		return "unregistered from BPF";
	case SCX_EXIT_UNREG_KERN:
		return "unregistered from the main kernel";
	case SCX_EXIT_SYSRQ:
		return "disabled by sysrq-S";
	case SCX_EXIT_PARENT:
		return "parent exiting";
	case SCX_EXIT_PARENT_KILL:
		return "killed by parent scheduler";
	case SCX_EXIT_ERROR:
		return "runtime error";
	case SCX_EXIT_ERROR_BPF:
		return "scx_bpf_error";
	case SCX_EXIT_ERROR_STALL:
		return "runnable task stall";
	case SCX_EXIT_ERROR_REENQ:
		return "reenqueue limit";
	case SCX_EXIT_ERROR_RESCUE:
		return "rescue bandwidth overload";
	default:
		return "<UNKNOWN>";
	}
}
/* …（同文件无关代码省略）… */
static bool scx_claim_exit(struct scx_sched *sch, enum scx_exit_kind kind)
{
	int none = SCX_EXIT_NONE;

	lockdep_assert_preemption_disabled();

	if (WARN_ON_ONCE(kind == SCX_EXIT_NONE || kind == SCX_EXIT_DONE))
		kind = SCX_EXIT_ERROR;

	if (!atomic_try_cmpxchg(&sch->exit_kind, &none, kind))
		return false;

	if (kind == SCX_EXIT_PARENT) {
		/* an ancestor is already sweeping the subtree */
		WRITE_ONCE(sch->aborting, true);
	} else {
		struct scx_sched *pos;

		/*
		 * CPUs may be live-locked in the dispatch paths of @sch or its
		 * descendants, which ->aborting breaks. Sweep the subtree
		 * locklessly so that this works from NMI. smp_store_mb() orders
		 * each node's ->aborting store before its children are walked -
		 * either we see a racing scx_link_sched() on ->children or it
		 * sees ->aborting.
		 */
		scoped_guard (rcu) {
			scx_for_each_descendant_pre(pos, sch)
				smp_store_mb(pos->aborting, true);
		}

		irq_work_queue(&sch->propagate_exit_irq_work);
	}

	/* fired after ->aborting is set so callbacks can't delay recovery */
	trace_sched_ext_exit(sch, kind);

	return true;
}
/* …（同文件无关代码省略）… */
static void dump_newline(struct seq_buf *s)
{
	trace_sched_ext_dump("");

	/* @s may be zero sized and seq_buf triggers WARN if so */
	if (s->size)
		seq_buf_putc(s, '\n');
}
/* …（同文件无关代码省略）… */
__printf(2, 3) void scx_dump_line(struct seq_buf *s, const char *fmt, ...)
{
	va_list args;

#ifdef CONFIG_TRACEPOINTS
	if (trace_sched_ext_dump_enabled()) {
		/* protected by scx_dump_lock */
		static char line_buf[SCX_EXIT_MSG_LEN];

		va_start(args, fmt);
		vscnprintf(line_buf, sizeof(line_buf), fmt, args);
		va_end(args);

		trace_call__sched_ext_dump(line_buf);
	}
#endif
	/* @s may be zero sized and seq_buf triggers WARN if so */
	if (s->size) {
		va_start(args, fmt);
		seq_buf_vprintf(s, fmt, args);
		va_end(args);

		seq_buf_putc(s, '\n');
	}
}
/* …（同文件无关代码省略）… */
	seq_buf_init(&ns, buf, avail);

	dump_newline(&ns);
	scx_dump_line(&ns, "CPU %-4d: nr_run=%u flags=0x%x cpu_rel=%d ops_qseq=%lu ksync=%lu",
		      cpu, rq->scx.nr_running, rq->scx.flags, rq->scx.cpu_released,
		      rq->scx.ops_qseq, rq->scx.kick_sync);
	scx_rescue_dump(&ns, rq);
	scx_dump_line(&ns, "          curr=%s[%d] class=%ps",
		      rq->curr->comm, rq->curr->pid, rq->curr->sched_class);
/* …（同文件无关代码省略）… */
static void scx_finish_exit(struct scx_sched *sch, enum scx_exit_kind kind,
			    s64 exit_code, s32 exit_cpu)
{
	struct scx_exit_info *ei = sch->exit_info;

	ei->exit_code = exit_code;
#ifdef CONFIG_STACKTRACE
	/*
	 * stack_trace_save()'s NMI-safety is arch-dependent and undocumented.
	 * Skip the backtrace when exiting from NMI.
	 */
	if (kind >= SCX_EXIT_ERROR && !in_nmi())
		ei->bt_len = stack_trace_save(ei->bt, SCX_EXIT_BT_LEN, 1);
#endif
	/*
	 * Set ei->kind and ->reason for scx_dump_state(). They'll be set again
	 * in scx_disable_workfn().
	 */
	ei->kind = kind;
	ei->reason = scx_exit_reason(ei->kind);
	ei->exit_cpu = exit_cpu;

	irq_work_queue(&sch->disable_irq_work);
}
/* …（同文件无关代码省略）… */
bool scx_vexit(struct scx_sched *sch,
	       enum scx_exit_kind kind, s64 exit_code, s32 exit_cpu,
	       const char *fmt, va_list args)
{
	struct scx_exit_info *ei = sch->exit_info;

	guard(preempt)();

	if (!scx_claim_exit(sch, kind))
		return false;

	vscnprintf(ei->msg, SCX_EXIT_MSG_LEN, fmt, args);

	scx_finish_exit(sch, kind, exit_code, exit_cpu);
	return true;
}
