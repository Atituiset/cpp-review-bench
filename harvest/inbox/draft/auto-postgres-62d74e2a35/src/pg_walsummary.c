// AUTO-DRAFT from postgres/postgres PR #2a8ce0f684a4aa3da8f8c6f2d255048d545e96ff
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	BlockNumber startblock = InvalidBlockNumber;
	BlockNumber endblock = InvalidBlockNumber;

	/* Dump limit block, if any. */
	if (limit_block != InvalidBlockNumber)
		printf("TS %u, DB %u, REL %u, FORK %s: limit %u\n",
			   rlocator->spcOid, rlocator->dbOid, rlocator->relNumber,
			   forkNames[forknum], limit_block);

	/* If we haven't allocated a block buffer yet, do that now. */
	if (block_buffer == NULL)
		block_buffer = palloc_array(BlockNumber, block_buffer_size);
/* …（同文件无关代码省略）… */
	if (opt->quiet)
		return;

	/*
	 * Sort the returned block numbers. If the block reference table was using
	 * the bitmap representation for a given chunk, the block numbers in that
