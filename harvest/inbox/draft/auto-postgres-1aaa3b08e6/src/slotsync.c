// AUTO-DRAFT from postgres/postgres PR #ba11c48390004ed49b889cbd718c9e5f691fe493
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static void
update_slotsync_skip_stats(SlotSyncSkipReason skip_reason)
{
	ReplicationSlot *slot;

	Assert(MyReplicationSlot);

	slot = MyReplicationSlot;

	/*
	 * Update the slot sync related stats in pg_stat_replication_slots when a
	 * slot sync is skipped
	 */
	if (skip_reason != SS_SKIP_NONE)
		pgstat_report_replslotsync(slot);

	/* Update the slot sync skip reason */
	if (slot->slotsync_skip_reason != skip_reason)
	{
		SpinLockAcquire(&slot->mutex);
		slot->slotsync_skip_reason = skip_reason;
		SpinLockRelease(&slot->mutex);
	}
}
/* …（同文件无关代码省略）… */
	ReplicationSlot *slot = MyReplicationSlot;
	bool		updated_xmin_or_lsn = false;
	bool		updated_config = false;
	SlotSyncSkipReason skip_reason = SS_SKIP_NONE;
	XLogRecPtr	latestFlushPtr = GetStandbyFlushRecPtr(NULL);

	Assert(slot->data.invalidated == RS_INVAL_NONE);

	/*
	 * Make sure that concerned WAL is received and flushed before syncing
	 * slot to target lsn received from the primary server.
/* …（同文件无关代码省略）… */
			SpinLockRelease(&slot->mutex);

			updated_xmin_or_lsn = true;
		}
		else
		{
			bool		found_consistent_snapshot;
			XLogRecPtr	old_confirmed_lsn = slot->data.confirmed_flush;
			XLogRecPtr	old_restart_lsn = slot->data.restart_lsn;
			TransactionId old_catalog_xmin = slot->data.catalog_xmin;

			LogicalSlotAdvanceAndCheckSnapState(remote_slot->confirmed_lsn,
												&found_consistent_snapshot);

			/* Sanity check */
			if (slot->data.confirmed_flush != remote_slot->confirmed_lsn)
/* …（同文件无关代码省略）… */
										   LSN_FORMAT_ARGS(remote_slot->confirmed_lsn),
										   LSN_FORMAT_ARGS(slot->data.confirmed_flush)));

			/*
			 * If we can't reach a consistent snapshot, the slot won't be
			 * persisted. See update_and_persist_local_synced_slot().
			 */
			if (!found_consistent_snapshot)
			{
				Assert(MyReplicationSlot->data.persistency == RS_TEMPORARY);

				ereport(LOG,
						errmsg("could not synchronize replication slot \"%s\"",
							   remote_slot->name),
						errdetail("Synchronization could lead to data loss, because the standby could not build a consistent snapshot to decode WALs at LSN %X/%08X.",
								  LSN_FORMAT_ARGS(slot->data.restart_lsn)));

				skip_reason = SS_SKIP_NO_CONSISTENT_SNAPSHOT;
			}

			/*
			 * It is possible that the slot's xmin or LSNs are not updated,
/* …（同文件无关代码省略）… */
		}
	}

	/* Update slot sync skip stats */
	update_slotsync_skip_stats(skip_reason);
