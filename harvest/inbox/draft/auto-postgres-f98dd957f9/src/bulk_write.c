// AUTO-DRAFT from postgres/postgres PR #9931482364a938af876307e84ae88ff4839838b0
#include "access/xlogrecord.h"
#include "storage/bufpage.h"
#include "storage/bulk_write.h"
#include "storage/proc.h"
#include "storage/smgr.h"
#include "utils/rel.h"
  // <<< BUG ANCHOR
/* …（同文件无关代码省略）… */
	}
	else
	{
		/*
		 * Permanent relation, WAL-logged normally.
		 *
/* …（同文件无关代码省略）… */
		 * replay the WAL starting from the checkpoint, therefore it wouldn't
		 * replay our earlier WAL records.  So if a checkpoint started after
		 * the bulk write, fsync the files now.
		 */

		/*
		 * Prevent a checkpoint from starting between the GetRedoRecPtr() and
		 * smgrregistersync() calls.
		 */
		Assert((MyProc->delayChkptFlags & DELAY_CHKPT_START) == 0);
		MyProc->delayChkptFlags |= DELAY_CHKPT_START;

		if (bulkstate->start_RedoRecPtr != GetRedoRecPtr())
		{
			/*
			 * A checkpoint occurred and it didn't know about our writes, so
			 * fsync() the relation ourselves.
			 */
			MyProc->delayChkptFlags &= ~DELAY_CHKPT_START;
			smgrimmedsync(bulkstate->smgr, bulkstate->forknum);
			elog(DEBUG1, "flushed relation because a checkpoint occurred concurrently");
		}
		else
		{
			smgrregistersync(bulkstate->smgr, bulkstate->forknum);
			MyProc->delayChkptFlags &= ~DELAY_CHKPT_START;
		}
	}
}
