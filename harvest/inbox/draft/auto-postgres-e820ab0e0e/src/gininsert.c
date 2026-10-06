// AUTO-DRAFT from postgres/postgres PR #3be29d9346db1d3225773e2a8e5a86b875696617
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
typedef struct GinBuffer
{
	OffsetNumber attnum;
	GinNullCategory category;
	Datum		key;			/* 0 if no key (and keylen == 0) */
	Size		keylen;			/* number of bytes (not typlen) */

	/* type info */
	int16		typlen;
	bool		typbyval;

	/* Number of TIDs to collect before attempt to write some out. */
	int			maxitems;

	/* array of TID values */
	int			nitems;
	int			nfrozen;
	SortSupport ssup;			/* for sorting/comparing keys */
	ItemPointerData *items;
} GinBuffer;
/* …（同文件无关代码省略）… */
static bool
GinBufferIsEmpty(GinBuffer *buffer)
{
	return (buffer->nitems == 0);
}
/* …（同文件无关代码省略）… */
static void
GinBufferReset(GinBuffer *buffer)
{
	Assert(!GinBufferIsEmpty(buffer));

	/* release byref values, do nothing for by-val ones */
	if ((buffer->category == GIN_CAT_NORM_KEY) && !buffer->typbyval)
		pfree(DatumGetPointer(buffer->key));

	/*
	 * Not required, but makes it more likely to trigger NULL dereference if
	 * using the value incorrectly, etc.
	 */
	buffer->key = (Datum) 0;

	buffer->attnum = 0;
	buffer->category = 0;
	buffer->keylen = 0;
	buffer->nitems = 0;
	buffer->nfrozen = 0;

	buffer->typlen = 0;
	buffer->typbyval = 0;
}
/* …（同文件无关代码省略）… */

		/* discard the existing data */
		GinBufferReset(buffer);

		/* Report progress */
		pgstat_progress_update_param(PROGRESS_CREATEIDX_TUPLES_DONE,
									 ++numtuples);
	}

	/* release all the memory */
/* …（同文件无关代码省略）… */
									buffer->items, buffer->nfrozen, &ntuplen);

			tuplesort_putgintuple(state->bs_sortstate, ntup, ntuplen);

			pfree(ntup);
