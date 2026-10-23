// AUTO-DRAFT from postgres/postgres PR #219334a2252729054db98288df64e61b2968f2b7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define REMOTE_SEQ_COL_COUNT 11
/* …（同文件无关代码省略）… */
typedef enum CopySeqResult
{
	COPYSEQ_SUCCESS,
	COPYSEQ_MISMATCH,
	COPYSEQ_SUBSCRIBER_INSUFFICIENT_PERM,
	COPYSEQ_PUBLISHER_INSUFFICIENT_PERM,
	COPYSEQ_SKIPPED,
	COPYSEQ_NOT_SUBSCRIBED
} CopySeqResult;
/* …（同文件无关代码省略）… */
	}
}

/*
 * report_sequence_errors
 *
/* …（同文件无关代码省略）… */
static CopySeqResult
get_and_validate_seq_info(TupleTableSlot *slot, Relation *sequence_rel,
						  LogicalRepSequenceInfo **seqinfo, int *seqidx)
{
	bool		isnull;
	int			col = 0;
	Datum		datum;
	bool		remote_has_select_priv;
	Oid			remote_typid;
	int64		remote_start;
	int64		remote_increment;
	int64		remote_min;
	int64		remote_max;
	bool		remote_cycle;
	CopySeqResult result = COPYSEQ_SUCCESS;
	HeapTuple	tup;
	Form_pg_sequence local_seq;
	LogicalRepSequenceInfo *seqinfo_local;

	*seqidx = DatumGetInt32(slot_getattr(slot, ++col, &isnull));
	Assert(!isnull);

	/*
	 * The publisher only echoes back an index that we put in the VALUES list,
	 * so this should always identify an entry of seqinfos. Check it anyway
	 * before using it as a list subscript, since list_nth() does not
	 * bounds-check on non-assert builds and we would then write the remote
	 * sequence state through a pointer fetched from beyond the list.
	 *
	 * This only keeps the subscript inside the list. An index that is wrong
	 * but still in range is not detected, and cannot be; the sequence it
	 * points at then receives another sequence's data. That is the same kind
	 * of damage as the publisher reporting a wrong value in any other column,
	 * and is likewise beyond what we can check.
	 */
	if (*seqidx < 0 || *seqidx >= list_length(seqinfos))
		elog(ERROR, "invalid sequence index %d received from the publisher",
			 *seqidx);

	/* Identify the corresponding local sequence for the given index. */
	*seqinfo = seqinfo_local =
		(LogicalRepSequenceInfo *) list_nth(seqinfos, *seqidx);

	/*
	 * has_sequence_privilege() itself returns NULL, rather than false, when
	 * the sequence has been dropped concurrently after it was identified in
	 * the catalog snapshot (see has_sequence_privilege_id()). Treat that as a
	 * missing sequence on the publisher.
	 */
	datum = slot_getattr(slot, ++col, &isnull);
	if (isnull)
		return COPYSEQ_SKIPPED;

	remote_has_select_priv = DatumGetBool(datum);

	/*
	 * The remote sequence state can be NULL if the publisher lacks the
	 * required privileges or if the sequence was dropped concurrently after
	 * it was identified in the catalog snapshot (see pg_get_sequence_data()).
	 */
	datum = slot_getattr(slot, ++col, &isnull);
	if (isnull)
	{
		/*
		 * The sequence was dropped concurrently after it was identified in
		 * the catalog snapshot. Treat it as skipped (and, since it no longer
		 * exists on the publisher, ultimately missing).
		 */
		if (remote_has_select_priv)
			return COPYSEQ_SKIPPED;

		/*
		 * The publisher lacks the SELECT privilege required by
		 * pg_get_sequence_data(). Since has_sequence_privilege() returned
		 * false, not NULL, do not classify this sequence as missing on the
		 * publisher.
		 */
		seqinfo_local->found_on_pub = true;
		return COPYSEQ_PUBLISHER_INSUFFICIENT_PERM;
	}

	seqinfo_local->last_value = DatumGetInt64(datum);

	seqinfo_local->is_called = DatumGetBool(slot_getattr(slot, ++col, &isnull));
	Assert(!isnull);

	seqinfo_local->page_lsn = DatumGetLSN(slot_getattr(slot, ++col, &isnull));
	Assert(!isnull);

	remote_typid = DatumGetObjectId(slot_getattr(slot, ++col, &isnull));
	Assert(!isnull);

	remote_start = DatumGetInt64(slot_getattr(slot, ++col, &isnull));
	Assert(!isnull);

	remote_increment = DatumGetInt64(slot_getattr(slot, ++col, &isnull));
	Assert(!isnull);

	remote_min = DatumGetInt64(slot_getattr(slot, ++col, &isnull));
	Assert(!isnull);

	remote_max = DatumGetInt64(slot_getattr(slot, ++col, &isnull));
	Assert(!isnull);

	remote_cycle = DatumGetBool(slot_getattr(slot, ++col, &isnull));
	Assert(!isnull);

	/* Sanity check */
	Assert(col == REMOTE_SEQ_COL_COUNT);

	seqinfo_local->found_on_pub = true;

	*sequence_rel = try_table_open(seqinfo_local->localrelid, RowExclusiveLock);

	/* Sequence was concurrently dropped? */
	if (!*sequence_rel)
		return COPYSEQ_SKIPPED;

	tup = SearchSysCache1(SEQRELID, ObjectIdGetDatum(seqinfo_local->localrelid));

	/* Sequence was concurrently dropped? */
	if (!HeapTupleIsValid(tup))
		elog(ERROR, "cache lookup failed for sequence %u",
			 seqinfo_local->localrelid);

	local_seq = (Form_pg_sequence) GETSTRUCT(tup);

	/* Sequence parameters for remote/local are the same? */
	if (local_seq->seqtypid != remote_typid ||
		local_seq->seqstart != remote_start ||
		local_seq->seqincrement != remote_increment ||
		local_seq->seqmin != remote_min ||
		local_seq->seqmax != remote_max ||
		local_seq->seqcycle != remote_cycle)
		result = COPYSEQ_MISMATCH;

	/* Sequence was concurrently renamed? */
	if (strcmp(seqinfo_local->nspname,
			   get_namespace_name(RelationGetNamespace(*sequence_rel))) ||
		strcmp(seqinfo_local->seqname, RelationGetRelationName(*sequence_rel)))
		result = COPYSEQ_MISMATCH;

	ReleaseSysCache(tup);
	return result;
}
/* …（同文件无关代码省略）… */
static CopySeqResult
copy_sequence(LogicalRepSequenceInfo *seqinfo, Oid seqowner)
{
	UserContext ucxt;
	AclResult	aclresult;
	bool		run_as_owner = MySubscription->runasowner;
	Oid			seqoid = seqinfo->localrelid;
	Relation	rel;

	/*
	 * Take the subscription object lock before checking whether this sequence
	 * is still part of the subscription. The lock is held until the end of
	 * the transaction, so the check and the state update below are protected
	 * from a concurrent ALTER SUBSCRIPTION ... REFRESH PUBLICATION.
	 *
	 * AlterSubscription() takes this lock in AccessExclusiveLock mode while
	 * removing pg_subscription_rel rows, so the row cannot be removed between
	 * the check and the state update.
	 */
	LockSharedObject(SubscriptionRelationId, MySubscription->oid, 0,
					 AccessShareLock);

	/*
	 * The sequence may no longer be part of the subscription, in which case
	 * there is nothing to synchronize and the caller just skips it.
	 */
	if (!SearchSysCacheExists2(SUBSCRIPTIONRELMAP,
							   ObjectIdGetDatum(seqoid),
							   ObjectIdGetDatum(MySubscription->oid)))
		return COPYSEQ_NOT_SUBSCRIBED;

	/*
	 * If the user did not opt to run as the owner of the subscription
	 * ('run_as_owner'), then copy the sequence as the owner of the sequence.
	 */
	if (!run_as_owner)
		SwitchToUntrustedUser(seqowner, &ucxt);

	aclresult = pg_class_aclcheck(seqoid, GetUserId(), ACL_UPDATE);

	if (aclresult != ACLCHECK_OK)
	{
		if (!run_as_owner)
			RestoreUserContext(&ucxt);

		return COPYSEQ_SUBSCRIBER_INSUFFICIENT_PERM;
	}

	/*
	 * The log counter (log_cnt) tracks how many sequence values are still
	 * unused locally. It is only relevant to the local node and managed
	 * internally by nextval() when allocating new ranges. Since log_cnt does
	 * not affect the visible sequence state (like last_value or is_called)
	 * and is only used for local caching, it need not be copied to the
	 * subscriber during synchronization.
	 */
	SetSequence(seqoid, seqinfo->last_value, seqinfo->is_called);

	if (!run_as_owner)
		RestoreUserContext(&ucxt);

	rel = table_open(SubscriptionRelRelationId, RowExclusiveLock);

	/*
	 * Record the remote sequence's LSN in pg_subscription_rel and mark the
	 * sequence as READY. Both locks it needs are held already, the object
	 * lock from further up and the relation lock just taken, so say so rather
	 * than have it take and release them again.
	 */
	UpdateSubscriptionRelState(MySubscription->oid, seqoid, SUBREL_STATE_READY,
							   seqinfo->page_lsn, true);

	table_close(rel, NoLock);

	return COPYSEQ_SUCCESS;
}
/* …（同文件无关代码省略）… */
		while (tuplestore_gettupleslot(res->tuplestore, true, false, slot))
		{
			CopySeqResult sync_status;
			LogicalRepSequenceInfo *seqinfo;
			Relation	sequence_rel = NULL;
			int			seqidx;

			CHECK_FOR_INTERRUPTS();

/* …（同文件无关代码省略）… */
				ProcessConfigFile(PGC_SIGHUP);
			}

			sync_status = get_and_validate_seq_info(slot, &sequence_rel,
													&seqinfo, &seqidx);
			if (sync_status == COPYSEQ_SUCCESS)
				sync_status = copy_sequence(seqinfo,
											sequence_rel->rd_rel->relowner);

			switch (sync_status)
			{
				case COPYSEQ_SUCCESS:
