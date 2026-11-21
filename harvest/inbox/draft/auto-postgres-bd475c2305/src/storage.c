// AUTO-DRAFT from postgres/postgres PR #ecb260853526019a1340ba8a6b4c6de27a07d69b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
 */
void
RelationCopyStorage(SMgrRelation src, SMgrRelation dst,
					ForkNumber forkNum, char relpersistence)
{
	bool		use_wal;
	bool		copying_initfork;
/* …（同文件无关代码省略）… */
	for (blkno = 0; blkno < nblocks; blkno++)
	{
		BulkWriteBuffer buf;
		int			piv_flags;
		bool		checksum_failure;
		bool		verified;
/* …（同文件无关代码省略）… */
		}

		/*
		 * Queue the page for WAL-logging and writing out.  Unfortunately we
		 * don't know what kind of a page this is, so we have to log the full
		 * page including any unused space.
		 */
		smgr_bulk_write(bulkstate, blkno, buf, false);
	}
	smgr_bulk_finish(bulkstate);
}
