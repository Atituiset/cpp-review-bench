// AUTO-DRAFT from postgres/postgres PR #1813f951cad59ea7f86063831d0222c429989c91
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct DecodingWorker
{
	/* The worker. */
	BackgroundWorkerHandle *handle;

	/* DecodingWorkerShared is in this segment. */
	dsm_segment *seg;

	/* Handle of the error queue. */
	shm_mq_handle *error_mqh;
} DecodingWorker;
/* …（同文件无关代码省略）… */
	if (concurrent)
		check_concurrent_repack_requirements(OldHeap, &ident_idx);

	/*
	 * Also check the state of indexes; this can abort the command for REPACK.
	 * Historically this hasn't affected CLUSTER or VACUUM FULL, so don't do
/* …（同文件无关代码省略）… */
static void
check_concurrent_repack_requirements(Relation rel, Oid *ident_idx_p)
{
	char		relpersistence,
				replident;
	Oid			ident_idx;

	if (wal_level < WAL_LEVEL_REPLICA)
		ereport(ERROR,
				errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("cannot execute %s in this configuration",
					   "REPACK (CONCURRENTLY)"),
				errdetail("This operation requires \"wal_level\" to be set to \"replica\" or higher."));

	/*
	 * A table AM that doesn't support logical decoding would cause REPACK
	 * (CONCURRENTLY) to silently lose the changes made during the rewrite.
	 * Nothing in TableAmRoutine tells us whether it does, so for now restrict
	 * to heap. Check the routine rather than the AM OID, so that an AM
	 * reusing the heap handler still works.
	 */
	if (rel->rd_tableam != GetHeapamTableAmRoutine())
		ereport(ERROR,
				errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("cannot execute %s on relation \"%s\"",
					   "REPACK (CONCURRENTLY)", RelationGetRelationName(rel)),
				errdetail("This operation is only supported for the \"heap\" access method."));

	/* Data changes in system relations are not logically decoded. */
	if (IsCatalogRelation(rel))
		ereport(ERROR,
				errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("cannot execute %s on relation \"%s\"",
					   "REPACK (CONCURRENTLY)", RelationGetRelationName(rel)),
				errdetail("This operation is not supported for system catalogs."));

	/*
	 * REPACK (CONCURRENTLY) is not MVCC-safe; it doesn't preserve visibility
	 * information, which logical decoding needs because it reads user catalog
	 * tables under a historic snapshot. Removing this check requires making
	 * it MVCC-safe and logical rewrite mappings.
	 */
	if (RelationIsUsedAsCatalogTable(rel))
		ereport(ERROR,
				errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("cannot execute %s on relation \"%s\"",
					   "REPACK (CONCURRENTLY)", RelationGetRelationName(rel)),
				errdetail("This operation is not supported for user catalog tables."));

	/*
	 * reorderbuffer.c does not seem to handle processing of TOAST relation
	 * alone.
	 */
	if (IsToastRelation(rel))
		ereport(ERROR,
				errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("cannot execute %s on relation \"%s\"",
					   "REPACK (CONCURRENTLY)", RelationGetRelationName(rel)),
				errdetail("This operation is not supported for TOAST tables."));

	relpersistence = rel->rd_rel->relpersistence;
	if (relpersistence != RELPERSISTENCE_PERMANENT)
		ereport(ERROR,
				errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				errmsg("cannot execute %s on relation \"%s\"",
					   "REPACK (CONCURRENTLY)", RelationGetRelationName(rel)),
				errdetail("This operation is only supported for permanent relations."));

	/* A materialized view produces no logically decoded changes. */
	if (rel->rd_rel->relkind == RELKIND_MATVIEW)
		ereport(ERROR,
				errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("cannot execute %s on relation \"%s\"",
					   "REPACK (CONCURRENTLY)", RelationGetRelationName(rel)),
				errdetail_relkind_not_supported(rel->rd_rel->relkind));

	/*
	 * With NOTHING, WAL does not contain the old tuple; FULL is not yet
	 * supported.
	 */
	replident = rel->rd_rel->relreplident;
	if (replident == REPLICA_IDENTITY_NOTHING ||
		replident == REPLICA_IDENTITY_FULL)
		ereport(ERROR,
				errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				errmsg("cannot execute %s on relation \"%s\"",
					   "REPACK (CONCURRENTLY)", RelationGetRelationName(rel)),
				errdetail("This operation does not support tables with %s.",
						  replident == REPLICA_IDENTITY_NOTHING ?
						  "REPLICA IDENTITY NOTHING" : "REPLICA IDENTITY FULL"));

	/*
	 * Obtain the replica identity index to use.  If there isn't one, the
	 * table cannot be repacked concurrently.  (Replica identity FULL is not
	 * supported yet.)
	 */
	ident_idx = RelationGetReplicaIndex(rel);
	if (!OidIsValid(ident_idx))
	{
		/* This special case warrants its own error message */
		if (OidIsValid(rel->rd_pkindex) && rel->rd_ispkdeferrable)
			ereport(ERROR,
					errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("cannot execute %s on relation \"%s\"",
						   "REPACK (CONCURRENTLY)",
						   RelationGetRelationName(rel)),
					errdetail("This operation does not support deferrable primary keys."),
					errhint("Use ALTER TABLE ... REPLICA IDENTITY USING INDEX to designate another index as replica identity."));

		ereport(ERROR,
				errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				errmsg("cannot execute %s on relation \"%s\"",
					   "REPACK (CONCURRENTLY)", RelationGetRelationName(rel)),
				errdetail("Relation \"%s\" has no identity index.",
						  RelationGetRelationName(rel)));
	}

	*ident_idx_p = ident_idx;
}
/* …（同文件无关代码省略）… */
		 */
		BecomeLockGroupLeader();

		/*
		 * Start the worker that decodes data changes applied while we're
		 * copying the table contents.
		 *
		 * Note that the worker has to wait for all transactions with XID
		 * already assigned to finish. If some of those transactions is
		 * waiting for a lock conflicting with ShareUpdateExclusiveLock on our
		 * table (e.g.  it runs CREATE INDEX), we can end up in a deadlock.
		 * Not sure this risk is worth unlocking/locking the table (and its
		 * clustering index) and checking again if it's still eligible for
		 * REPACK CONCURRENTLY.
		 */
		start_repack_decoding_worker(tableOid);

/* …（同文件无关代码省略）… */
	 *
	 * We don't need to open the toast relation here, just lock it.  The lock
	 * will be held till end of transaction.
	 */
	if (OldHeap->rd_rel->reltoastrelid)
		LockRelationOid(OldHeap->rd_rel->reltoastrelid, lmode);

	/*
	 * If both tables have TOAST tables, perform toast swap by content.  It is
/* …（同文件无关代码省略）… */
static void
start_repack_decoding_worker(Oid relid)
{
	Size		size;
	DecodingWorkerShared *shared;
	shm_mq	   *mq;
	BackgroundWorker bgw;

	decoding_worker = palloc0_object(DecodingWorker);

	/* Setup shared memory. */
	size = BUFFERALIGN(offsetof(DecodingWorkerShared, error_queue)) +
		BUFFERALIGN(REPACK_ERROR_QUEUE_SIZE);
	decoding_worker->seg = dsm_create(size, 0);

	shared = (DecodingWorkerShared *) dsm_segment_address(decoding_worker->seg);
	shared->initialized = false;
	shared->lsn_upto = InvalidXLogRecPtr;
	shared->done = false;
	SharedFileSetInit(&shared->sfs, decoding_worker->seg);
	shared->last_exported = -1;
	SpinLockInit(&shared->mutex);
	shared->dbid = MyDatabaseId;

	/*
	 * This is the UserId set in cluster_rel(). Security context shouldn't be
	 * needed for decoding worker.
	 */
	shared->roleid = GetUserId();
	shared->relid = relid;
	ConditionVariableInit(&shared->cv);
	shared->backend_pid = MyProcPid;
	shared->backend_proc_number = MyProcNumber;

	/* Transmit our timeouts to the worker too */
	shared->lock_timeout = LockTimeout;
	shared->transaction_timeout = TransactionTimeout;

	mq = shm_mq_create((char *) BUFFERALIGN(shared->error_queue),
					   REPACK_ERROR_QUEUE_SIZE);
	shm_mq_set_receiver(mq, MyProc);

	decoding_worker->error_mqh = shm_mq_attach(mq, decoding_worker->seg, NULL);

	memset(&bgw, 0, sizeof(bgw));
	snprintf(bgw.bgw_name, BGW_MAXLEN,
			 "REPACK decoding worker for relation \"%s\"",
			 get_rel_name(relid));
	snprintf(bgw.bgw_type, BGW_MAXLEN, "REPACK decoding worker");
	bgw.bgw_flags = BGWORKER_SHMEM_ACCESS |
		BGWORKER_BACKEND_DATABASE_CONNECTION;
	bgw.bgw_start_time = BgWorkerStart_RecoveryFinished;
	bgw.bgw_restart_time = BGW_NEVER_RESTART;
	snprintf(bgw.bgw_library_name, MAXPGPATH, "postgres");
	snprintf(bgw.bgw_function_name, BGW_MAXLEN, "RepackWorkerMain");
	bgw.bgw_main_arg = UInt32GetDatum(dsm_segment_handle(decoding_worker->seg));
	bgw.bgw_notify_pid = MyProcPid;

	if (!RegisterDynamicBackgroundWorker(&bgw, &decoding_worker->handle))
		ereport(ERROR,
				errcode(ERRCODE_CONFIGURATION_LIMIT_EXCEEDED),
				errmsg("out of background worker slots"),
				errhint("You might need to increase \"%s\".", "max_worker_processes"));

	/*
	 * Now that the worker is registered, connect the error message queue to
	 * it.
	 */
	shm_mq_set_handle(decoding_worker->error_mqh, decoding_worker->handle);

	/*
	 * Make sure the worker has started before we wait for it to initialize
	 * decoding below, so that the failure-to-start case does not hang
	 * forever.
	 */
	wait_for_repack_decoding_worker();

	/*
	 * The decoding setup must be done before the caller can have XID assigned
	 * for any reason, otherwise the worker might end up in a deadlock,
	 * waiting for the caller's transaction to end. Therefore wait here until
	 * the worker indicates that it has the logical decoding initialized.
	 */
	ConditionVariablePrepareToSleep(&shared->cv);
	for (;;)
	{
		bool		initialized;

		SpinLockAcquire(&shared->mutex);
		initialized = shared->initialized;
		SpinLockRelease(&shared->mutex);

		if (initialized)
			break;

		ConditionVariableSleep(&shared->cv, WAIT_EVENT_REPACK_WORKER_EXPORT);
	}
	ConditionVariableCancelSleep();
}
/* …（同文件无关代码省略）… */
static void
wait_for_repack_decoding_worker(void)
{
	for (;;)
	{
		BgwHandleStatus status;
		shm_mq	   *mq;
		int			rc;
		pid_t		pid;

		/*
		 * This will process any repack messages that are pending and it may
		 * also throw an error propagated from a worker.
		 */
		CHECK_FOR_INTERRUPTS();

		/* If error_mqh is NULL, the worker has exited cleanly */
		if (decoding_worker->error_mqh == NULL)
			break;

		status = GetBackgroundWorkerPid(decoding_worker->handle, &pid);
		if (status == BGWH_STARTED)
		{
			/* Has the worker attached to the error message queue? */
			mq = shm_mq_get_queue(decoding_worker->error_mqh);
			if (shm_mq_get_sender(mq) != NULL)
				break;
		}
		else if (status == BGWH_STOPPED)
		{
			/*
			 * If the worker stopped without attaching to the error message
			 * queue, throw an error. Otherwise, assume it attached and
			 * reported an error before exiting, so mark it attached and let
			 * the next attempt to process pending messages, here or later
			 * while the initial snapshot is set up, throw that error.
			 */
			mq = shm_mq_get_queue(decoding_worker->error_mqh);
			if (shm_mq_get_sender(mq) == NULL)
				ereport(ERROR,
						errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
						errmsg("REPACK decoding worker failed to start"),
						errhint("More details may be available in the server log."));
			break;
		}

		/* Worker neither started or stopped yet, so wait. */
		rc = WaitLatch(MyLatch,
					   WL_LATCH_SET | WL_EXIT_ON_PM_DEATH,
					   -1, WAIT_EVENT_BGWORKER_STARTUP);

		if (rc & WL_LATCH_SET)
			ResetLatch(MyLatch);
	}
}
