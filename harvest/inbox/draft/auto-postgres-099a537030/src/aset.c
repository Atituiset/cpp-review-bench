// AUTO-DRAFT from postgres/postgres PR #7b84cf8cfc7935d9ef275ea425c7bbd6e40b8c90
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdlib.h>
  // <<< BUG ANCHOR
#define ALLOC_MINBITS		3	/* smallest chunk size is 8 bytes */
#define ALLOCSET_NUM_FREELISTS	11
#define ALLOC_CHUNK_LIMIT	(1 << (ALLOCSET_NUM_FREELISTS-1+ALLOC_MINBITS))
/* …（同文件无关代码省略）… */
#define ALLOC_CHUNK_FRACTION	4
/* …（同文件无关代码省略）… */
#define ALLOC_BLOCKHDRSZ	MAXALIGN(sizeof(AllocBlockData))
#define ALLOC_CHUNKHDRSZ	sizeof(MemoryChunk)
/* …（同文件无关代码省略）… */
typedef struct AllocBlockData *AllocBlock;	/* forward reference */
/* …（同文件无关代码省略）… */
typedef struct AllocSetContext
{
	MemoryContextData header;	/* Standard memory-context fields */
	/* Info about storage allocated in this context: */
	AllocBlock	blocks;			/* head of list of blocks in this set */
	MemoryChunk *freelist[ALLOCSET_NUM_FREELISTS];	/* free chunk lists */
	/* Allocation parameters for this context: */
	uint32		initBlockSize;	/* initial block size */
	uint32		maxBlockSize;	/* maximum block size */
	uint32		nextBlockSize;	/* next block size to allocate */
	uint32		allocChunkLimit;	/* effective chunk size limit */
	/* freelist this context could be put in, or -1 if not a candidate: */
	int			freeListIndex;	/* index in context_freelists[], or -1 */
} AllocSetContext;
/* …（同文件无关代码省略）… */
typedef AllocSetContext *AllocSet;
/* …（同文件无关代码省略）… */
typedef struct AllocBlockData
{
	AllocSet	aset;			/* aset that owns this block */
	AllocBlock	prev;			/* prev block in aset's blocks list, if any */
	AllocBlock	next;			/* next block in aset's blocks list, if any */
	char	   *freeptr;		/* start of free space in this block */
	char	   *endptr;			/* end of space in this block */
}			AllocBlockData;
/* …（同文件无关代码省略）… */
	return idx;
}


/*
 * Public routines
/* …（同文件无关代码省略）… */
			freelist->first_free = (AllocSet) set->header.nextchild;
			freelist->num_free--;

			/* Update its maxBlockSize; everything else should be OK */
			set->maxBlockSize = maxBlockSize;

			/* Reinitialize its header, installing correct name and parent */
			MemoryContextCreate((MemoryContext) set,
/* …（同文件无关代码省略）… */
	set->nextBlockSize = (uint32) initBlockSize;
	set->freeListIndex = freeListIndex;

	/*
	 * Compute the allocation chunk size limit for this context.  It can't be
	 * more than ALLOC_CHUNK_LIMIT because of the fixed number of freelists.
	 * If maxBlockSize is small then requests exceeding the maxBlockSize, or
	 * even a significant fraction of it, should be treated as large chunks
	 * too.  For the typical case of maxBlockSize a power of 2, the chunk size
	 * limit will be at most 1/8th maxBlockSize, so that given a stream of
	 * requests that are all the maximum chunk size we will waste at most
	 * 1/8th of the allocated space.
	 *
	 * Determine the maximum size that a chunk can be before we allocate an
	 * entire AllocBlock dedicated for that chunk.  We set the absolute limit
	 * of that size as ALLOC_CHUNK_LIMIT but we reduce it further so that we
	 * can fit about ALLOC_CHUNK_FRACTION chunks this size on a maximally
	 * sized block.  (We opt to keep allocChunkLimit a power-of-2 value
	 * primarily for legacy reasons rather than calculating it so that exactly
	 * ALLOC_CHUNK_FRACTION chunks fit on a maximally sized block.)
	 */
	set->allocChunkLimit = ALLOC_CHUNK_LIMIT;
	while ((Size) (set->allocChunkLimit + ALLOC_CHUNKHDRSZ) >
		   (Size) ((maxBlockSize - ALLOC_BLOCKHDRSZ) / ALLOC_CHUNK_FRACTION))
		set->allocChunkLimit >>= 1;

	/* Finally, do the type-independent part of context creation */
	MemoryContextCreate((MemoryContext) set,
