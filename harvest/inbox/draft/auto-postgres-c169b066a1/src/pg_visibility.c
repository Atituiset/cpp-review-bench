// AUTO-DRAFT from postgres/postgres PR #1ed083f90d29871b21d5b3d2351accf4a0d9ca81
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include "access/heapam.h"
#include "access/htup_details.h"
#include "access/visibilitymap.h"
#include "access/xloginsert.h"
#include "catalog/pg_type.h"
#include "catalog/storage_xlog.h"
/* …（同文件无关代码省略）… */
typedef struct vbits
{
	BlockNumber next;
	BlockNumber count;
	pg_attribute_counted_by(count) uint8 bits[FLEXIBLE_ARRAY_MEMBER];
} vbits;
/* …（同文件无关代码省略）… */
typedef struct corrupt_items
{
	BlockNumber next;
	BlockNumber count;
	ItemPointer tids;
} corrupt_items;
/* …（同文件无关代码省略）… */
static void record_corrupt_item(corrupt_items *items, ItemPointer tid);
static bool tuple_all_visible(HeapTuple tup, TransactionId OldestXmin,
							  Buffer buffer);
static void check_relation_relkind(Relation rel);

/*
 * Visibility map information for a single block of a relation.
/* …（同文件无关代码省略）… */
	rel = relation_open(relid, AccessShareLock);

	/* Only some relkinds have a visibility map */
	check_relation_relkind(rel);

	if (blkno < 0 || blkno > MaxBlockNumber)
		ereport(ERROR,
/* …（同文件无关代码省略）… */
	rel = relation_open(relid, AccessShareLock);

	/* Only some relkinds have a visibility map */
	check_relation_relkind(rel);

	if (blkno < 0 || blkno > MaxBlockNumber)
		ereport(ERROR,
/* …（同文件无关代码省略）… */
	rel = relation_open(relid, AccessShareLock);

	/* Only some relkinds have a visibility map */
	check_relation_relkind(rel);

	visibilitymap_count(rel, &all_visible, &all_frozen);

/* …（同文件无关代码省略）… */
	rel = relation_open(relid, AccessExclusiveLock);

	/* Only some relkinds have a visibility map */
	check_relation_relkind(rel);

	/* Forcibly reset cached file size */
	RelationGetSmgr(rel)->smgr_cached_nblocks[VISIBILITYMAP_FORKNUM] = InvalidBlockNumber;
/* …（同文件无关代码省略）… */
	rel = relation_open(relid, AccessShareLock);

	/* Only some relkinds have a visibility map */
	check_relation_relkind(rel);

	nblocks = RelationGetNumberOfBlocks(rel);
	info = palloc0(offsetof(vbits, bits) + nblocks);
/* …（同文件无关代码省略）… */
static TransactionId
GetStrictOldestNonRemovableTransactionId(Relation rel)
{
	RunningTransactions runningTransactions;

	if (RecoveryInProgress())
	{
		TransactionId result;

		/* As we ignore KnownAssignedXids on standby, just pick nextXid */
		LWLockAcquire(XidGenLock, LW_SHARED);
		result = XidFromFullTransactionId(TransamVariables->nextXid);
		LWLockRelease(XidGenLock);
		return result;
	}
	else if (rel == NULL || rel->rd_rel->relisshared)
	{
		/* Shared relation: take into account all running xids */
		runningTransactions = GetRunningTransactionData();
		LWLockRelease(ProcArrayLock);
		LWLockRelease(XidGenLock);
		return runningTransactions->oldestRunningXid;
	}
	else if (!RELATION_IS_LOCAL(rel))
	{
		/*
		 * Normal relation: take into account xids running within the current
		 * database
		 */
		runningTransactions = GetRunningTransactionData();
		LWLockRelease(ProcArrayLock);
		LWLockRelease(XidGenLock);
		return runningTransactions->oldestDatabaseRunningXid;
	}
	else
	{
		/*
		 * For temporary relations, ComputeXidHorizons() uses only
		 * TransamVariables->latestCompletedXid and MyProc->xid.  These two
		 * shouldn't go backwards.  So we're fine with this horizon.
		 */
		return GetOldestNonRemovableTransactionId(rel);
	}
}
/* …（同文件无关代码省略）… */
	rel = relation_open(relid, AccessShareLock);

	/* Only some relkinds have a visibility map */
	check_relation_relkind(rel);

	if (all_visible)
		OldestXmin = GetStrictOldestNonRemovableTransactionId(rel);
/* …（同文件无关代码省略）… */
static void
record_corrupt_item(corrupt_items *items, ItemPointer tid)
{
	/* enlarge output array if needed. */
	if (items->next >= items->count)
	{
		items->count *= 2;
		items->tids = repalloc_array(items->tids, ItemPointerData, items->count);
	}
	/* and add the new item */
	items->tids[items->next++] = *tid;
}
/* …（同文件无关代码省略）… */
static bool
tuple_all_visible(HeapTuple tup, TransactionId OldestXmin, Buffer buffer)
{
	HTSV_Result state;
	TransactionId xmin;

	state = HeapTupleSatisfiesVacuum(tup, OldestXmin, buffer);
	if (state != HEAPTUPLE_LIVE)
		return false;			/* all-visible implies live */

	/*
	 * Neither lazy_scan_heap nor heap_page_is_all_visible will mark a page
	 * all-visible unless every tuple is hinted committed. However, those hint
	 * bits could be lost after a crash, so we can't be certain that they'll
	 * be set here.  So just check the xmin.
	 */

	xmin = HeapTupleHeaderGetXmin(tup->t_data);
	if (!TransactionIdPrecedes(xmin, OldestXmin))
		return false;			/* xmin not old enough for all to see */

	return true;
}

/*
 * check_relation_relkind - convenience routine to check that relation
 * is of the relkind supported by the callers
 */
static void
check_relation_relkind(Relation rel)
{
	if (!RELKIND_HAS_TABLE_AM(rel->rd_rel->relkind))
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("relation \"%s\" is of wrong relation kind",
						RelationGetRelationName(rel)),
				 errdetail_relkind_not_supported(rel->rd_rel->relkind)));
}
