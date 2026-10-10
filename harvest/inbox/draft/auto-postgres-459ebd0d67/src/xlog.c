// AUTO-DRAFT from postgres/postgres PR #6873a138f782cf816e6019bbe922857050df96c9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct XLogwrtRqst
{
	XLogRecPtr	Write;			/* last byte + 1 to write out */
	XLogRecPtr	Flush;			/* last byte + 1 to flush */
} XLogwrtRqst;
/* …（同文件无关代码省略）… */
typedef struct
{
	LWLock		lock;
	pg_atomic_uint64 insertingAt;
	XLogRecPtr	lastImportantAt;
} WALInsertLock;
/* …（同文件无关代码省略）… */
typedef union WALInsertLockPadded
{
	WALInsertLock l;
	char		pad[PG_CACHE_LINE_SIZE];
} WALInsertLockPadded;
/* …（同文件无关代码省略）… */
typedef struct XLogCtlInsert
{
	slock_t		insertpos_lck;	/* protects CurrBytePos and PrevBytePos */

	/*
	 * CurrBytePos is the end of reserved WAL. The next record will be
	 * inserted at that position. PrevBytePos is the start position of the
	 * previously inserted (or rather, reserved) record - it is copied to the
	 * prev-link of the next record. These are stored as "usable byte
	 * positions" rather than XLogRecPtrs (see XLogBytePosToRecPtr()).
	 */
	uint64		CurrBytePos;
	uint64		PrevBytePos;

	/*
	 * Make sure the above heavily-contended spinlock and byte positions are
	 * on their own cache line. In particular, the RedoRecPtr and full page
	 * write variables below should be on a different cache line. They are
	 * read on every WAL insertion, but updated rarely, and we don't want
	 * those reads to steal the cache line containing Curr/PrevBytePos.
	 */
	char		pad[PG_CACHE_LINE_SIZE];

	/*
	 * fullPageWrites is the authoritative value used by all backends to
	 * determine whether to write full-page image to WAL. This shared value,
	 * instead of the process-local fullPageWrites, is required because, when
	 * full_page_writes is changed by SIGHUP, we must WAL-log it before it
	 * actually affects WAL-logging by backends.  Checkpointer sets at startup
	 * or after SIGHUP.
	 *
	 * To read these fields, you must hold an insertion lock. To modify them,
	 * you must hold ALL the locks.
	 */
	XLogRecPtr	RedoRecPtr;		/* current redo point for insertions */
	bool		fullPageWrites;

	/*
	 * runningBackups is a counter indicating the number of backups currently
	 * in progress. lastBackupStart is the latest checkpoint redo location
	 * used as a starting point for an online backup.
	 */
	int			runningBackups;
	XLogRecPtr	lastBackupStart;

	/*
	 * WAL insertion locks.
	 */
	WALInsertLockPadded *WALInsertLocks;
} XLogCtlInsert;
/* …（同文件无关代码省略）… */
typedef struct XLogCtlData
{
	XLogCtlInsert Insert;

	/* Protected by info_lck: */
	XLogwrtRqst LogwrtRqst;
	XLogRecPtr	RedoRecPtr;		/* a recent copy of Insert->RedoRecPtr */
	XLogRecPtr	asyncXactLSN;	/* LSN of newest async commit/abort */
	XLogRecPtr	replicationSlotMinLSN;	/* oldest LSN needed by any slot */

	XLogSegNo	lastRemovedSegNo;	/* latest removed/recycled XLOG segment */

	/* Fake LSN counter, for unlogged relations. */
	pg_atomic_uint64 unloggedLSN;

	/* Time and LSN of last xlog segment switch. Protected by WALWriteLock. */
	pg_time_t	lastSegSwitchTime;
	XLogRecPtr	lastSegSwitchLSN;

	/* These are accessed using atomics -- info_lck not needed */
	pg_atomic_uint64 logInsertResult;	/* last byte + 1 inserted to buffers */
	pg_atomic_uint64 logWriteResult;	/* last byte + 1 written out */
	pg_atomic_uint64 logFlushResult;	/* last byte + 1 flushed */

	/*
	 * Latest initialized page in the cache (last byte position + 1).
	 *
	 * To change the identity of a buffer (and InitializedUpTo), you need to
	 * hold WALBufMappingLock.  To change the identity of a buffer that's
	 * still dirty, the old page needs to be written out first, and for that
	 * you need WALWriteLock, and you need to ensure that there are no
	 * in-progress insertions to the page by calling
	 * WaitXLogInsertionsToFinish().
	 */
	XLogRecPtr	InitializedUpTo;

	/*
	 * These values do not change after startup, although the pointed-to pages
	 * and xlblocks values certainly do.  xlblocks values are protected by
	 * WALBufMappingLock.
	 */
	char	   *pages;			/* buffers for unwritten XLOG pages */
	pg_atomic_uint64 *xlblocks; /* 1st byte ptr-s + XLOG_BLCKSZ */
	int			XLogCacheBlck;	/* highest allocated xlog buffer index */

	/*
	 * InsertTimeLineID is the timeline into which new WAL is being inserted
	 * and flushed. It is zero during recovery, and does not change once set.
	 *
	 * If we create a new timeline when the system was started up,
	 * PrevTimeLineID is the old timeline's ID that we forked off from.
	 * Otherwise it's equal to InsertTimeLineID.
	 *
	 * We set these fields while holding info_lck. Most that reads these
	 * values knows that recovery is no longer in progress and so can safely
	 * read the value without a lock, but code that could be run either during
	 * or after recovery can take info_lck while reading these values.
	 */
	TimeLineID	InsertTimeLineID;
	TimeLineID	PrevTimeLineID;

	/*
	 * SharedRecoveryState indicates if we're still in crash or archive
	 * recovery.  Protected by info_lck.
	 */
	RecoveryState SharedRecoveryState;

	/*
	 * InstallXLogFileSegmentActive indicates whether the checkpointer should
	 * arrange for future segments by recycling and/or PreallocXlogFiles().
	 * Protected by ControlFileLock.  Only the startup process changes it.  If
	 * true, anyone can use InstallXLogFileSegment().  If false, the startup
	 * process owns the exclusive right to install segments, by reading from
	 * the archive and possibly replacing existing files.
	 */
	bool		InstallXLogFileSegmentActive;

	/*
	 * WalWriterSleeping indicates whether the WAL writer is currently in
	 * low-power mode (and hence should be nudged if an async commit occurs).
	 * Protected by info_lck.
	 */
	bool		WalWriterSleeping;

	/*
	 * During recovery, we keep a copy of the latest checkpoint record here.
	 * lastCheckPointRecPtr points to start of checkpoint record and
	 * lastCheckPointEndPtr points to end+1 of checkpoint record.  Used by the
	 * checkpointer when it wants to create a restartpoint.
	 *
	 * Protected by info_lck.
	 */
	XLogRecPtr	lastCheckPointRecPtr;
	XLogRecPtr	lastCheckPointEndPtr;
	CheckPoint	lastCheckPoint;

	/*
	 * lastFpwDisableRecPtr points to the start of the last replayed
	 * XLOG_FPW_CHANGE record that instructs full_page_writes is disabled.
	 */
	XLogRecPtr	lastFpwDisableRecPtr;

	/* current data checksum state of this node */
	uint32		data_checksum_version;

	/*
	 * Copies of control file fields with the same names, see pg_control.h for
	 * an in-depth description of these fields.  Must be updated together with
	 * data_checksum_version under info_lck.
	 */
	XLogRecPtr	data_checksum_lsn;
	bool		data_checksum_is_local;

	slock_t		info_lck;		/* locks shared variables shown above */

	/*
	 * lastChecksumChangeRecPtr points to the end of the last XLOG2_CHECKSUMS
	 * record inserted or replayed which corresponds to the last change of
	 * data_checksum_version.  InvalidXLogRecPtr if the state hasn't changed
	 * since the server started.
	 */
	pg_atomic_uint64 lastChecksumChangeRecPtr;
} XLogCtlData;
/* …（同文件无关代码省略）… */
	char	   *allocptr;
	int			i;

#ifdef WAL_DEBUG

	/*
	 * Create a memory context for WAL debugging that's exempt from the normal
	 * "no pallocs in critical section" rule. Yes, that can lead to a PANIC if
	 * an allocation fails, but wal_debug is not for production use anyway.
	 */
	if (walDebugCxt == NULL)
	{
		walDebugCxt = AllocSetContextCreate(TopMemoryContext,
											"WAL Debug",
											ALLOCSET_DEFAULT_SIZES);
		MemoryContextAllowInCriticalSection(walDebugCxt, true);
	}
#endif

	memset(XLogCtl, 0, sizeof(XLogCtlData));

	/*
/* …（同文件无关代码省略）… */
	WALInsertLocks = XLogCtl->Insert.WALInsertLocks;
}

/*
 * This func must be called ONCE on system install.  It creates pg_control
 * and the initial XLOG segment.
