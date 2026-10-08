// AUTO-DRAFT from postgres/postgres PR #1f2245371e31f0def347546f5ad250552a4eb8a4
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
		return false;
	}

	/*
	 * Do not persist the slot if logical decoding got disabled concurrently.
	 * This can happen if the last logical slot on the primary was dropped and
	 * the corresponding XLOG_LOGICAL_DECODING_STATUS_CHANGE record was
	 * replayed after we fetched the remote slot information: WAL records
	 * following the slot's restart_lsn might lack the information required by
	 * logical decoding, and the slot invalidation performed when replaying
	 * the record could not find our slot as it was not created yet.
	 *
	 * It is important to perform this check after creating the slot and
	 * before persisting it. This way, even if the status change record is
	 * replayed after this check, the replay will invalidate our slot.
	 *
	 * If the check fails, we keep the temporary slot and let the caller
	 * retry; the next cycle fetches the remote slot information again and
	 * will drop this slot as the remote slot no longer exists.
	 *
	 * XXX: this check cannot detect the case where logical decoding is
	 * already re-enabled by a slot creation on the primary at this point.
	 * Detecting that would require comparing the slot's restart_lsn with the
	 * LSN at which logical decoding was last enabled.
	 */
	if (!IsLogicalDecodingEnabled())
	{
		ereport(LOG,
				errmsg("could not synchronize replication slot \"%s\"",
					   remote_slot->name),
				errdetail("Logical decoding was concurrently disabled."));

		if (slot_persistence_pending)
			*slot_persistence_pending = true;

		return false;
	}

	ReplicationSlotPersist();

	ereport(LOG,
/* …（同文件无关代码省略）… */
	{
		NameData	plugin_name;
		TransactionId xmin_horizon = InvalidTransactionId;

		/* Skip creating the local slot if remote_slot is invalidated already */
		if (remote_slot->invalidated != RS_INVAL_NONE)
/* …（同文件无关代码省略）… */
							  remote_slot->failover,
							  true);

		/* For shorter lines. */
		slot = MyReplicationSlot;
