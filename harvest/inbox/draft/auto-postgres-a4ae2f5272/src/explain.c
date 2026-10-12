// AUTO-DRAFT from postgres/postgres PR #a241bc4fedefae3c0b0952c0a9d3a28d7747bfe3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define BYTES_TO_KILOBYTES(b) (((b) + 1023) / 1024)
/* …（同文件无关代码省略）… */
static void
show_storage_info(bool diskUsed, int64 maxSpaceUsed, ExplainState *es)
{
	int64		maxSpaceUsedKB = BYTES_TO_KILOBYTES(maxSpaceUsed);
	const char *storageType = diskUsed ? "Disk" : "Memory";

	if (es->format != EXPLAIN_FORMAT_TEXT)
	{
		ExplainPropertyText("Storage", storageType, es);
		ExplainPropertyInteger("Maximum Storage", "kB", maxSpaceUsedKB, es);
	}
	else
	{
		ExplainIndentText(es);
		appendStringInfo(es->str,
						 "Storage: %s  Maximum Storage: " INT64_FORMAT "kB\n",
						 storageType,
						 maxSpaceUsedKB);
	}
}
/* …（同文件无关代码省略）… */

	Tuplestorestate *tupstore = mstate->tuplestorestate;

	/*
	 * Nothing to show if ANALYZE option wasn't used or if execution didn't
	 * get as far as creating the tuplestore.
	 */
	if (!es->analyze || tupstore == NULL)
		return;

	tuplestore_get_stats(tupstore, &usedDisk, &maxSpaceUsed);
	show_storage_info(usedDisk, maxSpaceUsed, es);
}

/*
