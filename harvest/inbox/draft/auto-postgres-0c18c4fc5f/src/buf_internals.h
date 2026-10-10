// AUTO-DRAFT from postgres/postgres PR #15a947f30726cadff695b7601329d90b6703e268
typedef struct buftag
{
	Oid			spcOid;			/* tablespace oid */
	Oid			dbOid;			/* database oid */
	RelFileNumber relNumber;	/* relation file number */
	ForkNumber	forkNum;		/* fork number */
	BlockNumber blockNum;		/* blknum relative to begin of reln */
} BufferTag;
/* …（同文件无关代码省略）… */
typedef struct BufferDesc
{
	/*
	 * ID of page contained in buffer. The buffer header spinlock needs to be
	 * held to modify this field.
	 */
	BufferTag	tag;
  // <<< BUG ANCHOR
	/*
	 * Buffer's index number (from 0). The field never changes after
	 * initialization, so does not need locking.
	 */
	int			buf_id;

	/*
	 * State of the buffer, containing flags, refcount and usagecount. See
	 * BUF_* and BM_* defines at the top of this file.
	 */
	pg_atomic_uint64 state;

	/*
	 * Backend of pin-count waiter. The buffer header spinlock needs to be
	 * held to modify this field.
	 */
	int			wait_backend_pgprocno;

	PgAioWaitRef io_wref;		/* set iff AIO is in progress */

	/*
	 * List of PGPROCs waiting for the buffer content lock. Protected by the
	 * buffer header spinlock.
	 */
	proclist_head lock_waiters;
} BufferDesc;
/* …（同文件无关代码省略）… */
#define BUFFERDESC_PAD_TO_SIZE	(SIZEOF_VOID_P == 8 ? 64 : 1)
/* …（同文件无关代码省略）… */
typedef union BufferDescPadded
{
	BufferDesc	bufferdesc;
	char		pad[BUFFERDESC_PAD_TO_SIZE];
} BufferDescPadded;
/* …（同文件无关代码省略）… */
typedef struct PendingWriteback
{
	/* could store different types of pending flushes here */
	BufferTag	tag;
} PendingWriteback;
/* …（同文件无关代码省略）… */
typedef struct WritebackContext
{
	/* pointer to the max number of writeback requests to coalesce */
	int		   *max_pending;

	/* current number of pending writeback requests */
	int			nr_pending;

	/* pending requests */
	PendingWriteback pending_writebacks[WRITEBACK_MAX_PENDING_FLUSHES];
} WritebackContext;
/* …（同文件无关代码省略）… */
/* in buf_init.c */
extern PGDLLIMPORT BufferDescPadded *BufferDescriptors;
extern PGDLLIMPORT ConditionVariableMinimallyPadded *BufferIOCVArray;
extern PGDLLIMPORT WritebackContext BackendWritebackContext;

/* in localbuf.c */
extern PGDLLIMPORT BufferDesc *LocalBufferDescriptors;
