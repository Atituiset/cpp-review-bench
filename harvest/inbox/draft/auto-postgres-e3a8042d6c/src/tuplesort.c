// AUTO-DRAFT from postgres/postgres PR #d9c6f22c3b2beaf34e47cf47fd21703886be8011
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define FREESTATE(state)	((state)->base.freestate ? (*(state)->base.freestate) (state) : (void) 0)
/* …（同文件无关代码省略）… */
#define SERIAL(state)		((state)->shared == NULL)
/* …（同文件无关代码省略）… */
	return state->boundUsed;
}

/*
 * tuplesort_set_allowbounded
 *		Set or unset TUPLESORT_ALLOWBOUNDED sort flag on an existing state.
 *
 * NOTE: tuplesort_reset() must be called after this function for the changes
 * to take effect.
 */
void
tuplesort_set_allowbounded(Tuplesortstate *state, bool allowbounded)
{
	if (allowbounded)
		state->base.sortopt |= TUPLESORT_ALLOWBOUNDED;
	else
		state->base.sortopt &= ~TUPLESORT_ALLOWBOUNDED;
}

/*
 * tuplesort_free
 *
/* …（同文件无关代码省略）… */
static void
tuplesort_free(Tuplesortstate *state)
{
	/* context swap probably not needed, but let's be safe */
	MemoryContext oldcontext = MemoryContextSwitchTo(state->base.sortcontext);
	int64		spaceUsed;

	if (state->tapeset)
		spaceUsed = LogicalTapeSetBlocks(state->tapeset);
	else
		spaceUsed = (state->allowedMem - state->availMem + 1023) / 1024;

	/*
	 * Delete temporary "tape" files, if any.
	 *
	 * We don't bother to destroy the individual tapes here. They will go away
	 * with the sortcontext.  (In TSS_FINALMERGE state, we have closed
	 * finished tapes already.)
	 */
	if (state->tapeset)
		LogicalTapeSetClose(state->tapeset);

	if (trace_sort)
	{
		if (state->tapeset)
			elog(LOG, "%s of worker %d ended, %" PRId64 " disk blocks used: %s",
				 SERIAL(state) ? "external sort" : "parallel external sort",
				 state->worker, spaceUsed, pg_rusage_show(&state->ru_start));
		else
			elog(LOG, "%s of worker %d ended, %" PRId64 " KB used: %s",
				 SERIAL(state) ? "internal sort" : "unperformed parallel sort",
				 state->worker, spaceUsed, pg_rusage_show(&state->ru_start));
	}

	TRACE_POSTGRESQL_SORT_DONE(state->tapeset != NULL, spaceUsed);

	FREESTATE(state);
	MemoryContextSwitchTo(oldcontext);

	/*
	 * Free the per-sort memory context, thereby releasing all working memory.
	 */
	MemoryContextReset(state->base.sortcontext);
}
/* …（同文件无关代码省略）… */
static void
tuplesort_updatemax(Tuplesortstate *state)
{
	int64		spaceUsed;
	bool		isSpaceDisk;

	/*
	 * Note: it might seem we should provide both memory and disk usage for a
	 * disk-based sort.  However, the current code doesn't track memory space
	 * accurately once we have begun to return tuples to the caller (since we
	 * don't account for pfree's the caller is expected to do), so we cannot
	 * rely on availMem in a disk sort.  This does not seem worth the overhead
	 * to fix.  Is it worth creating an API for the memory context code to
	 * tell us how much is actually used in sortcontext?
	 */
	if (state->tapeset)
	{
		isSpaceDisk = true;
		spaceUsed = LogicalTapeSetBlocks(state->tapeset) * BLCKSZ;
	}
	else
	{
		isSpaceDisk = false;
		spaceUsed = state->allowedMem - state->availMem;
	}

	/*
	 * Sort evicts data to the disk when it wasn't able to fit that data into
	 * main memory.  This is why we assume space used on the disk to be more
	 * important for tracking resource usage than space used in memory. Note
	 * that the amount of space occupied by some tupleset on the disk might be
	 * less than amount of space occupied by the same tupleset in memory due
	 * to more compact representation.
	 */
	if ((isSpaceDisk && !state->isMaxSpaceDisk) ||
		(isSpaceDisk == state->isMaxSpaceDisk && spaceUsed > state->maxSpace))
	{
		state->maxSpace = spaceUsed;
		state->isMaxSpaceDisk = isSpaceDisk;
		state->maxSpaceStatus = state->status;
	}
}
/* …（同文件无关代码省略）… */
 *	Reset the tuplesort.  Reset all the data in the tuplesort, but leave the
 *	meta-information in.  After tuplesort_reset, tuplesort is ready to start
 *	a new sort.  This allows avoiding recreation of tuple sort states (and
 *	save resources) when sorting multiple small batches.
 */
void
tuplesort_reset(Tuplesortstate *state)
{
	tuplesort_updatemax(state);
	tuplesort_free(state);

	/*
	 * After we've freed up per-batch memory, re-setup all of the state common
	 * to both the first batch and any subsequent batch.
