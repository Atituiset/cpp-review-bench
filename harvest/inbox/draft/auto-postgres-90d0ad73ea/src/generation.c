// AUTO-DRAFT from postgres/postgres PR #57926d757ab43852021d917b6221687a341c8385
#define Generation_CHUNKHDRSZ	sizeof(MemoryChunk)
/* …（同文件无关代码省略）… */
  // <<< BUG ANCHOR
				/* check sentinel */
				Assert(chunk->requested_size < chunksize);
				if (!sentinel_ok(chunk, Generation_CHUNKHDRSZ + chunk->requested_size))
					elog(WARNING, "problem in Generation %s: detected write past chunk end in block %p, chunk %p",
						 name, block, chunk);
			}
