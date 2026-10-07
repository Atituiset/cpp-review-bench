// AUTO-DRAFT from postgres/postgres PR #12f866cd5f72d65ed4435b8203540efde45ba01d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
typedef struct
{
	uint32		mask;
} BackendTypeMask;
/* …（同文件无关代码省略）… */
static inline bool
btmask_contains(BackendTypeMask mask, BackendType t)
{
	return (mask.mask & (1 << t)) != 0;
}
/* …（同文件无关代码省略）… */
typedef enum
{
	PM_INIT,					/* postmaster starting */
	PM_STARTUP,					/* waiting for startup subprocess */
	PM_RECOVERY,				/* in archive recovery mode */
	PM_HOT_STANDBY,				/* in hot standby mode */
	PM_RUN,						/* normal "database is alive" state */
	PM_STOP_BACKENDS,			/* need to stop remaining backends */
	PM_WAIT_BACKENDS,			/* waiting for live backends to exit */
	PM_WAIT_XLOG_SHUTDOWN,		/* waiting for checkpointer to do shutdown
								 * ckpt */
	PM_WAIT_XLOG_ARCHIVAL,		/* waiting for archiver and walsenders to
								 * finish */
	PM_WAIT_IO_WORKERS,			/* waiting for io workers to exit */
	PM_WAIT_CHECKPOINTER,		/* waiting for checkpointer to shut down */
	PM_WAIT_DEAD_END,			/* waiting for dead-end children to exit */
	PM_NO_CHILDREN,				/* all important children have exited */
} PMState;
/* …（同文件无关代码省略）… */
			 */
			ForgetUnstartedBackgroundWorkers();

			SignalChildren(SIGTERM, targetMask);

			UpdatePMState(PM_WAIT_BACKENDS);
		}

		/* Are any of the target processes still running? */
/* …（同文件无关代码省略）… */
static const char *
pmstate_name(PMState state)
{
#define PM_TOSTR_CASE(sym) case sym: return #sym
	switch (state)
	{
			PM_TOSTR_CASE(PM_INIT);
			PM_TOSTR_CASE(PM_STARTUP);
			PM_TOSTR_CASE(PM_RECOVERY);
			PM_TOSTR_CASE(PM_HOT_STANDBY);
			PM_TOSTR_CASE(PM_RUN);
			PM_TOSTR_CASE(PM_STOP_BACKENDS);
			PM_TOSTR_CASE(PM_WAIT_BACKENDS);
			PM_TOSTR_CASE(PM_WAIT_XLOG_SHUTDOWN);
			PM_TOSTR_CASE(PM_WAIT_XLOG_ARCHIVAL);
			PM_TOSTR_CASE(PM_WAIT_IO_WORKERS);
			PM_TOSTR_CASE(PM_WAIT_DEAD_END);
			PM_TOSTR_CASE(PM_WAIT_CHECKPOINTER);
			PM_TOSTR_CASE(PM_NO_CHILDREN);
	}
#undef PM_TOSTR_CASE

	pg_unreachable();
	return "";					/* silence compiler */
}
/* …（同文件无关代码省略）… */
static void
UpdatePMState(PMState newState)
{
	elog(DEBUG1, "updating PMState from %s to %s",
		 pmstate_name(pmState), pmstate_name(newState));
	pmState = newState;
}
/* …（同文件无关代码省略）… */
static const char *
pm_signame(int signal)
{
#define PM_TOSTR_CASE(sym) case sym: return #sym
	switch (signal)
	{
			PM_TOSTR_CASE(SIGABRT);
			PM_TOSTR_CASE(SIGCHLD);
			PM_TOSTR_CASE(SIGHUP);
			PM_TOSTR_CASE(SIGINT);
			PM_TOSTR_CASE(SIGKILL);
			PM_TOSTR_CASE(SIGQUIT);
			PM_TOSTR_CASE(SIGTERM);
			PM_TOSTR_CASE(SIGUSR1);
			PM_TOSTR_CASE(SIGUSR2);
		default:
			/* all signals sent by postmaster should be listed here */
			Assert(false);
			return "(unknown)";
	}
#undef PM_TOSTR_CASE

	return "";					/* silence compiler */
}
/* …（同文件无关代码省略）… */
static void
signal_child(PMChild *pmchild, int signal)
{
	pid_t		pid = pmchild->pid;

	ereport(DEBUG3,
			(errmsg_internal("sending signal %d/%s to %s process with pid %d",
							 signal, pm_signame(signal),
							 GetBackendTypeDesc(pmchild->bkend_type),
							 (int) pmchild->pid)));

	if (kill(pid, signal) < 0)
		elog(DEBUG3, "kill(%ld,%d) failed: %m", (long) pid, signal);
#ifdef HAVE_SETSID
	switch (signal)
	{
		case SIGINT:
		case SIGTERM:
		case SIGQUIT:
		case SIGKILL:
		case SIGABRT:
			if (kill(-pid, signal) < 0)
				elog(DEBUG3, "kill(%ld,%d) failed: %m", (long) (-pid), signal);
			break;
		default:
			break;
	}
#endif
}
/* …（同文件无关代码省略）… */
static bool
SignalChildren(int signal, BackendTypeMask targetMask)
{
	dlist_iter	iter;
	bool		signaled = false;

	dlist_foreach(iter, &ActiveChildList)
	{
		PMChild    *bp = dlist_container(PMChild, elem, iter.cur);

		/*
		 * If we need to distinguish between B_BACKEND and B_WAL_SENDER, check
		 * if any B_BACKEND backends have recently announced that they are
		 * actually WAL senders.
		 */
		if (btmask_contains(targetMask, B_WAL_SENDER) != btmask_contains(targetMask, B_BACKEND) &&
			bp->bkend_type == B_BACKEND)
		{
			if (IsPostmasterChildWalSender(bp->child_slot))
				bp->bkend_type = B_WAL_SENDER;
		}

		if (!btmask_contains(targetMask, bp->bkend_type))
			continue;

		signal_child(bp, signal);
		signaled = true;
	}
	return signaled;
}
