// AUTO-DRAFT from postgres/postgres PR #5a5e3b88dead9cbea2352283addd927a32dc4d4b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static bool
tuples_equal(TupleTableSlot *slot1, TupleTableSlot *slot2,
			 TypeCacheEntry **eq, Bitmapset *columns)
{
	int			attrnum;

	Assert(slot1->tts_tupleDescriptor->natts ==
		   slot2->tts_tupleDescriptor->natts);

	slot_getallattrs(slot1);
	slot_getallattrs(slot2);

	/* Check equality of the attributes. */
	for (attrnum = 0; attrnum < slot1->tts_tupleDescriptor->natts; attrnum++)
	{
		Form_pg_attribute att;
		TypeCacheEntry *typentry;

		att = TupleDescAttr(slot1->tts_tupleDescriptor, attrnum);

		/*
		 * Ignore dropped and generated columns as the publisher doesn't send
		 * those
		 */
		if (att->attisdropped || att->attgenerated)
			continue;

		/*
		 * Ignore columns that are not listed for checking.
		 */
		if (columns &&
			!bms_is_member(att->attnum - FirstLowInvalidHeapAttributeNumber,
						   columns))
			continue;

		/*
		 * If one value is NULL and other is not, then they are certainly not
		 * equal
		 */
		if (slot1->tts_isnull[attrnum] != slot2->tts_isnull[attrnum])
			return false;

		/*
		 * If both are NULL, they can be considered equal.
		 */
		if (slot1->tts_isnull[attrnum] || slot2->tts_isnull[attrnum])
			continue;

		typentry = eq[attrnum];
		if (typentry == NULL)
		{
			typentry = lookup_type_cache(att->atttypid,
										 TYPECACHE_EQ_OPR_FINFO);
			if (!OidIsValid(typentry->eq_opr_finfo.fn_oid))
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_FUNCTION),
						 errmsg("could not identify an equality operator for type %s",
								format_type_be(att->atttypid))));
			eq[attrnum] = typentry;
		}

		if (!DatumGetBool(FunctionCall2Coll(&typentry->eq_opr_finfo,
											att->attcollation,
											slot1->tts_values[attrnum],
											slot2->tts_values[attrnum])))
			return false;
	}

	return true;
}
/* …（同文件无关代码省略）… */
static void
update_most_recent_deletion_info(TupleTableSlot *scanslot,
								 TransactionId oldestxmin,
								 TransactionId *delete_xid,
								 TimestampTz *delete_time,
								 ReplOriginId *delete_origin)
{
	BufferHeapTupleTableSlot *hslot;
	HeapTuple	tuple;
	Buffer		buf;
	bool		recently_dead = false;
	TransactionId xmax;
	TimestampTz localts;
	ReplOriginId localorigin;

	hslot = (BufferHeapTupleTableSlot *) scanslot;

	tuple = ExecFetchSlotHeapTuple(scanslot, false, NULL);
	buf = hslot->buffer;

	LockBuffer(buf, BUFFER_LOCK_SHARE);

	/*
	 * We do not consider HEAPTUPLE_DEAD status because it indicates either
	 * tuples whose inserting transaction was aborted (meaning there is no
	 * commit timestamp or origin), or tuples deleted by a transaction older
	 * than oldestxmin, making it safe to ignore them during conflict
	 * detection (See comments atop worker.c for details).
	 */
	if (HeapTupleSatisfiesVacuum(tuple, oldestxmin, buf) == HEAPTUPLE_RECENTLY_DEAD)
		recently_dead = true;

	LockBuffer(buf, BUFFER_LOCK_UNLOCK);

	if (!recently_dead)
		return;

	xmax = HeapTupleHeaderGetUpdateXid(tuple->t_data);
	if (!TransactionIdIsValid(xmax))
		return;

	/* Select the dead tuple with the most recent commit timestamp */
	if (TransactionIdGetCommitTsData(xmax, &localts, &localorigin) &&
		TimestampDifferenceExceeds(*delete_time, localts, 0))
	{
		*delete_xid = xmax;
		*delete_time = localts;
		*delete_origin = localorigin;
	}
}
/* …（同文件无关代码省略）… */
 * returns the transaction ID, origin, and commit timestamp of the transaction
 * that deleted this tuple.
 *
 * 'oldestxmin' acts as a cutoff transaction ID. Tuples deleted by transactions
 * with IDs >= 'oldestxmin' are considered recently dead and are eligible for
 * conflict detection.
/* …（同文件无关代码省略）… */
 * tuple was deleted most recently.
 */
bool
RelationFindDeletedTupleInfoSeq(Relation rel, TupleTableSlot *searchslot,
								TransactionId oldestxmin,
								TransactionId *delete_xid,
								ReplOriginId *delete_origin,
								TimestampTz *delete_time)
{
	TupleTableSlot *scanslot;
	TableScanDesc scan;
	TypeCacheEntry **eq;
	Bitmapset  *indexbitmap;
	TupleDesc	desc PG_USED_FOR_ASSERTS_ONLY = RelationGetDescr(rel);

	Assert(equalTupleDescs(desc, searchslot->tts_tupleDescriptor));

	*delete_xid = InvalidTransactionId;
	*delete_origin = InvalidReplOriginId;
	*delete_time = 0;

	/*
	 * If the relation has a replica identity key or a primary key that is
	 * unusable for locating deleted tuples (see
	 * IsIndexUsableForFindingDeletedTuple), a full table scan becomes
	 * necessary. In such cases, comparing the entire tuple is not required,
	 * since the remote tuple might not include all column values. Instead,
	 * the indexed columns alone are sufficient to identify the target tuple
	 * (see logicalrep_rel_mark_updatable).
	 */
	indexbitmap = RelationGetIndexAttrBitmap(rel,
											 INDEX_ATTR_BITMAP_IDENTITY_KEY);

	/* fallback to PK if no replica identity */
	if (!indexbitmap)
		indexbitmap = RelationGetIndexAttrBitmap(rel,
												 INDEX_ATTR_BITMAP_PRIMARY_KEY);

	eq = palloc0_array(TypeCacheEntry *, searchslot->tts_tupleDescriptor->natts);

	/*
	 * Start a heap scan using SnapshotAny to identify dead tuples that are
	 * not visible under a standard MVCC snapshot. Tuples from transactions
	 * not yet committed or those just committed prior to the scan are
	 * excluded in update_most_recent_deletion_info().
	 */
	scan = table_beginscan(rel, SnapshotAny, 0, NULL,
						   SO_NONE);
	scanslot = table_slot_create(rel, NULL);

	table_rescan(scan, NULL);

	/* Try to find the tuple */
	while (table_scan_getnextslot(scan, ForwardScanDirection, scanslot))
	{
		if (!tuples_equal(scanslot, searchslot, eq, indexbitmap))
			continue;

		update_most_recent_deletion_info(scanslot, oldestxmin, delete_xid,
										 delete_time, delete_origin);
	}

	table_endscan(scan);
	ExecDropSingleTupleTableSlot(scanslot);

	return *delete_time != 0;
}
