// AUTO-DRAFT from postgres/postgres PR #57926d757ab43852021d917b6221687a341c8385
#define Slab_CHUNKHDRSZ sizeof(MemoryChunk)
/* …（同文件无关代码省略）… */
  // <<< BUG ANCHOR
					/* check the sentinel byte is intact */
					Assert(slab->chunkSize < (slab->fullChunkSize - Slab_CHUNKHDRSZ));
					if (!sentinel_ok(chunk, Slab_CHUNKHDRSZ + slab->chunkSize))
						elog(WARNING, "problem in slab %s: detected write past chunk end in block %p, chunk %p",
							 name, block, chunk);
				}
