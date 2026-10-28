// AUTO-DRAFT from postgres/postgres PR #4848fd6752ad6b88c5a066442df35ba208dfcec5
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

 *
 *-------------------------------------------------------------------------
 */
/*
 * INTERFACE ROUTINES
 *
 * predicate lock reporting
 *		GetPredicateLockStatusData(void)
 *
 * predicate lock maintenance
 *		GetSerializableTransactionSnapshot(Snapshot snapshot)
 *		SetSerializableTransactionSnapshot(Snapshot snapshot,
 *										   VirtualTransactionId *sourcevxid)
 *		RegisterPredicateLockingXid(void)
 *		PredicateLockRelation(Relation relation, Snapshot snapshot)
 *		PredicateLockPage(Relation relation, BlockNumber blkno,
 *						Snapshot snapshot)
 *		PredicateLockTID(Relation relation, const ItemPointerData *tid, Snapshot snapshot,
 *						 TransactionId tuple_xid)
 *		PredicateLockPageSplit(Relation relation, BlockNumber oldblkno,
 *							   BlockNumber newblkno)
 *		PredicateLockPageCombine(Relation relation, BlockNumber oldblkno,
 *								 BlockNumber newblkno)
 *		TransferPredicateLocksToHeapRelation(Relation relation)
 *		ReleasePredicateLocks(bool isCommit, bool isReadOnlySafe)
 *
 * conflict detection (may also trigger rollback)
 *		CheckForSerializableConflictOut(Relation relation, TransactionId xid,
 *										Snapshot snapshot)
 *		CheckForSerializableConflictIn(Relation relation, const ItemPointerData *tid,
 *									   BlockNumber blkno)
 *		CheckTableForSerializableConflictIn(Relation relation)
 *
 * final rollback checking
 *		PreCommit_CheckForSerializationFailure(void)
 *
 * two-phase commit support
 *		AtPrepare_PredicateLocks(void);
 *		PostPrepare_PredicateLocks(TransactionId xid);
 *		PredicateLockTwoPhaseFinish(FullTransactionId fxid, bool isCommit);
 *		predicatelock_twophase_recover(FullTransactionId fxid, uint16 info,
 *									   void *recdata, uint32 len);
 */

#include "postgres.h"

/* …（同文件无关代码省略）… */
 */

/*
 * AtPrepare_Locks
 *		Do the preparatory work for a PREPARE: make 2PC state file
 *		records for all predicate locks currently held.
 */
/* …（同文件无关代码省略）… */
}

/*
 * PostPrepare_Locks
 *		Clean up after successful PREPARE. Unlike the non-predicate
 *		lock manager, we do not need to transfer locks to a dummy
 *		PGPROC because our SERIALIZABLEXACT will stay around
