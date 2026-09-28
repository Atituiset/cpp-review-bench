// AUTO-DRAFT from postgres/postgres PR #5bd2e236e21b0e158dda43c184260b9395ede4f4
			GrantAwaitedLock();
	}

	ResetAwaitedLock();

	LWLockRelease(partitionLock);
/* …（同文件无关代码省略）… */
		}
	} while (myWaitStatus == PROC_WAIT_STATUS_WAITING);

	/*
	 * Disable the timers, if they are still running.  As in LockErrorCleanup,
	 * we must preserve the LOCK_TIMEOUT indicator flag: if a lock timeout has
