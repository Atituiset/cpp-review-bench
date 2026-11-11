// AUTO-DRAFT from postgres/postgres PR #5bf919736b20ffdc8d646c33ed2d9054609b23ec
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR

/*
 * Terminate all background workers connected to the given database, if the
 * workers can be interrupted.
 */
void
TerminateBackgroundWorkersForDatabase(Oid databaseId)
{
	bool		signal_postmaster = false;

	elog(DEBUG1, "attempting worker termination for database %u",
		 databaseId);

	LWLockAcquire(BackgroundWorkerLock, LW_EXCLUSIVE);

	/*
	 * Iterate through slots, looking for workers connected to the given
/* …（同文件无关代码省略）… */
			LWLockAcquire(ProcArrayLock, LW_SHARED);
			proc = BackendPidGetProcWithLock(pid);
			if (proc && proc->databaseId == databaseId)
			{
				slot->terminate = true;
				signal_postmaster = true;

				elog(DEBUG1, "termination requested for worker (PID %d) on database %u",
					 (int) pid, databaseId);
			}
			LWLockRelease(ProcArrayLock);
		}
	}

	LWLockRelease(BackgroundWorkerLock);

	/* Make sure the postmaster notices the change to shared memory. */
	if (signal_postmaster)
		SendPostmasterSignal(PMSIGNAL_BACKGROUND_WORKER_CHANGE);
}
