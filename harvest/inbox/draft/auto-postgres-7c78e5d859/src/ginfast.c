// AUTO-DRAFT from postgres/postgres PR #96c99955b62994b2cb47380d84f463456cf16ee5
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <time.h>
  // <<< BUG ANCHOR
typedef struct KeyArray
{
	Datum	   *keys;			/* expansible array */
	GinNullCategory *categories;	/* another expansible array */
	int32		nvalues;		/* current number of valid entries */
	int32		maxvalues;		/* allocated size of arrays */
} KeyArray;
/* …（同文件无关代码省略）… */
static void
addDatum(KeyArray *keys, Datum datum, GinNullCategory category)
{
	if (keys->nvalues >= keys->maxvalues)
	{
		keys->maxvalues *= 2;
		keys->keys = repalloc_array(keys->keys, Datum, keys->maxvalues);
		keys->categories = repalloc_array(keys->categories, GinNullCategory, keys->maxvalues);
	}

	keys->keys[keys->nvalues] = datum;
	keys->categories[keys->nvalues] = category;
	keys->nvalues++;
}
/* …（同文件无关代码省略）… */
static void
processPendingPage(BuildAccumulator *accum, KeyArray *ka,
				   Page page, OffsetNumber startoff)
{
	ItemPointerData heapptr;
	OffsetNumber i,
				maxoff;
	OffsetNumber attrnum;

	/* reset *ka to empty */
	ka->nvalues = 0;

	maxoff = PageGetMaxOffsetNumber(page);
	Assert(maxoff >= FirstOffsetNumber);
	ItemPointerSetInvalid(&heapptr);
	attrnum = 0;

	for (i = startoff; i <= maxoff; i = OffsetNumberNext(i))
	{
		IndexTuple	itup = (IndexTuple) PageGetItem(page, PageGetItemId(page, i));
		OffsetNumber curattnum;
		Datum		curkey;
		GinNullCategory curcategory;

		/* Check for change of heap TID or attnum */
		curattnum = gintuple_get_attrnum(accum->ginstate, itup);

		if (!ItemPointerIsValid(&heapptr))
		{
			heapptr = itup->t_tid;
			attrnum = curattnum;
		}
		else if (!(ItemPointerEquals(&heapptr, &itup->t_tid) &&
				   curattnum == attrnum))
		{
			/*
			 * ginInsertBAEntries can insert several datums per call, but only
			 * for one heap tuple and one column.  So call it at a boundary,
			 * and reset ka.
			 */
			ginInsertBAEntries(accum, &heapptr, attrnum,
							   ka->keys, ka->categories, ka->nvalues);
			ka->nvalues = 0;
			heapptr = itup->t_tid;
			attrnum = curattnum;
		}

		/* Add key to KeyArray */
		curkey = gintuple_get_key(accum->ginstate, itup, &curcategory);
		addDatum(ka, curkey, curcategory);
	}

	/* Dump out all remaining keys */
	ginInsertBAEntries(accum, &heapptr, attrnum,
					   ka->keys, ka->categories, ka->nvalues);
}
/* …（同文件无关代码省略）… */
		 */
		processPendingPage(&accum, &datums, page, FirstOffsetNumber);

		vacuum_delay_point(false);

		/*
		 * Is it time to flush memory to disk?	Flush if we are at the end of
		 * the pending list, or if we have a full row and memory is getting
/* …（同文件无关代码省略）… */
			UnlockReleaseBuffer(buffer);
		}

		/*
		 * Read next page in pending list
		 */
		vacuum_delay_point(false);
		buffer = ReadBuffer(index, blkno);
		LockBuffer(buffer, GIN_SHARE);
		page = BufferGetPage(buffer);
