// AUTO-DRAFT from torvalds/linux PR #7a3d2740d1b1b5ed8d22e1e5430de3b5046cf6a9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static noinline int __cpuidle cpu_idle_poll(void)
{
	instrumentation_begin();
	trace_cpu_idle(0, smp_processor_id());
	stop_critical_timings();
	ct_cpuidle_enter();

	raw_local_irq_enable();
	while (!tif_need_resched() &&
	       (cpu_idle_force_poll || tick_check_broadcast_expired()))
		cpu_relax();
	raw_local_irq_disable();

	ct_cpuidle_exit();
	start_critical_timings();
	trace_cpu_idle(PWR_EVENT_EXIT, smp_processor_id());
	local_irq_enable();
	instrumentation_end();

	return 1;
}
/* …（同文件无关代码省略）… */
void __weak arch_cpu_idle_enter(void) { }
void __weak arch_cpu_idle_exit(void) { }
void __weak __noreturn arch_cpu_idle_dead(void) { while (1); }
void __weak arch_cpu_idle(void)
{
	cpu_idle_force_poll = 1;
}
/* …（同文件无关代码省略）… */
static inline void cond_tick_broadcast_enter(void)
{
	if (static_branch_unlikely(&arch_needs_tick_broadcast))
		tick_broadcast_enter();
}
/* …（同文件无关代码省略）… */
static inline void cond_tick_broadcast_exit(void)
{
	if (static_branch_unlikely(&arch_needs_tick_broadcast))
		tick_broadcast_exit();
}
/* …（同文件无关代码省略）… */
void __cpuidle default_idle_call(void)
{
	instrumentation_begin();
	if (!current_clr_polling_and_test()) {
		cond_tick_broadcast_enter();
		trace_cpu_idle(1, smp_processor_id());
		stop_critical_timings();

		ct_cpuidle_enter();
		arch_cpu_idle();
		ct_cpuidle_exit();

		start_critical_timings();
		trace_cpu_idle(PWR_EVENT_EXIT, smp_processor_id());
		cond_tick_broadcast_exit();
	}
	local_irq_enable();
	instrumentation_end();
}
/* …（同文件无关代码省略）… */
static int call_cpuidle_s2idle(struct cpuidle_driver *drv,
			       struct cpuidle_device *dev,
			       u64 max_latency_ns)
{
	if (current_clr_polling_and_test())
		return -EBUSY;

	return cpuidle_enter_s2idle(drv, dev, max_latency_ns);
}
/* …（同文件无关代码省略）… */
static int call_cpuidle(struct cpuidle_driver *drv, struct cpuidle_device *dev,
		      int next_state)
{
	/*
	 * The idle task must be scheduled, it is pointless to go to idle, just
	 * update no idle residency and return.
	 */
	if (current_clr_polling_and_test()) {
		dev->last_residency_ns = 0;
		local_irq_enable();
		return -EBUSY;
	}

	/*
	 * Enter the idle state previously returned by the governor decision.
	 * This function will block until an interrupt occurs and will take
	 * care of re-enabling the local interrupts
	 */
	return cpuidle_enter(drv, dev, next_state);
}
/* …（同文件无关代码省略）… */
static void idle_call_stop_or_retain_tick(bool stop_tick)
{
	if (stop_tick || tick_nohz_tick_stopped())
		tick_nohz_idle_stop_tick();
	else
		tick_nohz_idle_retain_tick();
}
/* …（同文件无关代码省略）… */
static void cpuidle_idle_call(bool stop_tick)
{
	struct cpuidle_device *dev = cpuidle_get_device();
	struct cpuidle_driver *drv = cpuidle_get_cpu_driver(dev);
	int next_state, entered_state;

	/*
	 * Check if the idle task must be rescheduled. If it is the
	 * case, exit the function after re-enabling the local IRQ.
	 */
	if (need_resched()) {
		local_irq_enable();
		return;
	}

	if (cpuidle_not_available(drv, dev)) {
		idle_call_stop_or_retain_tick(stop_tick);

		default_idle_call();
		goto exit_idle;
	}

	/*
	 * Suspend-to-idle ("s2idle") is a system state in which all user space
	 * has been frozen, all I/O devices have been suspended and the only
	 * activity happens here and in interrupts (if any). In that case bypass
	 * the cpuidle governor and go straight for the deepest idle state
	 * available.  Possibly also suspend the local tick and the entire
	 * timekeeping to prevent timer interrupts from kicking us out of idle
	 * until a proper wakeup interrupt happens.
	 */

	if (idle_should_enter_s2idle() || dev->forced_idle_latency_limit_ns) {
		u64 max_latency_ns;

		if (idle_should_enter_s2idle()) {
			max_latency_ns = cpu_wakeup_latency_qos_limit() *
					 NSEC_PER_USEC;

			entered_state = call_cpuidle_s2idle(drv, dev,
							    max_latency_ns);
			if (entered_state > 0)
				goto exit_idle;
		} else {
			max_latency_ns = dev->forced_idle_latency_limit_ns;
		}

		tick_nohz_idle_stop_tick();

		next_state = cpuidle_find_deepest_state(drv, dev, max_latency_ns);
		call_cpuidle(drv, dev, next_state);
	} else if (drv->state_count > 1) {
		/*
		 * stop_tick is expected to be true by default by cpuidle
		 * governors, which allows them to select idle states with
		 * target residency above the tick period length.
		 */
		stop_tick = true;

		/*
		 * Ask the cpuidle framework to choose a convenient idle state.
		 */
		next_state = cpuidle_select(drv, dev, &stop_tick);

		idle_call_stop_or_retain_tick(stop_tick);

		entered_state = call_cpuidle(drv, dev, next_state);
		/*
		 * Give the governor an opportunity to reflect on the outcome
		 */
		cpuidle_reflect(dev, entered_state);
	} else {
		idle_call_stop_or_retain_tick(stop_tick);

		/*
		 * If there is only a single idle state (or none), there is
		 * nothing meaningful for the governor to choose.  Skip the
		 * governor and always use state 0.
		 */
		call_cpuidle(drv, dev, 0);
	}

exit_idle:
	__current_set_polling();

	/*
	 * It is up to the idle functions to re-enable local interrupts
	 */
	if (WARN_ON_ONCE(irqs_disabled()))
		local_irq_enable();
}
/* …（同文件无关代码省略）… */
static void do_idle(void)
{
	int cpu = smp_processor_id();
	bool got_tick = false;

	if (cpu_is_offline(cpu)) {
		local_irq_disable();
		/* All per-CPU kernel threads should be done by now. */
		WARN_ON_ONCE(need_resched());
		cpuhp_report_idle_dead();
		arch_cpu_idle_dead();
	}

	/*
	 * Check if we need to update blocked load
	 */
	nohz_run_idle_balance(cpu);

	/*
	 * If the arch has a polling bit, we maintain an invariant:
	 *
	 * Our polling bit is clear if we're not scheduled (i.e. if rq->curr !=
	 * rq->idle). This means that, if rq->idle has the polling bit set,
	 * then setting need_resched is guaranteed to cause the CPU to
	 * reschedule.
	 */

	__current_set_polling();
	tick_nohz_idle_enter();

	while (!need_resched()) {

		/*
		 * Interrupts shouldn't be re-enabled from that point on until
		 * the CPU sleeping instruction is reached. Otherwise an interrupt
		 * may fire and queue a timer that would be ignored until the CPU
		 * wakes from the sleeping instruction. And testing need_resched()
		 * doesn't tell about pending needed timer reprogram.
		 *
		 * Several cases to consider:
		 *
		 * - SLEEP-UNTIL-PENDING-INTERRUPT based instructions such as
		 *   "wfi" or "mwait" are fine because they can be entered with
		 *   interrupt disabled.
		 *
		 * - sti;mwait() couple is fine because the interrupts are
		 *   re-enabled only upon the execution of mwait, leaving no gap
		 *   in-between.
		 *
		 * - ROLLBACK based idle handlers with the sleeping instruction
		 *   called with interrupts enabled are NOT fine. In this scheme
		 *   when the interrupt detects it has interrupted an idle handler,
		 *   it rolls back to its beginning which performs the
		 *   need_resched() check before re-executing the sleeping
		 *   instruction. This can leak a pending needed timer reprogram.
		 *   If such a scheme is really mandatory due to the lack of an
		 *   appropriate CPU sleeping instruction, then a FAST-FORWARD
		 *   must instead be applied: when the interrupt detects it has
		 *   interrupted an idle handler, it must resume to the end of
		 *   this idle handler so that the generic idle loop is iterated
		 *   again to reprogram the tick.
		 */
		local_irq_disable();

		arch_cpu_idle_enter();
		rcu_nocb_flush_deferred_wakeup();

		/*
		 * In poll mode we re-enable interrupts and spin. Also if we
		 * detected in the wakeup from idle path that the tick
		 * broadcast device expired for us, we don't want to go deep
		 * idle as we know that the IPI is going to arrive right away.
		 */
		if (cpu_idle_force_poll || tick_check_broadcast_expired()) {
			tick_nohz_idle_restart_tick();
			cpu_idle_poll();
		} else {
			cpuidle_idle_call(got_tick);
		}
		got_tick = tick_nohz_idle_got_tick();
		arch_cpu_idle_exit();
	}

	/*
	 * Since we fell out of the loop above, we know TIF_NEED_RESCHED must
	 * be set, propagate it into PREEMPT_NEED_RESCHED.
	 *
	 * This is required because for polling idle loops we will not have had
	 * an IPI to fold the state for us.
	 */
	preempt_set_need_resched();
	tick_nohz_idle_exit();
	__current_clr_polling();

	/*
	 * We promise to call sched_ttwu_pending() and reschedule if
	 * need_resched() is set while polling is set. That means that clearing
	 * polling needs to be visible before doing these things.
	 */
	smp_mb__after_atomic();

	/*
	 * RCU relies on this call to be done outside of an RCU read-side
	 * critical section.
	 */
	flush_smp_call_function_queue();
	schedule_idle();

	if (unlikely(klp_patch_pending(current)))
		klp_update_patch_state(current);
}
/* …（同文件无关代码省略）… */
static enum hrtimer_restart idle_inject_timer_fn(struct hrtimer *timer)
{
	struct idle_timer *it = container_of(timer, struct idle_timer, timer);

	WRITE_ONCE(it->done, 1);
	set_tsk_need_resched(current);

	return HRTIMER_NORESTART;
}
/* …（同文件无关代码省略）… */
	cpuidle_use_deepest_state(latency_ns);

	it.done = 0;
	hrtimer_setup_on_stack(&it.timer, idle_inject_timer_fn, CLOCK_MONOTONIC,
			       HRTIMER_MODE_REL_HARD);
	hrtimer_start(&it.timer, ns_to_ktime(duration_ns),
		      HRTIMER_MODE_REL_PINNED_HARD);

	while (!READ_ONCE(it.done))
		do_idle();

	cpuidle_use_deepest_state(0);
	current->flags &= ~PF_IDLE;
