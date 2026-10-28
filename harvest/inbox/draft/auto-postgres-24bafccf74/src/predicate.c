// AUTO-DRAFT from postgres/postgres PR #fde4444b48c8dc069db2e29d096dbea45a158cc9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>  // <<< BUG ANCHOR
#include <stddef.h>
#include <stdlib.h>

 *
 * predicate lock reporting
 *		GetPredicateLockStatusData(void)
 *		PageIsPredicateLocked(Relation relation, BlockNumber blkno)
 *
 * predicate lock maintenance
 *		GetSerializableTransactionSnapshot(Snapshot snapshot)
/* …（同文件无关代码省略）… */
#define PredicateLockHashPartition(hashcode) \
	((hashcode) % NUM_PREDICATELOCK_PARTITIONS)
#define PredicateLockHashPartitionLock(hashcode) \
	(&MainLWLockArray[PREDICATELOCK_MANAGER_LWLOCK_OFFSET + \
		PredicateLockHashPartition(hashcode)].lock)
/* …（同文件无关代码省略）… */
#define PredicateLockTargetTagHashCode(predicatelocktargettag) \
	get_hash_value(PredicateLockTargetHash, predicatelocktargettag)
/* …（同文件无关代码省略）… */
}


/*
 * Check whether there are any predicate locks held by any transaction
 * for the page at the given block number.
 *
 * Note that the transaction may be completed but not yet subject to
 * cleanup due to overlapping serializable transactions.  This must
 * return valid information regardless of transaction isolation level.
 *
 * Also note that this doesn't check for a conflicting relation lock,
 * just a lock specifically on the given page.
 *
 * One use is to support proper behavior during GiST index vacuum.
 */
bool
PageIsPredicateLocked(Relation relation, BlockNumber blkno)
{
	PREDICATELOCKTARGETTAG targettag;
	uint32		targettaghash;
	LWLock	   *partitionLock;
	PREDICATELOCKTARGET *target;

	SET_PREDICATELOCKTARGETTAG_PAGE(targettag,
									relation->rd_locator.dbOid,
									relation->rd_id,
									blkno);

	targettaghash = PredicateLockTargetTagHashCode(&targettag);
	partitionLock = PredicateLockHashPartitionLock(targettaghash);
	LWLockAcquire(partitionLock, LW_SHARED);
	target = (PREDICATELOCKTARGET *)
		hash_search_with_hash_value(PredicateLockTargetHash,
									&targettag, targettaghash,
									HASH_FIND, NULL);
	LWLockRelease(partitionLock);

	return (target != NULL);
}


/*
 * Check whether a particular lock is held by this transaction.
 *
