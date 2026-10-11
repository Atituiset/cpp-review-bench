// AUTO-DRAFT from postgres/postgres PR #173cf85a079516fdd2b09ed853702b05fbfcd15e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#include "port/pg_bitutils.h"
#include "storage/predicate.h"
#include "storage/smgr.h"
#include "utils/rel.h"

static bool _hash_alloc_buckets(Relation rel, BlockNumber firstblock,
/* …（同文件无关代码省略）… */
Buffer
_hash_getbuf(Relation rel, BlockNumber blkno, int access, int flags)
{
	Buffer		buf;

	if (blkno == P_NEW)
		elog(ERROR, "hash AM does not use P_NEW");

	buf = ReadBuffer(rel, blkno);

	if (access != HASH_NOLOCK)
		LockBuffer(buf, access);

	/* ref count and lock type are correct */

	_hash_checkpage(rel, buf, flags);

	return buf;
}
/* …（同文件无关代码省略）… */
void
_hash_relbuf(Relation rel, Buffer buf)
{
	UnlockReleaseBuffer(buf);
}
/* …（同文件无关代码省略）… */
void
_hash_pageinit(Page page, Size size)
{
	PageInit(page, size, sizeof(HashPageOpaqueData));
}
/* …（同文件无关代码省略）… */
static bool
_hash_alloc_buckets(Relation rel, BlockNumber firstblock, uint32 nblocks)
{
	BlockNumber lastblock;
	PGIOAlignedBlock zerobuf;
	Page		page;
	HashPageOpaque ovflopaque;

	lastblock = firstblock + nblocks - 1;

	/*
	 * Check for overflow in block number calculation; if so, we cannot extend
	 * the index anymore.
	 */
	if (lastblock < firstblock || lastblock == InvalidBlockNumber)
		return false;

	page = (Page) zerobuf.data;

	/*
	 * Initialize the page.  Just zeroing the page won't work; see
	 * _hash_freeovflpage for similar usage.  We take care to make the special
	 * space valid for the benefit of tools such as pageinspect.
	 */
	_hash_pageinit(page, BLCKSZ);

	ovflopaque = HashPageGetOpaque(page);

	ovflopaque->hasho_prevblkno = InvalidBlockNumber;
	ovflopaque->hasho_nextblkno = InvalidBlockNumber;
	ovflopaque->hasho_bucket = InvalidBucket;
	ovflopaque->hasho_flag = LH_UNUSED_PAGE;
	ovflopaque->hasho_page_id = HASHO_PAGE_ID;

	if (RelationNeedsWAL(rel))
		log_newpage(&rel->rd_locator,
					MAIN_FORKNUM,
					lastblock,
					zerobuf.data,
					true);

	PageSetChecksum(page, lastblock);
	smgrextend(RelationGetSmgr(rel), MAIN_FORKNUM, lastblock, zerobuf.data,
			   false);

	return true;
}
/* …（同文件无关代码省略）… */
static void
_hash_splitbucket(Relation rel,
				  Buffer metabuf,
				  Bucket obucket,
				  Bucket nbucket,
				  Buffer obuf,
				  Buffer nbuf,
				  HTAB *htab,
				  uint32 maxbucket,
				  uint32 highmask,
				  uint32 lowmask)
{
	Buffer		bucket_obuf;
	Buffer		bucket_nbuf;
	Page		opage;
	Page		npage;
	HashPageOpaque oopaque;
	HashPageOpaque nopaque;
	OffsetNumber itup_offsets[MaxIndexTuplesPerPage];
	IndexTuple	itups[MaxIndexTuplesPerPage];
	Size		all_tups_size = 0;
	int			i;
	uint16		nitups = 0;
	XLogRecPtr	recptr;

	bucket_obuf = obuf;
	opage = BufferGetPage(obuf);
	oopaque = HashPageGetOpaque(opage);

	bucket_nbuf = nbuf;
	npage = BufferGetPage(nbuf);
	nopaque = HashPageGetOpaque(npage);

	/* Copy the predicate locks from old bucket to new bucket. */
	PredicateLockPageSplit(rel,
						   BufferGetBlockNumber(bucket_obuf),
						   BufferGetBlockNumber(bucket_nbuf));

	/*
	 * Partition the tuples in the old bucket between the old bucket and the
	 * new bucket, advancing along the old bucket's overflow bucket chain and
	 * adding overflow pages to the new bucket as needed.  Outer loop iterates
	 * once per page in old bucket.
	 */
	for (;;)
	{
		BlockNumber oblkno;
		OffsetNumber ooffnum;
		OffsetNumber omaxoffnum;

		/* Scan each tuple in old page */
		omaxoffnum = PageGetMaxOffsetNumber(opage);
		for (ooffnum = FirstOffsetNumber;
			 ooffnum <= omaxoffnum;
			 ooffnum = OffsetNumberNext(ooffnum))
		{
			IndexTuple	itup;
			Size		itemsz;
			Bucket		bucket;
			bool		found = false;

			/* skip dead tuples */
			if (ItemIdIsDead(PageGetItemId(opage, ooffnum)))
				continue;

			/*
			 * Before inserting a tuple, probe the hash table containing TIDs
			 * of tuples belonging to new bucket, if we find a match, then
			 * skip that tuple, else fetch the item's hash key (conveniently
			 * stored in the item) and determine which bucket it now belongs
			 * in.
			 */
			itup = (IndexTuple) PageGetItem(opage,
											PageGetItemId(opage, ooffnum));

			if (htab)
				(void) hash_search(htab, &itup->t_tid, HASH_FIND, &found);

			if (found)
				continue;

			bucket = _hash_hashkey2bucket(_hash_get_indextuple_hashkey(itup),
										  maxbucket, highmask, lowmask);

			if (bucket == nbucket)
			{
				IndexTuple	new_itup;

				/*
				 * make a copy of index tuple as we have to scribble on it.
				 */
				new_itup = CopyIndexTuple(itup);

				/*
				 * mark the index tuple as moved by split, such tuples are
				 * skipped by scan if there is split in progress for a bucket.
				 */
				new_itup->t_info |= INDEX_MOVED_BY_SPLIT_MASK;

				/*
				 * insert the tuple into the new bucket.  if it doesn't fit on
				 * the current page in the new bucket, we must allocate a new
				 * overflow page and place the tuple on that page instead.
				 */
				itemsz = IndexTupleSize(new_itup);
				itemsz = MAXALIGN(itemsz);

				if (PageGetFreeSpaceForMultipleTuples(npage, nitups + 1) < (all_tups_size + itemsz))
				{
					/*
					 * Change the shared buffer state in critical section,
					 * otherwise any error could make it unrecoverable.
					 */
					START_CRIT_SECTION();

					_hash_pgaddmultitup(rel, nbuf, itups, itup_offsets, nitups);
					MarkBufferDirty(nbuf);
					/* log the split operation before releasing the lock */
					log_split_page(rel, nbuf);

					END_CRIT_SECTION();

					/* drop lock, but keep pin */
					LockBuffer(nbuf, BUFFER_LOCK_UNLOCK);

					/* be tidy */
					for (i = 0; i < nitups; i++)
						pfree(itups[i]);
					nitups = 0;
					all_tups_size = 0;

					/* chain to a new overflow page */
					nbuf = _hash_addovflpage(rel, metabuf, nbuf, (nbuf == bucket_nbuf));
					npage = BufferGetPage(nbuf);
					nopaque = HashPageGetOpaque(npage);
				}

				itups[nitups++] = new_itup;
				all_tups_size += itemsz;
			}
			else
			{
				/*
				 * the tuple stays on this page, so nothing to do.
				 *
