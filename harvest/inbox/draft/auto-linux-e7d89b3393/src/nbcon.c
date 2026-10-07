// AUTO-DRAFT from torvalds/linux PR #22430ae5d90ab288b0ee2ad99ae941f4a666b694
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static inline void nbcon_state_read(struct console *con, struct nbcon_state *state)
{
	state->atom = atomic_read(&ACCESS_PRIVATE(con, nbcon_state));
}
/* …（同文件无关代码省略）… */
static inline bool nbcon_state_try_cmpxchg(struct console *con, struct nbcon_state *cur,
					   struct nbcon_state *new)
{
	return atomic_try_cmpxchg(&ACCESS_PRIVATE(con, nbcon_state), &cur->atom, new->atom);
}
/* …（同文件无关代码省略）… */
u64 nbcon_seq_read(struct console *con)
{
	unsigned long nbcon_seq = atomic_long_read(&ACCESS_PRIVATE(con, nbcon_seq));

	return __ulseq_to_u64seq(prb, nbcon_seq);
}
/* …（同文件无关代码省略）… */
static void nbcon_seq_try_update(struct nbcon_context *ctxt, u64 new_seq)
{
	unsigned long nbcon_seq = __u64seq_to_ulseq(ctxt->seq);
	struct console *con = ctxt->console;

	if (atomic_long_try_cmpxchg(&ACCESS_PRIVATE(con, nbcon_seq), &nbcon_seq,
				    __u64seq_to_ulseq(new_seq))) {
		ctxt->seq = new_seq;
	} else {
		ctxt->seq = nbcon_seq_read(con);
	}
}
/* …（同文件无关代码省略）… */
static int nbcon_context_try_acquire_direct(struct nbcon_context *ctxt,
					    struct nbcon_state *cur, bool is_reacquire)
{
	unsigned int cpu = smp_processor_id();
	struct console *con = ctxt->console;
	struct nbcon_state new;

	do {
		/*
		 * Panic does not imply that the console is owned. However,
		 * since all non-panic CPUs are stopped during panic(), it
		 * is safer to have them avoid gaining console ownership.
		 *
		 * One exception is when kdb has locked for printing on this CPU.
		 *
		 * Second exception is a reacquire (and an unsafe takeover
		 * has not previously occurred) then it is allowed to attempt
		 * a direct acquire in panic. This gives console drivers an
		 * opportunity to perform any necessary cleanup if they were
		 * interrupted by the panic CPU while printing.
		 */
		if (panic_on_other_cpu() &&
		    !kdb_printf_on_this_cpu() &&
		    (!is_reacquire || cur->unsafe_takeover)) {
			return -EPERM;
		}

		if (ctxt->prio <= cur->prio || ctxt->prio <= cur->req_prio)
			return -EPERM;

		if (cur->unsafe)
			return -EBUSY;

		/*
		 * The console should never be safe for a direct acquire
		 * if an unsafe hostile takeover has ever happened.
		 */
		WARN_ON_ONCE(cur->unsafe_takeover);

		new.atom = cur->atom;
		new.prio	= ctxt->prio;
		new.req_prio	= NBCON_PRIO_NONE;
		new.unsafe	= cur->unsafe_takeover;
		new.cpu		= cpu;

	} while (!nbcon_state_try_cmpxchg(con, cur, &new));

	return 0;
}
/* …（同文件无关代码省略）… */
static bool nbcon_waiter_matches(struct nbcon_state *cur, int expected_prio)
{
	/*
	 * The request context is well defined by the @req_prio because:
	 *
	 * - Only a context with a priority higher than the owner can become
	 *   a waiter.
	 * - Only a context with a priority higher than the waiter can
	 *   directly take over the request.
	 * - There are only three priorities.
	 * - Only one CPU is allowed to request PANIC priority.
	 * - Lower priorities are ignored during panic() until reboot.
	 *
	 * As a result, the following scenario is *not* possible:
	 *
	 * 1. This context is currently a waiter.
	 * 2. Another context with a higher priority than this context
	 *    directly takes ownership.
	 * 3. The higher priority context releases the ownership.
	 * 4. Another lower priority context takes the ownership.
	 * 5. Another context with the same priority as this context
	 *    creates a request and starts waiting.
	 *
	 * Event #1 implies this context is EMERGENCY.
	 * Event #2 implies the new context is PANIC.
	 * Event #3 occurs when panic() has flushed the console.
	 * Event #4 occurs when a non-panic CPU reacquires.
	 * Event #5 is not possible due to the panic_on_other_cpu() check
	 *          in nbcon_context_try_acquire_handover().
	 */

	return (cur->req_prio == expected_prio);
}
/* …（同文件无关代码省略）… */
static int nbcon_context_try_acquire_requested(struct nbcon_context *ctxt,
					       struct nbcon_state *cur)
{
	unsigned int cpu = smp_processor_id();
	struct console *con = ctxt->console;
	struct nbcon_state new;

	/* Note that the caller must still remove the request! */
	if (panic_on_other_cpu())
		return -EPERM;

	/*
	 * Note that the waiter will also change if there was an unsafe
	 * hostile takeover.
	 */
	if (!nbcon_waiter_matches(cur, ctxt->prio))
		return -EPERM;

	/* If still locked, caller should continue waiting. */
	if (cur->prio != NBCON_PRIO_NONE)
		return -EBUSY;

	/*
	 * The previous owner should have never released ownership
	 * in an unsafe region.
	 */
	WARN_ON_ONCE(cur->unsafe);

	new.atom = cur->atom;
	new.prio	= ctxt->prio;
	new.req_prio	= NBCON_PRIO_NONE;
	new.unsafe	= cur->unsafe_takeover;
	new.cpu		= cpu;

	if (!nbcon_state_try_cmpxchg(con, cur, &new)) {
		/*
		 * The acquire could fail only when it has been taken
		 * over by a higher priority context.
		 */
		WARN_ON_ONCE(nbcon_waiter_matches(cur, ctxt->prio));
		return -EPERM;
	}

	/* Handover success. This context now owns the console. */
	return 0;
}
/* …（同文件无关代码省略）… */
static int nbcon_context_try_acquire_handover(struct nbcon_context *ctxt,
					      struct nbcon_state *cur)
{
	unsigned int cpu = smp_processor_id();
	struct console *con = ctxt->console;
	struct nbcon_state new;
	int timeout;
	int request_err = -EBUSY;

	/*
	 * Check that the handover is called when the direct acquire failed
	 * with -EBUSY.
	 */
	WARN_ON_ONCE(ctxt->prio <= cur->prio || ctxt->prio <= cur->req_prio);
	WARN_ON_ONCE(!cur->unsafe);

	/*
	 * Panic does not imply that the console is owned. However, it
	 * is critical that non-panic CPUs during panic are unable to
	 * wait for a handover in order to satisfy the assumptions of
	 * nbcon_waiter_matches(). In particular, the assumption that
	 * lower priorities are ignored during panic.
	 */
	if (panic_on_other_cpu())
		return -EPERM;

	/* Handover is not possible on the same CPU. */
	if (cur->cpu == cpu)
		return -EBUSY;

	/*
	 * Console stays unsafe after an unsafe 
