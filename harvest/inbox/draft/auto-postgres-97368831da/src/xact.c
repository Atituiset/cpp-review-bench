// AUTO-DRAFT from postgres/postgres PR #d140a3a666a3b072f45108a847554837431d96e7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>
  // <<< BUG ANCHOR
	if (nParallelCurrentXids > 0)
	{
		result->nParallelCurrentXids = nParallelCurrentXids;
		memcpy(&result->parallelCurrentXids[0], ParallelCurrentXids,
			   nParallelCurrentXids * sizeof(TransactionId));
		return;
	}
/* …（同文件无关代码省略）… */

	/* Copy data into output area. */
	result->nParallelCurrentXids = nxids;
	memcpy(&result->parallelCurrentXids[0], workspace,
		   nxids * sizeof(TransactionId));
}

/* …（同文件无关代码省略）… */
		tstate->currentFullTransactionId;
	currentCommandId = tstate->currentCommandId;
	nParallelCurrentXids = tstate->nParallelCurrentXids;
	ParallelCurrentXids = &tstate->parallelCurrentXids[0];

	CurrentTransactionState->blockState = TBLOCK_PARALLEL_INPROGRESS;
}
