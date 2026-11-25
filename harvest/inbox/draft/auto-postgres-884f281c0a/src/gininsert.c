// AUTO-DRAFT from postgres/postgres PR #b4cc5e924b7a85f76637227002eb1a4be443d956
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct GinBuildShared
{
	/*
	 * These fields are not modified during the build.  They primarily exist
	 * for the benefit of worker processes that need to create state
	 * corresponding to that used by the leader.
	 */
	Oid			heaprelid;
	Oid			indexrelid;
	bool		isconcurrent;
	int			scantuplesortstates;

	/*
	 * workersdonecv is used to monitor the progress of workers.  All parallel
	 * participants must indicate that they are done before leader can use
	 * results built by the workers (and before leader can write the data into
	 * the index).
	 */
	ConditionVariable workersdonecv;

	/*
	 * mutex protects all following fields
	 *
	 * These fields contain status information of interest to GIN index builds
	 * that must work just the same when an index is built in parallel.
	 */
	slock_t		mutex;

	/*
	 * Mutable state that is maintained by workers, and reported back to
	 * leader at end of the scans.
	 *
	 * nparticipantsdone is number of worker processes finished.
	 *
	 * reltuples is the total number of input heap tuples.
	 *
	 * indtuples is the total number of tuples that made it into the index.
	 *
	 * brokenhotchain indicates if any worker detected a broken HOT chain
	 * during build.
	 */
	int			nparticipantsdone;
	double		reltuples;
	double		indtuples;
	bool		brokenhotchain;

	/*
	 * ParallelTableScanDescData data follows. Can't directly embed here, as
	 * implementations of the parallel table scan desc interface might need
	 * stronger alignment.
	 */
} GinBuildShared;
/* …（同文件无关代码省略）… */
typedef struct GinLeader
{
	/* parallel context itself */
	ParallelContext *pcxt;

	/*
	 * nparticipanttuplesorts is the exact number of worker processes
	 * successfully launched, plus one leader process if it participates as a
	 * worker (only DISABLE_LEADER_PARTICIPATION builds avoid leader
	 * participating as a worker).
	 */
	int			nparticipanttuplesorts;

	/*
	 * Leader process convenience pointers to shared state (leader avoids TOC
	 * lookups).
	 *
	 * GinBuildShared is the shared state for entire build.  sharedsort is the
	 * shared, tuplesort-managed state passed to each process tuplesort.
	 * snapshot is the snapshot used by the scan iff an MVCC snapshot is
	 * required.
	 */
	GinBuildShared *ginshared;
	Sharedsort *sharedsort;
	Snapshot	snapshot;
	WalUsage   *walusage;
	BufferUsage *bufferusage;
} GinLeader;
/* …（同文件无关代码省略）… */
typedef struct
{
	GinState	ginstate;
	double		indtuples;
	GinStatsData buildStats;
	MemoryContext tmpCtx;
	MemoryContext funcCtx;
	BuildAccumulator accum;
	ItemPointerData tid;
	int			work_mem;

	/*
	 * bs_leader is only present when a parallel index build is performed, and
	 * only in the leader process.
	 */
	GinLeader  *bs_leader;

	/* number of participating workers (including leader) */
	int			bs_num_workers;

	/* used to pass information from workers to leader */
	double		bs_numtuples;
	double		bs_reltuples;

	/*
	 * The sortstate is used by workers (including the leader). It has to be
	 * part of the build state, because that's the only thing passed to the
	 * build callback etc.
	 */
	Tuplesortstate *bs_sortstate;

	/*
	 * The sortstate used only within a single worker for the first merge pass
	 * happening there. In principle it doesn't need to be part of the build
	 * state and we could pass it around directly, but it's more convenient
	 * this way. And it's part of the build state, after all.
	 */
	Tuplesortstate *bs_worker_sort;
} GinBuildState;
/* …（同文件无关代码省略）… */
static IndexTuple
addItemPointersToLeafTuple(GinState *ginstate,
						   IndexTuple old,
						   ItemPointerData *items, uint32 nitem,
						   GinStatsData *buildStats, Buffer buffer)
{
	OffsetNumber attnum;
	Datum		key;
	GinNullCategory category;
	IndexTuple	res;
	ItemPointerData *newItems,
			   *oldItems;
	int			oldNPosting,
				newNPosting,
				nwritten;
	GinPostingList *compressedList;

	Assert(!GinIsPostingTree(old));

	attnum = gintuple_get_attrnum(ginstate, old);
	key = gintuple_get_key(ginstate, old, &category);

	/* merge the old and new posting lists */
	oldItems = ginReadTuple(ginstate, attnum, old, &oldNPosting);

	newItems = ginMergeItemPointers(items, nitem,
									oldItems, oldNPosting,
									&newNPosting);

	/* Compress the posting list, and try to a build tuple with room for it */
	res = NULL;
	compressedList = ginCompressPostingList(newItems, newNPosting, GinMaxItemSize, &nwritten);
	if (nwritten == newNPosting)
	{
		res = GinFormTuple(ginstate, attnum, key, category,
						   (char *) compressedList,
						   SizeOfGinPostingList(compressedList),
						   newNPosting,
						   false);
	}

	pfree(newItems);
	pfree(compressedList);

	if (!res)
	{
		/* posting list would be too big, convert to posting tree */
		BlockNumber postingRoot;

		/*
		 * Initialize posting tree with the old tuple's posting list.  It's
		 * surely small enough to fit on one posting-tree page, and should
		 * already be in order with no duplicates.
		 */
		postingRoot = createPostingTree(ginstate->index,
										oldItems,
										oldNPosting,
										buildStats,
										buffer);

		/* Now insert the TIDs-to-be-added into the posting tree */
		ginInsertItemPointers(ginstate->index, postingRoot,
							  items, nitem,
							  buildStats);

		/* And build a new posting-tree-only result tuple */
		res = GinFormTuple(ginstate, attnum, key, category, NULL, 0, 0, true);
		GinSetPostingTree(res, postingRoot);
	}
	pfree(oldItems);

	return res;
}
/* …（同文件无关代码省略）… */
static IndexTuple
buildFreshLeafTuple(GinState *ginstate,
					OffsetNumber attnum, Datum key, GinNullCategory category,
					ItemPointerData *items, uint32 nitem,
					GinStatsData *buildStats, Buffer buffer)
{
	IndexTuple	res = NULL;
	GinPostingList *compressedList;
	int			nwritten;

	/* try to build a posting list tuple with all the items */
	compressedList = ginCompressPostingList(items,
