// AUTO-DRAFT from postgres/postgres PR #ad36e3608c8cb6f0848737ec81e548d4d3a0af3c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static int
build_replindex_scan_key(ScanKey skey, Relation rel, Relation idxrel,
						 TupleTableSlot *searchslot)
{
	int			index_attoff;
	int			skey_attoff = 0;
	Datum		indclassDatum;
	oidvector  *opclass;
	int2vector *indkey = &idxrel->rd_index->indkey;

	indclassDatum = SysCacheGetAttrNotNull(INDEXRELID, idxrel->rd_indextuple,
										   Anum_pg_index_indclass);
	opclass = (oidvector *) DatumGetPointer(indclassDatum);

	/* Build scankey for every non-expression attribute in the index. */
	for (index_attoff = 0; index_attoff < IndexRelationGetNumberOfKeyAttributes(idxrel);
		 index_attoff++)
	{
		Oid			operator;
		Oid			optype;
		Oid			opfamily;
		RegProcedure regop;
		int			table_attno = indkey->values[index_attoff];
		StrategyNumber eq_strategy;

		if (!AttributeNumberIsValid(table_attno))
		{
			/*
			 * XXX: Currently, we don't support expressions in the scan key,
			 * see code below.
			 */
			continue;
		}

		/*
		 * Load the operator info.  We need this to get the equality operator
		 * function for the scan key.
		 */
		optype = get_opclass_input_type(opclass->values[index_attoff]);
		opfamily = get_opclass_family(opclass->values[index_attoff]);
		eq_strategy = IndexAmTranslateCompareType(COMPARE_EQ, idxrel->rd_rel->relam, opfamily, false);
		operator = get_opfamily_member(opfamily, optype,
									   optype,
									   eq_strategy);

		if (!OidIsValid(operator))
			elog(ERROR, "missing operator %d(%u,%u) in opfamily %u",
				 eq_strategy, optype, optype, opfamily);

		regop = get_opcode(operator);

		/* Initialize the scankey. */
		ScanKeyInit(&skey[skey_attoff],
					index_attoff + 1,
					eq_strategy,
					regop,
					searchslot->tts_values[table_attno - 1]);

		skey[skey_attoff].sk_collation = idxrel->rd_indcollation[index_attoff];

		/* Check for null value. */
		if (searchslot->tts_isnull[table_attno - 1])
			skey[skey_attoff].sk_flags |= (SK_ISNULL | SK_SEARCHNULL);

		skey_attoff++;
	}

	/* There must always be at least one attribute for the index scan. */
	Assert(skey_attoff > 0);

	return skey_attoff;
}
/* …（同文件无关代码省略）… */
static bool
should_refetch_tuple(TM_Result res, TM_FailureData *tmfd)
{
	bool		refetch = false;

	switch (res)
	{
		case TM_Ok:
			break;
		case TM_Updated:
			/* XXX: Improve handling here */
			if (ItemPointerIndicatesMovedPartitions(&tmfd->ctid))
				ereport(LOG,
						(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
						 errmsg("tuple to be locked was already moved to another partition due to concurrent update, retrying")));
			else
				ereport(LOG,
						(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
						 errmsg("concurrent update, retrying")));
			refetch = true;
			break;
		case TM_Deleted:
			/* XXX: Improve handling here */
			ereport(LOG,
					(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
					 errmsg("concurrent delete, retrying")));
			refetch = true;
			break;
		case TM_Invisible:
			elog(ERROR, "attempted to lock invisible tuple");
			break;
		default:
			elog(ERROR, "unexpected table_tuple_lock status: %u", res);
			break;
	}

	return refetch;
}
/* …（同文件无关代码省略）… */
 *
 * If a matching tuple is found, lock it with lockmode, fill the slot with its
 * contents, and return true.  Return false otherwise.
 */
bool
RelationFindReplTupleByIndex(Relation rel, Oid idxoid,
							 LockTupleMode lockmode,
							 TupleTableSlot *searchslot,
							 TupleTableSlot *outslot)
{
	ScanKeyData skey[INDEX_MAX_KEYS];
	int			skey_attoff;
	IndexScanDesc scan;
	SnapshotData snap;
	TransactionId xwait;
	Relation	idxrel;
	bool		found;
	TypeCacheEntry **eq = NULL;
	bool		isIdxSafeToSkipDuplicates;

	/* Open the index. */
	idxrel = index_open(idxoid, RowExclusiveLock);

	isIdxSafeToSkipDuplicates = (GetRelationIdentityOrPK(rel) == idxoid);

	InitDirtySnapshot(snap);

	/* Build scan key. */
	skey_attoff = build_replindex_scan_key(skey, rel, idxrel, searchslot);

	/* Start an index scan. */
	scan = index_beginscan(rel, idxrel, false,
						   &snap, NULL, skey_attoff, 0, SO_NONE);

retry:
	found = false;

	index_rescan(scan, skey, skey_attoff, NULL, 0);

	/* Try to find the tuple */
	while (table_index_getnext_slot(scan, ForwardScanDirection, outslot))
	{
		/*
		 * Avoid expensive equality check if the index is primary key or
		 * replica identity index.
		 */
		if (!isIdxSafeToSkipDuplicates)
		{
			if (eq == NULL)
				eq = palloc0_array(TypeCacheEntry *, outslot->tts_tupleDescriptor->natts);

			if (!tuples_equal(outslot, searchslot, eq, NULL))
				continue;
		}

		ExecMaterializeSlot(outslot);

		xwait = TransactionIdIsValid(snap.xmin) ?
			snap.xmin : snap.xmax;

		/*
		 * If the tuple is locked, wait for locking transaction to finish and
		 * retry.
		 */
		if (TransactionIdIsValid(xwait))
		{
			XactLockTableWait(xwait, NULL, NULL, XLTW_None);
			goto retry;
		}

		/* Found our tuple and it's not locked */
		found = true;
		break;
	}

	/* Found tuple, try to lock it in the lockmode. */
	if (found)
	{
		TM_FailureData tmfd;
		TM_Result	res;

		PushActiveSnapshot(GetLatestSnapshot());

		res = table_tuple_lock(rel, &(outslot->tts_tid), GetActiveSnapshot(),
							   outslot,
							   GetCurrentCommandId(false),
							   lockmode,
							   LockWaitBlock,
							   0 /* don't follow updates */ ,
							   &tmfd);

		PopActiveSnapshot();

		if (should_refetch_tuple(res, &tmfd))
			goto retry;
	}

	index_endscan(scan);

	/* Don't release lock until commit. */
	index_close(idxrel, NoLock);

	return found;
}
/* …（同文件无关代码省略）… */
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
