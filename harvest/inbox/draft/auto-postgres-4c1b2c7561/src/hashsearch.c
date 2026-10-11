// AUTO-DRAFT from postgres/postgres PR #173cf85a079516fdd2b09ed853702b05fbfcd15e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include "storage/predicate.h"
#include "utils/rel.h"

static bool _hash_readpage(IndexScanDesc scan, Buffer *bufP,
						   ScanDirection dir);
static int	_hash_load_qualified_items(IndexScanDesc scan, Page page,
									   OffsetNumber offnum, ScanDirection dir);
static inline void _hash_saveitem(HashScanOpaque so, int itemIndex,
								  OffsetNumber offnum, IndexTuple itup);
static void _hash_readnext(IndexScanDesc scan, Buffer *bufp,
						   Page *pagep, HashPageOpaque *opaquep);

/*
 *	_hash_next() -- Get the next item in a scan.
/* …（同文件无关代码省略）… */
				if (!_hash_readpage(scan, &buf, dir))
					end_of_scan = true;
			}
			else
				end_of_scan = true;
		}
/* …（同文件无关代码省略）… */
				if (!_hash_readpage(scan, &buf, dir))
					end_of_scan = true;
			}
			else
				end_of_scan = true;
		}
	}

	if (end_of_scan)
	{
		_hash_dropscanbuf(rel, so);
		HashScanPosInvalidate(so->currPos);
		return false;
/* …（同文件无关代码省略）… */
static void
_hash_readnext(IndexScanDesc scan,
			   Buffer *bufp, Page *pagep, HashPageOpaque *opaquep)
{
	BlockNumber blkno;
	Relation	rel = scan->indexRelation;
	HashScanOpaque so = (HashScanOpaque) scan->opaque;
	bool		block_found = false;

	blkno = (*opaquep)->hasho_nextblkno;

	/*
	 * Retain the pin on primary bucket page till the end of scan.  Refer the
	 * comments in _hash_first to know the reason of retaining pin.
	 */
	if (*bufp == so->hashso_bucket_buf || *bufp == so->hashso_split_bucket_buf)
		LockBuffer(*bufp, BUFFER_LOCK_UNLOCK);
	else
		_hash_relbuf(rel, *bufp);

	*bufp = InvalidBuffer;
	/* check for interrupts while we're not holding any buffer lock */
	CHECK_FOR_INTERRUPTS();
	if (BlockNumberIsValid(blkno))
	{
		*bufp = _hash_getbuf(rel, blkno, HASH_READ, LH_OVERFLOW_PAGE);
		block_found = true;
	}
	else if (so->hashso_buc_populated && !so->hashso_buc_split)
	{
		/*
		 * end of bucket, scan bucket being split if there was a split in
		 * progress at the start of scan.
		 */
		*bufp = so->hashso_split_bucket_buf;

		/*
		 * buffer for bucket being split must be valid as we acquire the pin
		 * on it before the start of scan and retain it till end of scan.
		 */
		Assert(BufferIsValid(*bufp));

		LockBuffer(*bufp, BUFFER_LOCK_SHARE);
		PredicateLockPage(rel, BufferGetBlockNumber(*bufp), scan->xs_snapshot);

		/*
		 * setting hashso_buc_split to true indicates that we are scanning
		 * bucket being split.
		 */
		so->hashso_buc_split = true;

		block_found = true;
	}

	if (block_found)
	{
		*pagep = BufferGetPage(*bufp);
		*opaquep = HashPageGetOpaque(*pagep);
	}
}
/* …（同文件无关代码省略）… */
static void
_hash_readprev(IndexScanDesc scan,
			   Buffer *bufp, Page *pagep, HashPageOpaque *opaquep)
{
	BlockNumber blkno;
	Relation	rel = scan->indexRelation;
	HashScanOpaque so = (HashScanOpaque) scan->opaque;
	bool		haveprevblk;

	blkno = (*opaquep)->hasho_prevblkno;

	/*
	 * Retain the pin on primary bucket page till the end of scan.  Refer the
	 * comments in _hash_first to know the reason of retaining pin.
	 */
	if (*bufp == so->hashso_bucket_buf || *bufp == so->hashso_split_bucket_buf)
	{
		LockBuffer(*bufp, BUFFER_LOCK_UNLOCK);
		haveprevblk = false;
	}
	else
	{
		_hash_relbuf(rel, *bufp);
		haveprevblk = true;
	}

	*bufp = InvalidBuffer;
	/* check for interrupts while we're not holding any buffer lock */
	CHECK_FOR_INTERRUPTS();

	if (haveprevblk)
	{
		Assert(BlockNumberIsValid(blkno));
		*bufp = _hash_getbuf(rel, blkno, HASH_READ,
							 LH_BUCKET_PAGE | LH_OVERFLOW_PAGE);
		*pagep = BufferGetPage(*bufp);
		*opaquep = HashPageGetOpaque(*pagep);

		/*
		 * We always maintain the pin on bucket page for whole scan operation,
		 * so releasing the additional pin we have acquired here.
		 */
		if (*bufp == so->hashso_bucket_buf || *bufp == so->hashso_split_bucket_buf)
			_hash_dropbuf(rel, *bufp);
	}
	else if (so->hashso_buc_populated && so->hashso_buc_split)
	{
		/*
		 * end of bucket, scan bucket being populated if there was a split in
		 * progress at the start of scan.
		 */
		*bufp = so->hashso_bucket_buf;

		/*
		 * buffer for bucket being populated must be valid as we acquire the
		 * pin on it before the start of scan and retain it till end of scan.
		 */
		Assert(BufferIsValid(*bufp));

		LockBuffer(*bufp, BUFFER_LOCK_SHARE);
		*pagep = BufferGetPage(*bufp);
		*opaquep = HashPageGetOpaque(*pagep);

		/* move to the end of bucket chain */
		while (BlockNumberIsValid((*opaquep)->hasho_nextblkno))
			_hash_readnext(scan, bufp, pagep, opaquep);

		/*
		 * setting hashso_buc_split to false indicates that we are scanning
		 * bucket being populated.
		 */
		so->hashso_buc_split = false;
	}
}

/*
/* …（同文件无关代码省略）… */
static bool
_hash_readpage(IndexScanDesc scan, Buffer *bufP, ScanDirection dir)
{
	Relation	rel = scan->indexRelation;
	HashScanOpaque so = (HashScanOpaque) scan->opaque;
	Buffer		buf;
	Page		page;
	HashPageOpaque opaque;
	OffsetNumber offnum;
	uint16		itemIndex;

	buf = *bufP;
	Assert(BufferIsValid(buf));
	_hash_checkpage(rel, buf, LH_BUCKET_PAGE | LH_OVERFLOW_PAGE);
	page = BufferGetPage(buf);
	opaque = HashPageGetOpaque(page);

	so->currPos.buf = buf;
	so->currPos.currPage = BufferGetBlockNumber(buf);

	if (ScanDirectionIsForward(dir))
	{
		BlockNumber prev_blkno = InvalidBlockNumber;

		for (;;)
		{
			/* new page, locate starting position by binary search */
			offnum = _hash_binsearch(page, so->hashso_sk_hash);

			itemIndex = _hash_load_qualified_items(scan, page, offnum, dir);

			if (itemIndex != 0)
				break;

			/*
			 * Could not find any matching tuples in the current page, move to
			 * the next page. Before leaving the current page, deal with any
			 * killed items.
			 */
			if (so->numKilled > 0)
				_hash_kill_items(scan);

			/*
			 * If this is a primary bucket page, hasho_prevblkno is not a real
			 * block number.
			 */
			if (so->currPos.buf == so->hashso_bucket_buf ||
				so->currPos.buf == so->hashso_split_bucket_buf)
				prev_blkno = InvalidBlockNumber;
			e
