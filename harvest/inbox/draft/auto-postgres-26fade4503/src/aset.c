// AUTO-DRAFT from postgres/postgres PR #57926d757ab43852021d917b6221687a341c8385
#define ALLOC_CHUNKHDRSZ	sizeof(MemoryChunk)
/* …（同文件无关代码省略）… */
			 * Check for overwrite of padding space in an allocated chunk.
			 */
			if (dsize != InvalidAllocSize && dsize < chsize &&
				!sentinel_ok(chunk, ALLOC_CHUNKHDRSZ + dsize))  // <<< BUG ANCHOR
				elog(WARNING, "problem in alloc set %s: detected write past chunk end in block %p, chunk %p",
					 name, block, chunk);
