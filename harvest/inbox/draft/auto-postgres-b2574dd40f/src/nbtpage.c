// AUTO-DRAFT from postgres/postgres PR #1732ab2757af722a71e17160e179ca7f80980778
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
void
_bt_checkpage(Relation rel, Buffer buf)
{
	Page		page = BufferGetPage(buf);

	/*
	 * ReadBuffer verifies that every newly-read page passes
	 * PageHeaderIsValid, which means it either contains a reasonably sane
	 * page header or is all-zero.  We have to defend against the all-zero
	 * case, however.
	 */
	if (PageIsNew(page))
		ereport(ERROR,
				(errcode(ERRCODE_INDEX_CORRUPTED),
				 errmsg("index \"%s\" contains unexpected zero page at block %u",
						RelationGetRelationName(rel),
						BufferGetBlockNumber(buf)),
				 errhint("Please REINDEX it.")));

	/*
	 * Additionally check that the special area looks sane.
	 */
	if (PageGetSpecialSize(page) != MAXALIGN(sizeof(BTPageOpaqueData)))
		ereport(ERROR,
				(errcode(ERRCODE_INDEX_CORRUPTED),
				 errmsg("index \"%s\" contains corrupted page at block %u",
						RelationGetRelationName(rel),
						BufferGetBlockNumber(buf)),
				 errhint("Please REINDEX it.")));
}
/* …（同文件无关代码省略）… */
Buffer
_bt_getbuf(Relation rel, BlockNumber blkno, int access)
{
	Buffer		buf;

	Assert(BlockNumberIsValid(blkno));

	/* Read an existing block of the relation */
	buf = ReadBuffer(rel, blkno);
	_bt_lockbuf(rel, buf, access);
	_bt_checkpage(rel, buf);

	return buf;
}
/* …（同文件无关代码省略）… */
void
_bt_relbuf(Relation rel, Buffer buf)
{
	/*
	 * Buffer is pinned and locked, which means that it is expected to be
	 * defined and addressable.  Check that proactively.
	 */
	VALGRIND_CHECK_MEM_IS_DEFINED(BufferGetPage(buf), BLCKSZ);
	if (!RelationUsesLocalBuffers(rel))
		VALGRIND_MAKE_MEM_NOACCESS(BufferGetPage(buf), BLCKSZ);

	UnlockReleaseBuffer(buf);
}
/* …（同文件无关代码省略）… */
void
_bt_lockbuf(Relation rel, Buffer buf, int access)
{
	/* LockBuffer() asserts that pin is held by this backend */
	LockBuffer(buf, access);

	/*
	 * It doesn't matter that _bt_unlockbuf() won't get called in the event of
	 * an nbtree error (e.g. a unique violation error).  That won't cause
	 * Valgrind false positives.
	 *
	 * The nbtree client requests are superimposed on top of the bufmgr.c
	 * buffer pin client requests.  In the event of an nbtree error the buffer
	 * will certainly get marked as defined when the backend once again
	 * acquires its first pin on the buffer. (Of course, if the backend never
	 * touches the buffer again then it doesn't matter that it remains
	 * non-accessible to Valgrind.)
	 *
	 * Note: When an IndexTuple C pointer gets computed using an ItemId read
	 * from a page while a lock was held, the C pointer becomes unsafe to
	 * dereference forever as soon as the lock is released.  Valgrind can only
	 * detect cases where the pointer gets dereferenced with no _current_
	 * lock/pin held, though.
	 */
	if (!RelationUsesLocalBuffers(rel))
		VALGRIND_MAKE_MEM_DEFINED(BufferGetPage(buf), BLCKSZ);
}
/* …（同文件无关代码省略）… */
static bool
_bt_leftsib_splitflag(Relation rel, BlockNumber leftsib, BlockNumber target)
{
	Buffer		buf;
	Page		page;
	BTPageOpaque opaque;
	bool		result;

	/* Easy case: No left sibling */
	if (leftsib == P_NONE)
		return false;

	buf = _bt_getbuf(rel, leftsib, BT_READ);
	page = BufferGetPage(buf);
	opaque = BTPageGetOpaque(page);

	/*
	 * If the left sibling was concurrently split, so that its next-pointer
	 * doesn't point to the current page anymore, the split that created
	 * target must be completed.  Caller can reasonably expect that there will
	 * be a downlink to the target page that it can relocate using its stack.
	 * (We don't allow splitting an incompletely split page again until the
	 * previous split has been completed.)
	 */
	result = (opaque->btpo_next == target && P_INCOMPLETE_SPLIT(opaque));
	_bt_relbuf(rel, buf);

	return result;
}
/* …（同文件无关代码省略）… */
		 * left half of an incomplete split, but ensuring that it's not the
		 * right half is more complicated.  For that, we have to check that
		 * the left sibling doesn't have its INCOMPLETE_SPLIT flag set using
		 * _bt_leftsib_splitflag().  On the first iteration, we temporarily
		 * release the lock on scanblkno/leafbuf, check the left sibling, and
		 * construct a search stack to scanblkno.  On subsequent iterations,
		 * we know we stepped right from a page that passed these tests, so
		 * it's OK.
		 */
		if (P_RIGHTMOST(opaque) || P_ISROOT(opaque) ||
			P_FIRSTDATAKEY(opaque) <= PageGetMaxOffsetNumber(page) ||
/* …（同文件无关代码省略）… */
				 * Check that the left sibling of leafbuf (if any) is not
				 * marked with INCOMPLETE_SPLIT flag before proceeding
				 */
				Assert(leafblkno == scanblkno);
				if (_bt_leftsib_splitflag(rel, leftsib, leafblkno))
				{
					ReleaseBuffer(leafbuf);
