// AUTO-DRAFT from postgres/postgres PR #b3dbde59376f66e063ceacc03b6d2102825de692
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define HASH_SEGSIZE			   256
#define HASH_SEGSIZE_SHIFT	   8	/* must be log2(HASH_SEGSIZE) */
/* …（同文件无关代码省略）… */
#define NUM_FREELISTS			32
/* …（同文件无关代码省略）… */
typedef HASHELEMENT *HASHBUCKET;
/* …（同文件无关代码省略）… */
typedef HASHBUCKET *HASHSEGMENT;
/* …（同文件无关代码省略）… */
#define IS_PARTITIONED(hctl)  ((hctl)->num_partitions != 0)
/* …（同文件无关代码省略）… */
	char	   *tabname;		/* table name (for error messages) */
	bool		isshared;		/* true if table is in shared memory */

	/* freezing a shared table isn't allowed, so we can keep state here */
	bool		frozen;			/* true = no more inserts allowed */

	/* We keep local copies of these fixed values to reduce contention */
	Size		keysize;		/* hash key length in bytes */

/* …（同文件无关代码省略）… */
#define ELEMENTKEY(helem)  (((char *)(helem)) + MAXALIGN(sizeof(HASHELEMENT)))
/* …（同文件无关代码省略）… */
#define MOD(x,y)			   ((x) & ((y)-1))
/* …（同文件无关代码省略）… */
static void *
DynaHashAlloc(Size size, void *alloc_arg)
{
	MemoryContext hcxt = (MemoryContext) alloc_arg;

	Assert(MemoryContextIsValid(hcxt));
	return MemoryContextAllocExtended(hcxt, size, MCXT_ALLOC_NO_OOM);
}
/* …（同文件无关代码省略）… */
				(errcode(ERRCODE_OUT_OF_MEMORY),
				 errmsg("out of memory")));

	hashp->frozen = false;

	hdefault(hashp);

	hctl = hashp->hctl;
/* …（同文件无关代码省略）… */
static void
hdefault(HTAB *hashp)
{
	HASHHDR    *hctl = hashp->hctl;

	MemSet(hctl, 0, sizeof(HASHHDR));

	hctl->num_partitions = 0;	/* not partitioned */

	/* table has no fixed maximum size */
	hctl->max_dsize = NO_MAX_DSIZE;

	hctl->isfixed = false;		/* can be enlarged */

#ifdef HASH_STATISTICS
	hctl->accesses = hctl->collisions = hctl->expansions = 0;
#endif
}
/* …（同文件无关代码省略）… */
static inline uint32
calc_bucket(HASHHDR *hctl, uint32 hash_val)
{
	uint32		bucket;

	bucket = hash_val & hctl->high_mask;
	if (bucket > hctl->max_bucket)
		bucket = bucket & hctl->low_mask;

	return bucket;
}
/* …（同文件无关代码省略）… */
	if (action == HASH_ENTER || action == HASH_ENTER_NULL)
	{
		/*
		 * Can't split if running in partitioned mode, nor if frozen, nor if
		 * table is the subject of any active hash_seq_search scans.
		 */
		if (hctl->freeList[0].nentries > (int64) hctl->max_bucket &&
			!hctl->isfixed && !IS_PARTITIONED(hctl) && !hashp->frozen &&
			!has_seq_scans(hashp))
			(void) expand_table(hashp);
	}
/* …（同文件无关代码省略）… */
			if (currBucket != NULL)
				return ELEMENTKEY(currBucket);

			/* disallow inserts if frozen */
			if (hashp->frozen)
				elog(ERROR, "cannot insert into frozen hashtable \"%s\"",
					 hashp->tabname);

			currBucket = get_hash_entry(hashp, freelist_idx);
			if (currBucket == NULL)
			{
/* …（同文件无关代码省略）… */
	hctl->accesses++;
#endif

	/* disallow updates if frozen */
	if (hashp->frozen)
		elog(ERROR, "cannot update in frozen hashtable \"%s\"",
			 hashp->tabname);

	/*
	 * Lookup the existing element using its saved hash value.  We need to do
	 * this to be able to unlink it from its hash chain, but as a side benefit
/* …（同文件无关代码省略）… */
static HASHBUCKET
get_hash_entry(HTAB *hashp, int freelist_idx)
{
	HASHHDR    *hctl = hashp->hctl;
	HASHBUCKET	newElement;

	for (;;)
	{
		/* if partitioned, must lock to touch nentries and freeList */
		if (IS_PARTITIONED(hctl))
			SpinLockAcquire(&hctl->freeList[freelist_idx].mutex);

		/* try to get an entry from the freelist */
		newElement = hctl->freeList[freelist_idx].freeList;

		if (newElement != NULL)
			break;

		if (IS_PARTITIONED(hctl))
			SpinLockRelease(&hctl->freeList[freelist_idx].mutex);

		/*
		 * No free elements in this freelist.  In a partitioned table, there
		 * might be entries in other freelists, but to reduce contention we
		 * prefer to first try to get another chunk of buckets from the main
		 * shmem allocator.  If that fails, though, we *MUST* root through all
		 * the other freelists before giving up.  There are multiple callers
		 * that assume that they can allocate every element in the initially
		 * requested table size, or that deleting an element guarantees they
		 * can insert a new element, even if shared memory is entirely full.
		 * Failing because the needed element is in a different freelist is
		 * not acceptable.
		 */
		if (!element_alloc(hashp, hctl->nelem_alloc, freelist_idx))
		{
			int			borrow_from_idx;

			if (!IS_PARTITIONED(hctl))
				return NULL;	/* out of memory */

			/* try to borrow element from another freelist */
			borrow_from_idx = freelist_idx;
			for (;;)
			{
				borrow_from_idx = (borrow_from_idx + 1) % NUM_FREELISTS;
				if (borrow_from_idx == freelist_idx)
					break;		/* examined all freelists, fail */

				SpinLockAcquire(&(hctl->freeList[borrow_from_idx].mutex));
				newElement = hctl->freeList[borrow_from_idx].freeList;

				if (newElement != NULL)
				{
					hctl->freeList[borrow_from_idx].freeList = newElement->link;
					SpinLockRelease(&(hctl->freeList[borrow_from_idx].mutex));

					/* careful: count the new element in its proper freelist */
					SpinLockAcquire(&hctl->freeList[freelist_idx].mutex);
					hctl->freeList[freelist_idx].nentries++;
					SpinLockRelease(&hctl->freeList[freelist_idx].mutex);

					return newElement;
				}

				SpinLockRelease(&(hctl->freeList[borrow_from_idx].mutex));
			}

			/* no elements available to borrow either, so out of memory */
			return NULL;
		}
	}

	/* remove entry from freelist, bump nentries */
	hctl->freeList[freelist_idx].freeList = newElement->link;
	hctl->freeList[freelist_idx].nentries++;

	if (IS_PARTITIONED(hctl))
		SpinLockRelease(&hctl->freeList[freelist_idx].mutex);

	return newElement;
}
/* …（同文件无关代码省略）… */
 * if elements are added to the table while the scan is in progress, it is
 * unspecified whether they will be visited by the scan or not.
 *
 * NOTE: it is possible to use hash_seq_init/hash_seq_search without any
 * worry about hash_seq_term cleanup, if the hashtable is first locked against
 * further insertions by calling hash_freeze.
 *
 * 
