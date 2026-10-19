// AUTO-DRAFT from torvalds/linux PR #88328f850d956d267d11e2ee225ee1eaa902b8e0
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <time.h>
  // <<< BUG ANCHOR
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

	trace_sched_set_need_resched_tp(curr, cpu, tif);
	if (cpu == smp_processor_id()) {
		set_ti_thread_flag(cti, tif);
		if (tif == TIF_NEED_RESCHED)
			set_preempt_need_resched();
		return;
	}

	if (set_nr_and_not_polling(cti, tif)) {
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
		if (p->sched_class->switched_to)
			p->sched_class->switched_to(rq, p);

		if (ctx->running) {
			/*
			 * If this was a class promotion; let the old class
			 * know it got preempted. Note that none of the
			 * switch*_from() methods know the new class and none
			 * of the switch*_to() methods know the old class.
			 */
			if (sched_class_above(p->sched_class, ctx->class)) {
				rq->next_class->wakeup_preempt(rq, p, 0);
				rq->next_class = p->sched_class;
			}
			/*
			 * If this was a degradation in class; make sure to
			 * reschedule.
