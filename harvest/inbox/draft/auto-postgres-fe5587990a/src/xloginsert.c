// AUTO-DRAFT from postgres/postgres PR #6873a138f782cf816e6019bbe922857050df96c9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

#define SizeOfXlogOrigin	(sizeof(ReplOriginId) + sizeof(char))
#define SizeOfXLogTransactionId	(sizeof(TransactionId) + sizeof(char))
/* …（同文件无关代码省略）… */
#define HEADER_SCRATCH_SIZE \
	(SizeOfXLogRecord + \
	 MaxSizeOfXLogRecordBlockHeader * (XLR_MAX_BLOCK_ID + 1) + \
	 SizeOfXLogRecordDataHeaderLong + SizeOfXlogOrigin + \
	 SizeOfXLogTransactionId)
/* …（同文件无关代码省略）… */
	if (hdr_scratch == NULL)
		hdr_scratch = MemoryContextAllocZero(xloginsert_cxt,
											 HEADER_SCRATCH_SIZE);
}
