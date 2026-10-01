// AUTO-DRAFT from postgres/postgres PR #b69356cd789fe963447177a0077c12c6c322c203
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <time.h>

#define MultiXactOffsetCtl	(&MultiXactOffsetSlruDesc)
#define MultiXactMemberCtl	(&MultiXactMemberSlruDesc)
/* …（同文件无关代码省略）… */
	 * against catastrophic data loss due to multixact wraparound.  The basic
	 * rules are:
	 *
	 * If we're past multiVacLimit or the safe threshold for member storage
	 * space, or we don't know what the safe threshold for member storage is,
	 * start trying to force autovacuum cycles.
	 * If we're past multiWarnLimit, start issuing warnings.
	 * If we're past multiStopLimit, refuse to create new MultiXactIds.
	 *
/* …（同文件无关代码省略）… */
	 * Offsets are 64-bits wide and never wrap around, so we don't need to
	 * consider them for emergency autovacuum purposes.  But now that we're in
	 * a consistent state, determine MultiXactState->oldestOffset.  It will be
	 * used to adjust the freezing cutoff, to keep the offsets disk usage in
	 * check.
	 */
	SetOldestOffset();
/* …（同文件无关代码省略）… */
static void
SetOldestOffset(void)
{
	MultiXactId oldestMultiXactId;
	MultiXactId nextMXact;
	MultiXactOffset oldestOffset = 0;	/* placate compiler */
	MultiXactOffset nextOffset;
	bool		oldestOffsetKnown = false;

	/*
	 * NB: Have to prevent concurrent truncation, we might otherwise try to
	 * lookup an oldestMulti that's concurrently getting truncated away.
	 */
	LWLockAcquire(MultiXactTruncationLock, LW_SHARED);

	/* Read relevant fields from shared memory. */
	LWLockAcquire(MultiXactGenLock, LW_SHARED);
	oldestMultiXactId = MultiXactState->oldestMultiXactId;
	nextMXact = MultiXactState->nextMXact;
	nextOffset = MultiXactState->nextOffset;
	Assert(MultiXactState->finishedStartup);
	LWLockRelease(MultiXactGenLock);

	/*
	 * Determine the offset of the oldest multixact.  Normally, we can read
	 * the offset from the multixact itself, but there's an important special
	 * case: if there are no multixacts in existence at all, oldestMXact
	 * obviously can't point to one.  It will instead point to the multixact
	 * ID that will be assigned the next time one is needed.
	 */
	if (oldestMultiXactId == nextMXact)
	{
		/*
		 * When the next multixact gets created, it will be stored at the next
		 * offset.
		 */
		oldestOffset = nextOffset;
		oldestOffsetKnown = true;
	}
	else
	{
		/*
		 * Look up the offset at which the oldest existing multixact's members
		 * are stored.  If we cannot find it, be careful not to fail, and
		 * leave oldestOffset unchanged.  oldestOffset is initialized to zero
		 * at system startup, which prevents truncating members until a proper
		 * value is calculated.
		 *
		 * (We had bugs in early releases of PostgreSQL 9.3.X and 9.4.X where
		 * the supposedly-earliest multixact might not really exist.  Those
		 * should be long gone by now, so this should not fail, but let's
		 * still be defensive.)
		 */
		oldestOffsetKnown =
			find_multixact_start(oldestMultiXactId, &oldestOffset);

		if (oldestOffsetKnown)
			ereport(DEBUG1,
					(errmsg_internal("oldest MultiXactId member is at offset %" PRIu64,
									 oldestOffset)));
		else
			ereport(LOG,
					(errmsg("MultiXact member truncation is disabled because oldest checkpointed MultiXact %u does not exist on disk",
							oldestMultiXactId)));
	}

	LWLockRelease(MultiXactTruncationLock);

	/* Install the computed value */
	if (oldestOffsetKnown)
	{
		LWLockAcquire(MultiXactGenLock, LW_EXCLUSIVE);
		MultiXactState->oldestOffset = oldestOffset;
		LWLockRelease(MultiXactGenLock);
	}
}
/* …（同文件无关代码省略）… */
static bool
find_multixact_start(MultiXactId multi, MultiXactOffset *result)
{
	MultiXactOffset offset;
	int64		pageno;
	int			entryno;
	int			slotno;
	MultiXactOffset *offptr;

	Assert(MultiXactState->finishedStartup);

	pageno = MultiXactIdToOffsetPage(multi);
	entryno = MultiXactIdToOffsetEntry(multi);

	/*
	 * Write out dirty data, so PhysicalPageExists can work correctly.
	 */
	SimpleLruWriteAll(MultiXactOffsetCtl, true);
	SimpleLruWriteAll(MultiXactMemberCtl, true);

	if (!SimpleLruDoesPhysicalPageExist(MultiXactOffsetCtl, pageno))
		return false;

	/* lock is acquired by SimpleLruReadPage_ReadOnly */
	slotno = SimpleLruReadPage_ReadOnly(MultiXactOffsetCtl, pageno, &multi);
	offptr = (MultiXactOffset *) MultiXactOffsetCtl->shared->page_buffer[slotno];
	offptr += entryno;
	offset = *offptr;
	LWLockRelease(SimpleLruGetBankLock(MultiXactOffsetCtl, pageno));

	*result = offset;
	return true;
}
