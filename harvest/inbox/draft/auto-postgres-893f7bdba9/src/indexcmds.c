// AUTO-DRAFT from postgres/postgres PR #d94964f14f39936d69c6b8d3fc5020f0fdae1617
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
	List	   *indexIds = NIL;
	List	   *newIndexIds = NIL;
	List	   *relationLocks = NIL;
	List	   *lockTags = NIL;
	ListCell   *lc,
			   *lc2;
/* …（同文件无关代码省略）… */
						indexIds = lappend(indexIds, idx);

						MemoryContextSwitchTo(oldcontext);
					}

					index_close(indexRelation, NoLock);
/* …（同文件无关代码省略）… */
							/* other fields set later */

							MemoryContextSwitchTo(oldcontext);
						}

						index_close(indexRelation, NoLock);
/* …（同文件无关代码省略）… */
		newIndexIds = lappend(newIndexIds, newidx);

		/*
		 * Save lockrelid to protect each relation from drop then close
		 * relations. The lockrelid on parent relation is not taken here to
		 * avoid multiple locks taken on the same relation, instead we rely on
		 * parentRelationIds built earlier.
		 */
		lockrelid = palloc_object(LockRelId);
		*lockrelid = indexRel->rd_lockInfo.lockRelId;
/* …（同文件无关代码省略）… */

		MemoryContextSwitchTo(oldcontext);

		index_close(indexRel, NoLock);
		index_close(newIndexRel, NoLock);

		/* Roll back any GUC changes executed by index functions */
		AtEOXact_GUC(false, save_nestlevel);
/* …（同文件无关代码省略）… */
		LockRelationIdForSession(lockrelid, ShareUpdateExclusiveLock);
	}

	PopActiveSnapshot();
	CommitTransactionCommand();
	StartTransactionCommand();
