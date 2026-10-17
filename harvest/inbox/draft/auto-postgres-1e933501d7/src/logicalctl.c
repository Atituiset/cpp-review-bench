// AUTO-DRAFT from postgres/postgres PR #df1be4ddc8345235ffc736adc7dc954f0f944daa
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
static void
write_logical_decoding_status_update_record(bool status)
{
	XLogRecPtr	recptr;

	XLogBeginInsert();
	XLogRegisterData(&status, sizeof(bool));
	recptr = XLogInsert(RM_XLOG_ID, XLOG_LOGICAL_DECODING_STATUS_CHANGE);
	XLogFlush(recptr);
}
/* …（同文件无关代码省略）… */
}

/*
 * Updates the logical decoding status at end of recovery, and ensures that
 * all running processes have the updated XLogLogicalInfo status. This
 * function must be called before accepting writes.
 */
void
UpdateLogicalDecodingStatusEndOfRecovery(void)
{
	bool		new_status = false;

	Assert(RecoveryInProgress());

	LWLockAcquire(LogicalDecodingControlLock, LW_EXCLUSIVE);

	/*
	 * With 'minimal' WAL level, no logical replication slot can exist (see
	 * RestoreSlotFromDisk()), so the new status is always false. However,
	 * logical decoding could have been enabled during recovery by replaying
	 * an XLOG_LOGICAL_DECODING_STATUS_CHANGE record from WAL generated with a
	 * higher wal_level, e.g. if the server crashed right after the last
	 * logical slot was dropped and then restarted with wal_level='minimal'.
	 * The code below disables logical decoding in that case.
	 */
	if (wal_level == WAL_LEVEL_LOGICAL || CheckLogicalSlotExists())
		new_status = true;

	/*
	 * When recovery ends, we need to either enable or disable logical
	 * decoding based on the wal_level setting and the presence of logical
	 * slots. We need to note that concurrent slot creation and deletion could
	 * happen but WAL writes are still not permitted until recovery fully
	 * completes. Here's how we handle concurrent toggling of logical
	 * decoding:
	 *
	 * For 'enable' case, if there's a concurrent disable request before
	 * recovery fully completes, the checkpointer will handle it after
	 * recovery is done. This means there might be a brief period after
	 * recovery where logical decoding remains enabled even with no logical
	 * replication slots present. This temporary state is not new - it can
	 * already occur due to the checkpointer's asynchronous deactivation
	 * process.
	 *
	 * For 'disable' case, a backend concurrently creating a logical slot on a
	 * standby could have passed its CheckLogicalDecodingRequirements() check
	 * when creating its slot only after our slot check above. Such a backend
	 * rechecks the status after creating the slot in
	 * EnsureLogicalDecodingEnabled() and raises an error if logical decoding
	 * has been disabled meanwhile, so it cannot end up with a logical slot
	 * while logical decoding remains disabled. (If recovery has already ended
	 * by the time of the recheck, the backend instead enables logical
	 * decoding by itself, which is fine after promotion.)
	 */
	if (new_status != LogicalDecodingCtl->logical_decoding_enabled)
	{
		/*
		 * Update both the logical decoding status and logical WAL logging
		 * status. Unlike toggling these status during non-recovery, we don't
		 * need to worry about the operation order as WAL writes are still not
		 * permitted.
		 */
		LogicalDecodingCtl->xlog_logical_info = new_status;
		LogicalDecodingCtl->logical_decoding_enabled = new_status;

		elog(DEBUG1,
			 "update logical decoding status to %d at the end of recovery",
			 new_status);

		/*
		 * Now that we updated the logical decoding status, clear the pending
		 * disable flag. It's possible that a concurrent process drops the
		 * last logical slot and initiates the pending disable again. The
		 * checkpointer process will check it.
		 */
		LogicalDecodingCtl->pending_disable = false;

		LWLockRelease(LogicalDecodingControlLock);

		write_logical_decoding_status_update_record(new_status);
	}
	else
		LWLockRelease(LogicalDecodingControlLock);

	/*
	 * Ensure all running processes have the updated status. We don't need to
	 * wait for running transactions to finish as we don't accept any writes
	 * yet. On the other hand, we need to wait for synchronizing
	 * XLogLogicalInfo even if we've not updated the status above as the
	 * status have been turned on and off during recovery, having running
	 * processes have different status on their local caches.
	 */
	if (IsUnderPostmaster)
		WaitForProcSignalBarrier(
								 EmitProcSignalBarrier(PROCSIGNAL_BARRIER_UPDATE_XLOG_LOGICAL_INFO));

	INJECTION_POINT("startup-logical-decoding-status-change-end-of-recovery", NULL);
}
