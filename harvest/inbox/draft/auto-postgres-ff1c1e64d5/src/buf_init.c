// AUTO-DRAFT from postgres/postgres PR #15a947f30726cadff695b7601329d90b6703e268
BufferDescPadded *BufferDescriptors;
char	   *BufferBlocks;
ConditionVariableMinimallyPadded *BufferIOCVArray;
WritebackContext BackendWritebackContext;  // <<< BUG ANCHOR
CkptSortItem *CkptBufferIds;

static void BufferManagerShmemRequest(void *arg);
static void BufferManagerShmemInit(void *arg);
static void BufferManagerShmemAttach(void *arg);

const ShmemCallbacks BufferManagerShmemCallbacks = {
	.request_fn = BufferManagerShmemRequest,
	.init_fn = BufferManagerShmemInit,
	.attach_fn = BufferManagerShmemAttach,
};

/*
/* …（同文件无关代码省略）… */
static void
BufferManagerShmemRequest(void *arg)
{
	ShmemRequestStruct(.name = "Buffer Descriptors",
					   .size = NBuffers * sizeof(BufferDescPadded),
	/* Align descriptors to a cacheline boundary. */
					   .alignment = PG_CACHE_LINE_SIZE,
					   .ptr = (void **) &BufferDescriptors,
		);

	ShmemRequestStruct(.name = "Buffer Blocks",
					   .size = NBuffers * (Size) BLCKSZ,
	/* Align buffer pool on IO page size boundary. */
					   .alignment = PG_IO_ALIGN_SIZE,
					   .ptr = (void **) &BufferBlocks,
		);

	ShmemRequestStruct(.name = "Buffer IO Condition Variables",
					   .size = NBuffers * sizeof(ConditionVariableMinimallyPadded),
	/* Align descriptors to a cacheline boundary. */
					   .alignment = PG_CACHE_LINE_SIZE,
					   .ptr = (void **) &BufferIOCVArray,
		);

	/*
	 * The array used to sort to-be-checkpointed buffer ids is located in
	 * shared memory, to avoid having to allocate significant amounts of
	 * memory at runtime. As that'd be in the middle of a checkpoint, or when
	 * the checkpointer is restarted, memory allocation failures would be
	 * painful.
	 */
	ShmemRequestStruct(.name = "Checkpoint BufferIds",
					   .size = NBuffers * sizeof(CkptSortItem),
					   .ptr = (void **) &CkptBufferIds,
		);
}
/* …（同文件无关代码省略）… */
static void
BufferManagerShmemInit(void *arg)
{
	/*
	 * Initialize all the buffer headers.
	 */
	for (int i = 0; i < NBuffers; i++)
	{
		BufferDesc *buf = GetBufferDescriptor(i);

		ClearBufferTag(&buf->tag);

		pg_atomic_init_u64(&buf->state, 0);
		buf->wait_backend_pgprocno = INVALID_PROC_NUMBER;

		buf->buf_id = i;

		pgaio_wref_clear(&buf->io_wref);

		proclist_init(&buf->lock_waiters);
		ConditionVariableInit(BufferDescriptorGetIOCV(buf));
	}

	/* Initialize per-backend file flush context */
	WritebackContextInit(&BackendWritebackContext,
						 &backend_flush_after);
}

static void
BufferManagerShmemAttach(void *arg)
{
	/* Initialize per-backend file flush context */
	WritebackContextInit(&BackendWritebackContext,
						 &backend_flush_after);
}
