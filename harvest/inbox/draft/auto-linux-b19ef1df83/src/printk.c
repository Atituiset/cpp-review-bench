// AUTO-DRAFT from torvalds/linux PR #bdeb84ed24e543930487f03f5243bbf83aebaf31
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define down_console_sem() do { \
	down(&console_sem);\
	mutex_acquire(&console_lock_dep_map, 0, 0, _RET_IP_);\
} while (0)
/* …（同文件无关代码省略）… */
#define up_console_sem() __up_console_sem(_RET_IP_)
/* …（同文件无关代码省略）… */
#define MAX_CMDLINECONSOLES 8
/* …（同文件无关代码省略）… */
#define prb_read_valid(rb, seq, r)	false
/* …（同文件无关代码省略）… */
#define prb_next_seq(rb)		0
/* …（同文件无关代码省略）… */
void console_lock(void)
{
	might_sleep();

	/* On panic, the console_lock must be left to the panic cpu. */
	while (panic_on_other_cpu())
		msleep(1000);

	down_console_sem();
	console_locked = 1;
	console_may_schedule = 1;
}
/* …（同文件无关代码省略）… */
int console_trylock(void)
{
	/* On panic, the console_lock must be left to the panic cpu. */
	if (panic_on_other_cpu())
		return 0;
	if (down_trylock_console_sem())
		return 0;
	console_locked = 1;
	console_may_schedule = 0;
	return 1;
}
/* …（同文件无关代码省略）… */
static void __console_unlock(void)
{
	console_locked = 0;
	up_console_sem();
}
/* …（同文件无关代码省略）… */
static bool console_emit_next_record(struct console *con, bool *handover, int cookie)
{
	bool is_extended = console_srcu_read_flags(con) & CON_EXTENDED;
	char *outbuf = &printk_shared_pbufs.outbuf[0];
	struct printk_message pmsg = {
		.pbufs = &printk_shared_pbufs,
	};
	unsigned long flags;

	*handover = false;

	if (!printk_get_next_message(&pmsg, con->seq, is_extended, true))
		return false;

	con->dropped += pmsg.dropped;

	/* Skip messages of formatted length 0. */
	if (pmsg.outbuf_len == 0) {
		con->seq = pmsg.seq + 1;
		goto skip;
	}

	if (con->dropped && !is_extended) {
		console_prepend_dropped(&pmsg, con->dropped);
		con->dropped = 0;
	}

	/* Write everything out to the hardware. */

	if (force_legacy_kthread() && !panic_in_progress()) {
		/*
		 * With forced threading this function is in a task context
		 * (either legacy kthread or get_init_console_seq()). There
		 * is no need for concern about printk reentrance, handovers,
		 * or lockdep complaints.
		 */

		con->write(con, outbuf, pmsg.outbuf_len);
		con->seq = pmsg.seq + 1;
	} else {
		/*
		 * While actively printing out messages, if another printk()
		 * were to occur on another CPU, it may wait for this one to
		 * finish. This task can not be preempted if there is a
		 * waiter waiting to take over.
		 *
		 * Interrupts are disabled because the hand over to a waiter
		 * must not be interrupted until the hand over is completed
		 * (@console_waiter is cleared).
		 */
		printk_safe_enter_irqsave(flags);
		console_lock_spinning_enable();

		/* Do not trace print latency. */
		stop_critical_timings();

		printk_legacy_allow_spinlock_enter();
		con->write(con, outbuf, pmsg.outbuf_len);
		printk_legacy_allow_spinlock_exit();

		start_critical_timings();

		con->seq = pmsg.seq + 1;

		*handover = console_lock_spinning_disable_and_check(cookie);
		printk_safe_exit_irqrestore(flags);
	}
skip:
	return true;
}
/* …（同文件无关代码省略）… */
static inline void printk_kthreads_check_locked(void) { }
/* …（同文件无关代码省略）… */
static bool console_flush_one_record(bool do_cond_resched, u64 *next_seq, bool *handover,
				     bool *try_again)
{
	struct console_flush_type ft;
	bool any_usable = false;
	struct console *con;
	int cookie;

	*try_again = false;

	printk_get_console_flush_type(&ft);

	cookie = console_srcu_read_lock();
	for_each_console_srcu(con) {
		short flags = console_srcu_read_flags(con);
		u64 printk_seq;
		bool progress;

		/*
		 * console_flush_one_record() is only responsible for
		 * nbcon consoles when the nbcon consoles cannot print via
		 * their atomic or threaded flushing.
		 */
		if ((flags & CON_NBCON) && (ft.nbcon_atomic || ft.nbcon_offload))
			continue;

		if (!console_is_usable(con, flags, !do_cond_resched))
			continue;
		any_usable = true;

		if (flags & CON_NBCON) {
			progress = nbcon_legacy_emit_next_record(con, handover, cookie,
								 !do_cond_resched);
		} else {
			progress = console_emit_next_record(con, handover, cookie);
		}

		/*
		 * If a handover has occurred, the SRCU read lock
		 * is already released.
		 */
		if (*handover)
			goto fail;

		/*
		 * @con can be used here now that it is certain that this
		 * context is still holding the SRCU read lock.
		 */
		if (flags & CON_NBCON)
			printk_seq = nbcon_seq_read(con);
		else
			printk_seq = con->seq;

		/* Track the next of the highest seq flushed. */
		if (printk_seq > *next_seq)
			*next_seq = printk_seq;

		if (!progress)
			continue;

		/*
		 * A usable console made progress. There might still be
		 * pending messages.
		 */
		*try_again = true;

		/* Allow panic_cpu to take over the consoles safely. */
		if (panic_on_other_cpu())
			goto fail_srcu;

		if (do_cond_resched)
			cond_resched();
	}
	console_srcu_read_unlock(cookie);

	return any_usable;

fail_srcu:
	console_srcu_read_unlock(cookie);
fail:
	*try_again = false;
	return false;
}
/* …（同文件无关代码省略）… */
static bool console_flush_all(bool do_cond_resched, u64 *next_seq, bool *handover)
{
	bool try_again;
	bool ret;

	*next_seq = 0;
	*handover = false;

	do {
		ret = console_flush_one_record(do_cond_resched, next_seq,
					       handover, &try_again);
	} while (try_again);

	return ret;
}
/* …（同文件无关代码省略）… */
static void __console_flush_and_unlock(void)
{
	bool do_cond_resched;
	bool handover;
	bool flushed;
	u64 next_seq;

	/*
	 * Console drivers are called with interrupts disabled, so
	 * @console_may_schedule should be cleared before; however, we may
	 * end up dumping a lot of lines, for example, if called from
	 * console registration path, and should invoke cond_resched()
	 * between lines if allowable.  Not doing so can cause a very long
	 * scheduling stall on a slow console leading to RCU stall and
	 * softlockup warnings which exacerbate the issue with more
	 * messages practically incapacitating the system. Therefore, create
	 * a local to use for the printing loop.
	 */
	do_cond_resched = console_may_schedule;

	do {
		console_may_schedule = 0;

		flushed = console_flush_all(do_cond_resched, &next_seq, &handover);
		if (!handover)
			__console_unlock();

		/*
		 * Abort if there was a failure to flush all messages to all
		 * usable consoles. Either it is not possible to flush (in
		 * which case it would be an infinite loop of retrying) or
		 * another context has taken over printing.
		 */
		if (!flushed)
			break;

		/*
		 * Some context may have added new records after
		 * console_flush_all() but before unlocking the console.
		 * Re-check if there is a new record to flush. If the trylock
		 * fails, another context is already handling the printing.
		 */
	} while (prb_read_valid(prb, next_seq, NULL) && console_trylock());
}
/* …（同文件无关代码省略）… */
void console_unlock(void)
{
	struct console_flush_type ft;

	printk_get_console_flush_type(&ft);
	if (ft.legacy_direct)
		__console_flush_and_unlock();
	else
		__console_unlock();
}
/* …（同文件无关代码省略）… */
 * If appropriate, start nbcon kthreads and set @printk_kthreads_running.
 * If any kthreads fail to start, those consoles are unregistered.
 *
 * Must be called under console_list_lock().
 */
static void printk_kthreads_check_locked(void)
/* …（同文件无关代码省略）… */
static int console_call_setup(struct console *newcon, char *options)
{
	int err;

	if (!newcon->setup)
		return 0;

	/* Synchronize with possible boot console. */
	console_lock();
	err = newcon->setup(newcon, options);
	console_unlock();

	return err;
}
/* …（同文件无关代码省略）… */
static int try_enable_preferred_console(struct console *newcon,
					bool user_specified)
{
	struct console_cmdline *c;
	int i, err;

	for (i = 0, c = console_cmdline;
	     i < MAX_CMDLINECONSOLES && (c->name[0] || c->devname[0]);
	     i++, c++) {
		/* Console not yet initialized? */
		if (!c->name[0])
			continue;
		if (c->user_specified != user_specified)
			continue;
		if (!newcon->match ||
		    newcon->match(newcon, c->name, c->index, c->options) != 0) {
			/* default matching */
			BUILD_BUG_ON(sizeof(c->name) != sizeof(newcon->name));
			if (strcmp(c->name, newcon->name) != 0)
				continue;
			if (newcon->index >= 0 &&
			    newcon->index != c->index)
				continue;
			if (newcon->index < 0)
				newcon->index = c->index;

			if (_braille_register_console(newcon, c))
				return 0;

			err = console_call_setup(newcon, c->options);
			if (err)
				return err;
		}
		newcon->flags |= CON_ENABLED;
		if (i == preferred_console)
			newcon->flags |= CON_CONSDEV;
		return 0;
	}

	return -ENOENT;
}
/* …（同文件无关代码省略）… */
static void try_enable_default_console(struct console *newcon)
{
	if (newcon->index < 0)
		newcon->index = 0;

	if (console_call_setup(newcon, NULL) != 0)
		return;

	newcon->flags |= CON_ENABLED;

	if (newcon->device)
		newcon->flags |= CON_CONSDEV;
}
/* …（同文件无关代码省略）… */
static u64 get_init_console_seq(struct console *newcon, bool bootcon_registered)
{
	struct console *con;
	bool handover;
	u64 init_seq;

	if (newcon->flags & (CON_PRINTBUFFER | CON_BOOT)) {
		/* Get a consistent copy of @syslog_seq. */
		mutex_lock(&syslog_lock);
		init_seq = syslog_seq;
		mutex_unlock(&syslog_lock);
	} else {
		/* Begin with next message added to ringbuffer. */
		init_seq = prb_next_seq(prb);

		/*
		 * If any enabled boot consoles are due to be unregistered
		 * shortly, some may not be caught up and may be the same
		 * device as @newcon. Since it is not known which boot console
		 * is the same device, flush all consoles and, if necessary,
		 * start with the message of the enabled boot console that is
		 * the furthest behind.
		 */
		if (bootcon_registered && !keep_bootcon) {
			/*
			 * Hold the console_lock to stop console printing and
			 * guarantee safe access to console->seq.
			 */
			console_lock();

			/*
			 * Flush all consoles and set the console to start at
			 * the next unprinted sequence number.
			 */
			if (!console_flush_all(true, &init_seq, &handover)) {
				/*
				 * Flushing failed. Just choose the lowest
				 * sequence of the enabled boot consoles.
				 */

				/*
				 * If there was a handover, this context no
				 * longer holds the console_lock.
				 */
				if (handover)
					console_lock();

				init_seq = prb_next_seq(prb);
				for_each_console(con) {
					u64 seq;

					if (!(con->flags & CON_BOOT) ||
					    !(con->flags & CON_ENABLED)) {
						continue;
					}

					if (con->flags & CON_NBCON)
						seq = nbcon_seq_read(con);
					else
						seq = con->seq;

					if (seq < init_seq)
						init_seq = seq;
				}
			}

			console_unlock();
		}
	}

	return init_seq;
}
/* …（同文件无关代码省略）… */
#define console_first()				\
	hlist_entry(console_list.first, struct console, node)
/* …（同文件无关代码省略）… */
void register_console(struct console *newcon)
{
	bool use_device_lock = (newcon->flags & CON_NBCON) && newcon->write_atomic;
	bool bootcon_registered = false;
	bool realcon_registered = false;
	struct console *con;
	unsigned long flags;
	u64 init_seq;
	int err;

	console_list_lock();

	for_each_console(con) {
		if (WARN(con == newcon, "console '%s%d' already registered\n",
					 con->name, con->index)) {
			goto unlock;
		}

		if (con->flags & CON_BOOT)
			bootcon_registered = true;
		else
			realcon_registered = true;
	}

	/* Do not register boot consoles when there already is a real one. */
	if ((newcon->flags & CON_BOOT) && realcon_registered) {
		pr_info("Too late to register bootconsole %s%d\n",
			newcon->name, newcon->index);
		goto unlock;
	}

	if (newcon->flags & CON_NBCON) {
		/*
		 * Ensure the nbcon console buffers can be allocated
		 * before modifying any global data.
		 */
		if (!nbcon_alloc(newcon))
			goto unlock;
	}

	/*
	 * See if we want to enable this console driver by default.
	 *
	 * Nope when a console is preferred by the command line, device
	 * tree, or SPCR.
	 *
	 * The first real console with tty binding (driver) wins. More
	 * consoles might get enabled before the right one is found.
	 *
	 * Note that a console with tty binding will have CON_CONSDEV
	 * flag set and will be first in the list.
	 */
	if (preferred_console < 0) {
		if (hlist_empty(&console_list) || !console_first()->device ||
		    console_first()->flags & CON_BOOT) {
			try_enable_default_console(newcon);
		}
	}

	/* See if this console matches one we selected on the command line */
	err = try_enable_preferred_console(newcon, true);

	/* If not, try to match against the platform default(s) */
	if (err == -ENOENT)
		err = try_enable_preferred_console(newcon, false);

	/*
	 * Some consoles, such as pstore and netconsole, can be enabled even
	 * without matching. Accept them at this stage when they had a chance
	 * to match() and call setup().
	 */
	if (err == -ENOENT && (newcon->flags & CON_ENABLED))
		err = 0;

	/* printk() messages are not printed to the Braille console. */
	if (err || newcon->flags & CON_BRL) {
		if (newcon->flags & CON_NBCON)
			nbcon_free(newcon);
		goto unlock;
	}

	/*
	 * If we have a bootconsole, and are switching to a real console,
	 * don't print everything out again, since when the boot console, and
	 * the real console are the same physical device, it's annoying to
	 * see the beginning boot messages twice
	 */
	if (bootcon_registered &&
	    ((newcon->flags & (CON_CONSDEV | CON_BOOT)) == CON_CONSDEV)) {
		newcon->flags &= ~CON_PRINTBUFFER;
	}

	newcon->dropped = 0;
	init_seq = get_init_console_seq(newcon, bootcon_registered);

	if (newcon->flags & CON_NBCON) {
		have_nbcon_console = true;
		nbcon_seq_force(newcon, init_seq);
	} else {
		have_legacy_console = true;
		newcon->seq = init_seq;
	}

	if (newcon->flags & CON_BOOT)
		have_boot_console = true;

	/*
	 * If another context is actively using the hardware of this new
	 * console, it will not be aware of the nbcon synchronization. This
	 * is a risk that two contexts could access the hardware
	 * simultaneously if this new console is used for atomic printing
	 * and the other context is still using the hardware.
	 *
	 * Use the driver synchronization to ensure that the hardware is not
	 * in use while this new console transitions to being registered.
	 */
	if (use_device_lock)
		newcon->device_lock(newcon, &flags);

	/*
	 * Put this console in the list - keep the
	 * preferred driver at the head of the list.
	 */
	if (hlist_empty(&console_list)) {
		/* Ensure CON_CONSDEV is always set for the head. */
		newcon->flags |= CON_CONSDEV;
		hlist_add_head_rcu(&newcon->node, &console_list);

	} else if (newcon->flags & CON_CONSDEV) {
		/* Only the new head can have CON_CONSDEV set. */
		console_srcu_write_flags(console_first(), console_first()->flags & ~CON_CONSDEV);
		hlist_add_head_rcu(&newcon->node, &console_list);

	} else {
		hlist_add_behind_rcu(&newcon->node, console_list.first);
	}

	/*
	 * No need to synchronize SRCU here! The caller does not rely
	 * on all contexts being able to see the new console before
	 * register_console() completes.
	 */

	/* This new console is now registered. */
	if (use_device_lock)
		newcon->device_unlock(newcon, flags);

	console_sysfs_notify();

	/*
	 * By unregistering the bootconsoles after we enable the real console
	 * we get the "console xxx enabled" message on all the consoles -
	 * boot consoles, real consoles, etc - this is to ensure that end
	 * users know there might be something in the kernel's log buffer that
	 * went to the bootconsole (that they do not see on the real console)
	 */
	con_printk(KERN_INFO, newcon, "enabled\n");
	if (bootcon_registered &&
	    ((newcon->flags & (CON_CONSDEV | CON_BOOT)) == CON_CONSDEV) &&
	    !keep_bootcon) {
		struct hlist_node *tmp;

		hlist_for_each_entry_safe(con, tmp, &console_list, node) {
			if (con->flags & CON_BOOT)
				unregister_console_locked(con);
		}
	}

	/* Changed console list, may require printer threads to start/stop. */
	printk_kthreads_check_locked();
unlock:
	console_list_unlock();
}
EXPORT_SYMBOL(register_console);

/* Must be called under console_list_lock(). */
static int unregister_console_locked(struct console *console)
{
	bool use_device_lock = (console->flags & CON_NBCON) && console->write_atomic;
/* …（同文件无关代码省略）… */
	if (console->exit)
		res = console->exit(console);

	/* Changed console list, may require printer threads to start/stop. */
	printk_kthreads_check_locked();

	return res;
}

/* …（同文件无关代码省略）… */
	int res;

	console_list_lock();
	res = unregister_console_locked(console);
	console_list_unlock();
	return res;
}
/* …（同文件无关代码省略）… */

		unregister_console_locked(con);
	}
	console_list_unlock();

	ret = cpuhp_setup_state_nocalls(CPUHP_PRINTK_DEAD, "printk:dead", NULL,
