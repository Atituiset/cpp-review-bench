// AUTO-DRAFT from postgres/postgres PR #425daf545d9146e008220ae0b21415982220cd3f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR

#include "postgres.h"

#include "access/amapi.h"
#include "access/genam.h"
#include "access/htup_details.h"
#include "access/skey.h"
/* …（同文件无关代码省略）… */
#define RI_MAX_NUMKEYS					INDEX_MAX_KEYS
/* …（同文件无关代码省略）… */
#define RI_INIT_CONSTRAINTHASHSIZE		64
#define RI_INIT_QUERYHASHSIZE			(RI_INIT_CONSTRAINTHASHSIZE * 4)
/* …（同文件无关代码省略）… */
#define RI_PLAN_CHECK_LOOKUPPK			1
/* …（同文件无关代码省略）… */
#define RIAttType(rel, attnum)	attnumTypeId(rel, attnum)
#define RIAttCollation(rel, attnum) attnumCollationId(rel, attnum)
/* …（同文件无关代码省略）… */
typedef struct FastPathMeta FastPathMeta;
/* …（同文件无关代码省略）… */
typedef enum RI_FastPathState
{
	RI_FASTPATH_UNKNOWN,
	RI_FASTPATH_USABLE,
	RI_FASTPATH_UNUSABLE
} RI_FastPathState;
/* …（同文件无关代码省略）… */
typedef struct RI_ConstraintInfo
{
	Oid			constraint_id;	/* OID of pg_constraint entry (hash key) */
	bool		valid;			/* successfully initialized? */
	Oid			constraint_root_id; /* OID of topmost ancestor constraint;
									 * same as constraint_id if not inherited */
	uint32		oidHashValue;	/* hash value of constraint_id */
	uint32		rootHashValue;	/* hash value of constraint_root_id */
	NameData	conname;		/* name of the FK constraint */
	Oid			pk_relid;		/* referenced relation */
	Oid			fk_relid;		/* referencing relation */
	char		confupdtype;	/* foreign key's ON UPDATE action */
	char		confdeltype;	/* foreign key's ON DELETE action */
	int			ndelsetcols;	/* number of columns referenced in ON DELETE
								 * SET clause */
	int16		confdelsetcols[RI_MAX_NUMKEYS]; /* attnums of cols to set on
												 * delete */
	char		confmatchtype;	/* foreign key's match type */
	bool		hasperiod;		/* if the foreign key uses PERIOD */
	int			nkeys;			/* number of key columns */
	int16		pk_attnums[RI_MAX_NUMKEYS]; /* attnums of referenced cols */
	int16		fk_attnums[RI_MAX_NUMKEYS]; /* attnums of referencing cols */
	Oid			pf_eq_oprs[RI_MAX_NUMKEYS]; /* equality operators (PK = FK) */
	Oid			pp_eq_oprs[RI_MAX_NUMKEYS]; /* equality operators (PK = PK) */
	Oid			ff_eq_oprs[RI_MAX_NUMKEYS]; /* equality operators (FK = FK) */
	Oid			period_contained_by_oper;	/* anyrange <@ anyrange (or
											 * multiranges) */
	Oid			agged_period_contained_by_oper; /* fkattr <@ range_agg(pkattr) */
	Oid			period_intersect_oper;	/* anyrange * anyrange (or
										 * multiranges) */
	dlist_node	valid_link;		/* Link in list of valid entries */

	Oid			conindid;
	bool		pk_is_partitioned;
	RI_FastPathState fastpath_state;	/* populated lazily under lock */

	FastPathMeta *fpmeta;
} RI_ConstraintInfo;
/* …（同文件无关代码省略）… */
typedef struct RI_CompareHashEntry RI_CompareHashEntry;
/* …（同文件无关代码省略）… */
typedef struct RI_QueryKey
{
	Oid			constr_id;		/* OID of pg_constraint entry */
	int32		constr_queryno; /* query type ID, see RI_PLAN_XXX above */
} RI_QueryKey;
/* …（同文件无关代码省略）… */
typedef struct RI_QueryHashEntry
{
	RI_QueryKey key;
	SPIPlanPtr	plan;
} RI_QueryHashEntry;
/* …（同文件无关代码省略）… */
typedef struct RI_CompareKey
{
	Oid			eq_opr;			/* the equality operator to apply */
	Oid			typeid;			/* the data type to apply it to */
} RI_CompareKey;
/* …（同文件无关代码省略）… */
	FmgrInfo	cast_func_finfo;	/* in case we must coerce input */
} RI_CompareHashEntry;

/*
 * Maximum number of FK rows buffered before flushing.
 *
 * Larger batches amortize per-flush overhead and let the SK_SEARCHARRAY
 * path walk more leaf pages in a single sorted traversal.  But each
 * buffered row is a materialized HeapTuple in flush_cxt, and the matched[]
 * scan in ri_FastPathFlushArray() is O(batch_size) per index match.
 * Benchmarking showed little difference between 16 and 64, with 256
 * consistently slower.  64 is a reasonable default.
 */
#define RI_FASTPATH_BATCH_SIZE	64

/*
 * RI_FastPathKey
 *		Hash key for an RI_FastPathEntry.
 *
 * A constraint can be checked in nested trigger-firing cycles.  Each cycle
 * must have a separate entry so that its rows are checked with that cycle's
 * snapshot and its resources are released by that cycle's callback.
 */
typedef struct RI_FastPathKey
{
	Oid			conoid;			/* pg_constraint OID */
	int			query_depth;	/* after-trigger query depth */
} RI_FastPathKey;

/*
 * RI_FastPathEntry
 *		Per-constraint, per-firing-cycle cache of resources needed by
 *		ri_FastPathBatchFlush().
 *
 * Created lazily by ri_FastPathGetEntry() on first use within a
 * trigger-firing batch and torn down by ri_FastPathTeardown() at batch end.
 *
 * FK tuples are buffered in batch[] across trigger invocations and
 * flushed when the buffer fills or the batch ends.
 *
 * RI_FastPathEntry is not subject to cache invalidation.  The cached
 * relations are held open with locks for the transaction duration, preventing
 * relcache invalidation.  The entry itself is torn down at batch end by
 * ri_FastPathEndBatch(); on abort, ResourceOwner releases the cached
 * relations and AtEOXact_RI() NULLs the static cache pointer to prevent
 * any subsequent access.
 */
typedef struct RI_FastPathEntry
{
	RI_FastPathKey key;			/* hash key */
	Oid			fk_relid;		/* for ri_FastPathEndBatch() */
	Relation	pk_rel;
	Relation	idx_rel;
	TupleTableSlot *pk_slot;
	TupleTableSlot *fk_slot;
	MemoryContext flush_cxt;	/* short-lived context for per-flush work */

	/*
	 * TODO: batch[] is HeapTuple[] because the AFTER trigger machinery
	 * currently passes tuples as HeapTuples.  Once trigger infrastructure is
	 * slotified, this should use a slot array or whatever batched tuple
	 * storage abstraction exists at that point to be TAM-agnostic.
	 */
	HeapTuple	batch[RI_FASTPATH_BATCH_SIZE];
	int			batch_count;

	/*
	 * true while this entry's batch is being flushed; guards against
	 * re-entrant ri_FastPathBatchAdd from user code run during the flush.
	 */
	bool		flushing;

	/*
	 * Subtransaction whose resource owner opened this entry's relations.
	 * AtEOSubXact_RI() drops only entries matching an aborting subxact, so a
	 * subxact abort during outer-level trigger firing leaves the outer batch
	 * intact.
	 */
	SubTransactionId subid;
} RI_FastPathEntry;

/*
 * Local data
 */
/* …（同文件无关代码省略）… */
static HTAB *ri_compare_cache = NULL;
static dclist_head ri_constraint_cache_valid_list;

static HTAB *ri_fastpath_cache = NULL;
static bool ri_fastpath_flushing = false;

/*
 * FastPathMeta objects detached from their cache entry by invalidation, but
 * possibly still referenced by an RI check further up the stack.  Released
/* …（同文件无关代码省略）… */
							bool detectNewRows, int expect_OK);
static bool ri_FastPathCheck(RI_ConstraintInfo *riinfo,
							 Relation fk_rel, TupleTableSlot *newslot);
static bool ri_FastPathBatchAdd(RI_ConstraintInfo *riinfo,
								Relation fk_rel, TupleTableSlot *newslot);
static void ri_FastPathBatchFlush(RI_FastPathEntry *fpentry, Relation fk_rel,
								  RI_ConstraintInfo *riinfo);
static int	ri_FastPathFlushArray(RI_FastPathEntry *fpentry, TupleTableSlot *fk_slot,
								  const RI_ConstraintInfo *riinfo,
								  FastPathMeta *fpmeta, Relation fk_rel,
								  Snapshot snapshot, IndexScanDesc scandesc);
static int	ri_FastPathFlushLoop(RI_FastPathEntry *fpentry, TupleTableSlot *fk_slot,
								 const RI_ConstraintInfo *riinfo,
								 FastPathMeta *fpmeta, Relation fk_rel,
								 Snapshot snapshot, IndexScanDesc scandesc);
static bool ri_FastPathProbeOne(Relation pk_rel, Relation idx_rel,
								IndexScanDesc scandesc, TupleTableSlot *slot,
								Snapshot snapshot, const RI_ConstraintInfo *riinfo,
/* …（同文件无关代码省略）… */
										   Relation pk_rel, Relation fk_rel,
										   TupleTableSlot *violatorslot, TupleDesc tupdesc,
										   int queryno, bool is_restrict, bool partgone);
static RI_FastPathEntry *ri_FastPathGetEntry(RI_ConstraintInfo *riinfo,
											 Relation fk_rel);
static void ri_FastPathEndBatch(void *arg);
static void ri_FastPathTeardown(int depth);


/*
/* …（同文件无关代码省略）… */
	 * lock.  This is semantically equivalent to the SPI path below but avoids
	 * the per-row executor overhead.
	 *
	 * ri_FastPathBatchAdd() and ri_FastPathCheck() report the violation
	 * themselves if no matching PK row is found.  They return false if the
	 * index checks made after opening the relations require a SPI fallback.
	 */
	if (ri_fastpath_is_applicable(riinfo))
	{
		if (AfterTriggerIsActive() && !ri_fastpath_flushing)
		{
			/* Batched path: buffer and probe in groups */
			if (ri_FastPathBatchAdd(riinfo, fk_rel, newslot))
				return PointerGetDatum(NULL);
		}
		else
		{
			/*
			 * Per-row path, used when batching is not applicable:
			 *
			 * - ALTER TABLE validation, where no after-trigger firing is
			 * active;
			 *
			 * - a re-entrant check from user cast/operator code running
			 * during a batch flush, since adding a cache entry while
			 * ri_FastPathEndBatch is iterating the cache could leave it
			 * unflushed.
			 */
			if (ri_FastPathCheck(riinfo, fk_rel, newslot))
				return PointerGetDatum(NULL);
		}
	}

	SPI_connect();

/* …（同文件无关代码省略）… */
static RI_ConstraintInfo *
ri_LoadConstraintInfo(Oid constraintOid)
{
	RI_ConstraintInfo *riinfo;
	bool		found;
	HeapTuple	tup;
	Form_pg_constraint conForm;

	/*
	 * On the first call initialize the hashtable
	 */
	if (!ri_constraint_cache)
		ri_InitHashTables();

	/*
	 * Find or create a hash entry.  If we find a valid one, just return it.
	 */
	riinfo = (RI_ConstraintInfo *) hash_search(ri_constraint_cache,
											   &constraintOid,
											   HASH_ENTER, &found);
	if (!found)
		riinfo->valid = false;
	else if (riinfo->valid)
		return riinfo;

	/*
	 * Fetch the pg_constraint row so we can fill in the entry.
	 */
	tup = SearchSysCache1(CONSTROID, ObjectIdGetDatum(constraintOid));
	if (!HeapTupleIsValid(tup)) /* should not happen */
		elog(ERROR, "cache lookup failed for constraint %u", constraintOid);
	conForm = (Form_pg_constraint) GETSTRUCT(tup);

	if (conForm->contype != CONSTRAINT_FOREIGN) /* should not happen */
		elog(ERROR, "constraint %u is not a foreign key constraint",
			 constraintOid);

	/* And extract data */
	Assert(riinfo->constraint_id == constraintOid);
	if (OidIsValid(conForm->conparentid))
		riinfo->constraint_root_id =
			get_ri_constraint_root(conForm->conparentid);
	else
		riinfo->constraint_root_id = constraintOid;
	riinfo->oidHashValue = GetSysCacheHashValue1(CONSTROID,
												 ObjectIdGetDatum(constraintOid));
	riinfo->rootHashValue = GetSysCacheHashValue1(CONSTROID,
												  ObjectIdGetDatum(riinfo->constraint_root_id));
	memcpy(&riinfo->conname, &conForm->conname, sizeof(NameData));
	riinfo->pk_relid = conForm->confrelid;
	riinfo->fk_relid = conForm->conrelid;
	riinfo->confupdtype = conForm->confupdtype;
	riinfo->confdeltype = conForm->confdeltype;
	riinfo->confmatchtype = conForm->confmatchtype;
	riinfo->hasperiod = conForm->conperiod;

	DeconstructFkConstraintRow(tup,
							   &riinfo->nkeys,
							   riinfo->fk_attnums,
							   riinfo->pk_attnums,
							   riinfo->pf_eq_oprs,
							   riinfo->pp_eq_oprs,
							   riinfo->ff_eq_oprs,
							   &riinfo->ndelsetcols,
							   riinfo->confdelsetcols);

	/*
	 * For temporal FKs, get the operators and functions we need. We ask the
	 * opclass of the PK element for these. This all gets cached (as does the
	 * generated plan), so there's no performance issue.
	 */
	if (riinfo->hasperiod)
	{
		Oid			opclass = get_index_column_opclass(conForm->conindid, riinfo->nkeys);

		FindFKPeriodOpers(opclass,
						  &riinfo->period_contained_by_oper,
						  &riinfo->agged_period_contained_by_oper,
						  &riinfo->period_intersect_oper);
	}

	/* Metadata used by fast path. */
	riinfo->conindid = conForm->conindid;
	riinfo->pk_is_partitioned =
		(get_rel_relkind(riinfo->pk_relid) == RELKIND_PARTITIONED_TABLE);
	riinfo->fastpath_state = RI_FASTPATH_UNKNOWN;

	ReleaseSysCache(tup);

	/*
	 * For efficient processing of invalidation messages below, we keep a
	 * doubly-linked count list of all currently valid entries.
	 */
	dclist_push_tail(&ri_constraint_cache_valid_list, &riinfo->valid_link);

	riinfo->valid = true;

	riinfo->fpmeta = NULL;

	return riinfo;
}
/* …（同文件无关代码省略）… */
static Oid
get_ri_constraint_root(Oid constrOid)
{
	for (;;)
	{
		HeapTuple	tuple;
		Oid			constrParentOid;

		tuple = SearchSysCache1(CONSTROID, ObjectIdGetDatum(constrOid));
		if (!HeapTupleIsValid(tuple))
			elog(ERROR, "cache lookup failed for constraint %u", constrOid);
		constrParentOid = ((Form_pg_constraint) GETSTRUCT(tuple))->conparentid;
		ReleaseSysCache(tuple);
		if (!OidIsValid(constrParentOid))
			break;				/* we reached the root constraint */
		constrOid = constrParentOid;
	}
	return constrOid;
}
/* …（同文件无关代码省略）… */
static void
InvalidateConstraintCacheCallBack(Datum arg, SysCacheIdentifier cacheid,
								  uint32 hashvalue)
{
	dlist_mutable_iter iter;

	Assert(ri_constraint_cache != NULL);

	/*
	 * pg_amop changes can affect any constraint's fast-path metadata, and
	 * this pg_amop hashvalue can't be matched against the pg_constraint-keyed
	 * cache entries, so flush them all via the match-everything path below as
	 * the large-list reset below does.  Being selective would mean mapping
	 * the change back to the affected constraints, not worth it for DDL this
	 * rare.
	 */
	if (cacheid == AMOPOPID)
		hashvalue = 0;

	/*
	 * If the list of currently valid entries gets excessively large, we mark
	 * them all invalid so we can empty the list.  This arrangement avoids
	 * O(N^2) behavior in situations where a session touches many foreign keys
	 * and also does many ALTER TABLEs, such as a restore from pg_dump.
	 */
	if (dclist_count(&ri_constraint_cache_valid_list) > 1000)
		hashvalue = 0;			/* pretend it's a cache reset */

	dclist_foreach_modify(iter, &ri_constraint_cache_valid_list)
	{
		RI_ConstraintInfo *riinfo = dclist_container(RI_ConstraintInfo,
													 valid_link, iter.cur);

		/*
		 * We must invalidate not only entries directly matching the given
		 * hash value, but also child entries, in case the invalidation
		 * affects a root constraint.
		 */
		if (hashvalue == 0 ||
			riinfo->oidHashValue == hashvalue ||
			riinfo->rootHashValue == hashvalue)
		{
			riinfo->valid = false;

			/*
			 * Detach any fast-path metadata so that the next check
			 * repopulates it, but do not free it here.  ri_FastPathCheck()
			 * and the flush routines copy riinfo->fpmeta into a local (and
			 * take FmgrInfo pointers into it) and then run index scans, tuple
			 * locking, and user-supplied cast and equality functions, all of
			 * which can accept invalidation messages and reach this callback.
			 * Freeing now would leave those callers reading freed memory.
			 * Queue it instead; AtEOXact_RI() releases it once no RI check
			 * can be running.
			 */
			if (riinfo->fpmeta)
			{
				riinfo->fpmeta->next_dead = ri_fpmeta_dead_list;
				ri_fpmeta_dead_list = riinfo->fpmeta;
				riinfo->fpmeta = NULL;
			}

			/* Remove invalidated entries from the list, too */
			dclist_delete_from(&ri_constraint_cache_valid_list, iter.cur);
		}
	}
}
/* …（同文件无关代码省略）… */
static bool
ri_FastPathCheck(RI_ConstraintInfo *riinfo,
				 Relation fk_rel, TupleTableSlot *newslot)
{
	Relation	pk_rel;
	Relation	idx_rel;
	IndexScanDesc scandesc;
	TupleTableSlot *slot;
	Datum		pk_vals[INDEX_MAX_KEYS];
	char		pk_nulls[INDEX_MAX_KEYS];
	ScanKeyData skey[INDEX_MAX_KEYS];
	bool		found = false;
	Oid			saved_userid;
	int			saved_sec_context;
	Snapshot	snapshot;

	INJECTION_POINT("ri-before-pk-lock", NULL);

	pk_rel = table_open(riinfo->pk_relid, RowShareLock);

	/*
	 * Advance the command counter so the check sees the effects of prior
	 * triggers in this statement, as SPI does when executing the query issued
	 * by ri_PerformCheck().  Do this after locking the referenced relation
	 * and before reloading the constraint information, so local invalidations
	 * are processed under the lock.
	 */
	CommandCounterIncrement();

	/* Re-read the constraint under that lock; see ri_FastPathGetEntry(). */
	riinfo = ri_LoadConstraintInfo(riinfo->constraint_id);

	idx_rel = index_open(riinfo->conindid, AccessShareLock);

	if (!ri_check_fastpath_index(riinfo, pk_rel, idx_rel))
	{
		index_close(idx_rel, NoLock);
		table_close(pk_rel, NoLock);
		return false;
	}

	/*
	 * Only now take the snapshot the scan will use.  Acquiring it before
	 * table_open() would let an unbounded amount of time pass while we wait
	 * for the lock, during which another transaction can commit the very row
	 * we are about to look for.  The scan would not see it and the check
	 * would report a violation for a key that exists.
	 *
	 * The SPI path does not have this problem: for this check it passes
	 * InvalidSnapshot, so SPI takes the snapshot after the
	 * referenced-relation lock has been acquired.
	 *
	 * Make this snapshot active too, as SPI does.  STABLE cast and equality
	 * functions use the active snapshot, so leaving the outer query's
	 * snapshot active could hide changes made by earlier triggers even though
	 * the index scan can see them.
	 */
	snapshot = RegisterSnapshot(GetTransactionSnapshot());
	PushActiveSnapshot(snapshot);

	slot = table_slot_create(pk_rel, NULL);

	GetUserIdAndSecContext(&saved_userid, &saved_sec_context);
	SetUserIdAndSecContext(RelationGetForm(pk_rel)->relowner,
						   saved_sec_context |
						   SECURITY_LOCAL_USERID_CHANGE |
						   SECURITY_NOFORCE_RLS);
	ri_CheckPermissions(riinfo, pk_rel);

	/*
	 * Begin the scan under the switched user id, so that any access method
	 * code invoked by index_beginscan() runs as the PK relation's owner.  For
	 * btree this has no functional consequence, but it keeps the ordering
	 * correct for out-of-tree access methods.
	 */
	scandesc = index_beginscan(pk_rel, idx_rel, false,
							   snapshot, NULL,
							   riinfo->nkeys, 0,
							   SO_NONE);

	if (riinfo->fpmeta == NULL)
	{
		/* Reload to ensure it's valid. */
		riinfo = ri_LoadConstraintInfo(riinfo->constraint_id);
		ri_populate_fastpath_metadata(riinfo, fk_rel, idx_rel);
	}
	Assert(riinfo->fpmeta);
	ri_CheckFunctionPermissions(riinfo, riinfo->fpmeta);
	ri_ExtractValues(fk_rel, newslot, riinfo, false, pk_vals, pk_nulls);
	build_index_scankeys(riinfo, riinfo->fpmeta, idx_rel, pk_vals, pk_nulls,
						 skey);
	found = ri_FastPathProbeOne(pk_rel, idx_rel, scandesc, slot,
								snapshot, riinfo, skey, riinfo->nkeys);
	SetUserIdAndSecContext(saved_userid, saved_sec_context);
	index_endscan(scandesc);
	ExecDropSingleTupleTableSlot(slot);
	UnregisterSnapshot(snapshot);
	PopActiveSnapshot();

	if (!found)
		ri_ReportViolation(riinfo, pk_rel, fk_rel,
						   newslot, NULL,
						   RI_PLAN_CHECK_LOOKUPPK, false, false);

	index_close(idx_rel, NoLock);
	table_close(pk_rel, NoLock);
	return true;
}

/*
 * ri_FastPathBatchAdd
 *		Buffer a FK row for batched probing.
 *
 * Adds the row to the batch buffer.  When the buffer is full, flushes all
 * buffered rows by probing the PK index.  Any violation is reported
 * immediately during the flush via ri_ReportViolation (which does not return).
 *
 * Uses the per-batch cache (RI_FastPathEntry) to avoid per-row relation
 * open/close, slot creation, etc.
 *
 * The batch is also flushed at end of trigger-firing cycle via
 * ri_FastPathEndBatch().
 *
 * Return false if the index is unsuitable, without buffering the row, so the
 * caller can use SPI instead.
 */
static bool
ri_FastPathBatchAdd(RI_ConstraintInfo *riinfo,
					Relation fk_rel, TupleTableSlot *newslot)
{
	RI_FastPathEntry *fpentry = ri_FastPathGetEntry(riinfo, fk_rel);

	if (fpentry == NULL)
		return false;

	/*
	 * If this entry is already being flushed, a cast function or an operator
	 * invoked during the flush has re-entered with DML on the same FK.  Fall
	 * back to the per-row path rather than touching the batch array, which is
	 * mid-flush.
	 */
	if (unlikely(fpentry->flushing))
		return ri_FastPathCheck(riinfo, fk_rel, newslot);

	/*
	 * A batch is filled and flushed within a single trigger-firing cycle, so
	 * every row added to an entry comes from the subtransaction that created
	 * it.  AtEOSubXact_RI() relies on this to identify an aborting
	 * subtransaction's entries by the subid stamped at entry creation.
	 */
	Assert(fpentry->subid == GetCurrentSubTransactionId());

	/*
	 * Buffer the row.  A full batch is flushed below and re-entry is handled
	 * above, so there is always room here; the bounds check just guards the
	 * array write.
	 */
	if (fpentry->batch_count < RI_FASTPATH_BATCH_SIZE)
	{
		MemoryContext oldcxt = MemoryContextSwitchTo(fpentry->flush_cxt);

		fpentry->batch[fpentry->batch_count] =
			ExecCopySlotHeapTuple(newslot);
		fpentry->batch_count++;
		MemoryContextSwitchTo(oldcxt);
	}
	else
		elog(ERROR, "RI fast-path batch unexpectedly full");

	/* Flush as soon as the batch is full. */
	if (fpentry->batch_count == RI_FASTPATH_BATCH_SIZE)
		ri_FastPathBatchFlush(fpentry, fk_rel, riinfo);
	return true;
}

/*
 * ri_FastPathBatchFlush
 *		Flush all buffered FK rows by probing the PK index.
 *
 * Dispatches to ri_FastPathFlushArray() for single-column FKs
 * (using SK_SEARCHARRAY) or ri_FastPathFlushLoop() for multi-column
 * FKs (per-row probing).  Violations are reported immediately via
 * ri_ReportViolation(), which does not return.
 */
static void
ri_FastPathBatchFlush(RI_FastPathEntry *fpentry, Relation fk_rel,
					  RI_ConstraintInfo *riinfo)
{
	Relation	pk_rel = fpentry->pk_rel;
	Relation	idx_rel = fpentry->idx_rel;
	TupleTableSlot *fk_slot = fpentry->fk_slot;
	Snapshot	snapshot;
	IndexScanDesc scandesc;
	Oid			saved_userid;
	int			saved_sec_context;
	MemoryContext oldcxt;
	FastPathMeta *fpmeta;
	int			violation_index;

	if (fpentry->batch_count == 0)
		return;

	/*
	 * CCI and security context switch are done once for the entire batch.
	 * Per-row CCI is unnecessary because by the time a flush runs, all AFTER
	 * triggers for the buffered rows have already fired (trigger invocations
	 * strictly alternate per row), so a single CCI advances past all their
	 * effects.  Per-row security context switch is unnecessary because each
	 * row's probe runs entirely as the PK table owner, same as the SPI path
	 * -- the only difference is that the SPI path sets and restores the
	 * context per row whereas we do it once around the whole batch.
	 */
	CommandCounterIncrement();
	snapshot = RegisterSnapshot(GetTransactionSnapshot());

	/*
	 * build_index_scankeys() may palloc cast results for cross-type FKs. Use
	 * the entry's short-lived flush context so these don't accumulate across
	 * batches.
	 */
	oldcxt = MemoryContextSwitchTo(fpentry->flush_cxt);

	GetUserIdAndSecContext(&saved_userid, &saved_sec_context);
	SetUserIdAndSecContext(RelationGetForm(pk_rel)->relowner,
						   saved_sec_context |
						   SECURITY_LOCAL_USERID_CHANGE |
						   SECURITY_NOFORCE_RLS);

	/*
	 * Check that the current user has permission to access pk_rel. Done here
	 * rather than at entry creation so that permission changes between
	 * flushes are respected, matching the per-row behavior of the SPI path,
	 * albeit checked once per flush rather than once per row, like in
	 * ri_FastPathCheck().
	 */
	ri_CheckPermissions(riinfo, pk_rel);

	/*
	 * Begin the scan under the switched user id, so that any access method
	 * code invoked by index_beginscan() runs as the PK relation's owner.  For
	 * btree this has no functional consequence, but it keeps the ordering
	 * correct for out-of-tree access methods.
	 */
	scandesc = index_beginscan(pk_rel, idx_rel, false, snapshot, NULL,
							   riinfo->nkeys, 0, SO_NONE);

	if (riinfo->fpmeta == NULL)
	{
		/* Reload to ensure it's valid. */
		riinfo = ri_LoadConstraintInfo(riinfo->constraint_id);
		ri_populate_fastpath_metadata(riinfo, fk_rel, idx_rel);
	}
	Assert(riinfo->fpmeta);

	/*
	 * Take our own reference to the metadata for the duration of the flush.
	 * The probe below runs user-defined cast and equality functions, which
	 * can accept invalidation messages; InvalidateConstraintCacheCallBack()
	 * then clears riinfo->fpmeta, so re-reading it partway through the batch
	 * would find NULL.  The object itself stays valid until AtEOXact_RI().
	 */
	fpmeta = riinfo->fpmeta;

	/*
	 * The probe runs user-defined cast and equality functions.  Set the
	 * flushing flag around it so a re-entrant ri_FastPathBatchAdd on this
	 * entry takes the per-row path, and clear it even on error so the entry
	 * is reusable if the error is caught by a savepoint.
	 */
	Assert(!fpentry->flushing);
	fpentry->flushing = true;
	PG_TRY();
	{
		/* Skip array overhead for single-row batches. */
		if (riinfo->nkeys == 1 && fpentry->batch_count > 1)
			violation_index = ri_FastPathFlushArray(fpentry, fk_slot, riinfo,
													fpmeta, fk_rel, snapshot,
													scandesc);
		else
			violation_index = ri_FastPathFlushLoop(fpentry, fk_slot, riinfo,
												   fpmeta, fk_rel, snapshot,
												   scandesc);
	}
	PG_FINALLY();
	{
		fpentry->flushing = false;
		fpentry->batch_count = 0;
	}
	PG_END_TRY();

	SetUserIdAndSecContext(saved_userid, saved_sec_context);
	UnregisterSnapshot(snapshot);
	index_endscan(scandesc);

	if (violation_index >= 0)
	{
		ExecStoreHeapTuple(fpentry->batch[violation_index], fk_slot, false);
		ri_ReportViolation(riinfo, pk_rel, fk_rel,
						   fk_slot, NULL,
						   RI_PLAN_CHECK_LOOKUPPK, false, false);
	}

	MemoryContextReset(fpentry->flush_cxt);
	MemoryContextSwitchTo(oldcxt);
}

/*
 * ri_FastPathFlushLoop
 *		Multi-column fallback: probe the index once per buffered row.
 *
 * Used for composite foreign keys where SK_SEARCHARRAY does not
 * apply, and also for single-row batches of single-column FKs where
 * the array overhead is not worth it.
 *
 * Returns the index of the first violating row in the batch array, or -1 if
 * all rows are valid.
 */
static int
ri_FastPathFlushLoop(RI_FastPathEntry *fpentry, TupleTableSlot *fk_slot,
					 const RI_ConstraintInfo *riinfo, FastPathMeta *fpmeta,
					 Relation fk_rel, Snapshot snapshot,
					 IndexScanDesc scandesc)
{
	Relation	pk_rel = fpentry->pk_rel;
	Relation	idx_rel = fpentry->idx_rel;
	TupleTableSlot *pk_slot = fpentry->pk_slot;
	Datum		pk_vals[INDEX_MAX_KEYS];
	char		pk_nulls[INDEX_MAX_KEYS];
	ScanKeyData skey[INDEX_MAX_KEYS];
	bool		found = true;

	for (int i = 0; i < fpentry->batch_count; i++)
	{
		ExecStoreHeapTuple(fpentry->batch[i], fk_slot, false);
		ri_ExtractValues(fk_rel, fk_slot, riinfo, false, pk_vals, pk_nulls);
		build_index_scankeys(riinfo, fpmeta, idx_rel, pk_vals, pk_nulls, skey);

		found = ri_FastPathProbeOne(pk_rel, idx_rel, scandesc, pk_slot,
									snapshot, riinfo, skey, riinfo->nkeys);

		/* Report first unmatched row */
		if (!found)
			return i;
	}

	/* All pass. */
	return -1;
}

/*
 * ri_FastPathFlushArray
 *		Single-column fast path using SK_SEARCHARRAY.
 *
 * Builds an array of FK values and does one index scan with
 * SK_SEARCHARRAY.  The index AM sorts and deduplicates the array
 * internally, then walks matching leaf pages in order.  Each
 * matched PK tuple is locked and rechecked as before; a matched[]
 * bitmap tracks which batch items were satisfied.
 *
 * Returns the index of the first violating row in the batch array, or -1 if
 * all rows are valid.
 */
static int
ri_FastPathFlushArray(RI_FastPathEntry *fpentry, TupleTableSlot *fk_slot,
					  const RI_ConstraintInfo *riinfo, FastPathMeta *fpmeta,
					  Relation fk_rel, Snapshot snapshot,
					  IndexScanDesc scandesc)
{
	Relation	pk_rel = fpentry->pk_rel;
	Relation	idx_rel = fpentry->idx_rel;
	TupleTableSlot *pk_slot = fpentry->pk_slot;
	Datum		search_vals[RI_FASTPATH_BATCH_SIZE];
	bool		matched[RI_FASTPATH_BATCH_SIZE];
	int			nvals = fpentry->batch_count;
	Datum		pk_vals[INDEX_MAX_KEYS];
	char		pk_nulls[INDEX_MAX_KEYS];
	ScanKeyData skey[1];
	FmgrInfo   *cast_func_finfo;
	FmgrInfo   *eq_opr_finfo;
	Oid			elem_type;
	int16		elem_len;
	bool		elem_byval;
	char		elem_align;
	ArrayType  *arr;

	Assert(fpmeta);

	memset(matched, 0, nvals * sizeof(bool));

	/*
	 * Extract FK values, casting to the operator's expected input type if
	 * needed (e.g. int8 FK -> int4 for int48eq).
	 */
	cast_func_finfo = &fpmeta->cast_func_finfo[0];
	eq_opr_finfo = &fpmeta->eq_opr_finfo[0];
	for (int i = 0; i < nvals; i++)
	{
		ExecStoreHeapTuple(fpentry->batch[i], fk_slot, false);
		ri_ExtractValues(fk_rel, fk_slot, riinfo, false, pk_vals, pk_nulls);

		/* Cast if needed (e.g. int8 FK -> numeric PK) */
		if (OidIsValid(cast_func_finfo->fn_oid))
			search_vals[i] = FunctionCall3(cast_func_finfo,
										   pk_vals[0],
										   Int32GetDatum(-1),
										   BoolGetDatum(false));
		else
			search_vals[i] = pk_vals[0];
	}

	/*
	 * Array element type must match the operator's right-hand input type,
	 * which is what the index comparison expects on the search side.
	 * ri_populate_fastpath_metadata() stores exactly this via
	 * get_op_opfamily_properties(), which returns the operator's right-hand
	 * type as the subtype for cross-type operators (e.g. int8 for int48eq)
	 * and the common type for same-type operators.
	 */
	elem_type = fpmeta->subtypes[0];
	Assert(OidIsValid(elem_type));
	get_typlenbyvalalign(elem_type, &elem_len, &elem_byval, &elem_align);

	arr = construct_array(search_vals, nvals,
						  elem_type, elem_len, elem_byval, elem_align);

	/*
	 * Build scan key with SK_SEARCHARRAY.  The index AM code will internally
	 * sort and deduplicate, then walk leaf pages in order.
	 *
	 * ri_check_fastpath_index() restricts the fast path to btree indexes,
	 * which support SK_SEARCHARRAY.
	 *
	 * This path handles single-column FKs only, so index_attnos[0] == 1.
	 */
	Assert(idx_rel->rd_indam->amsearcharray);
	Assert(fpmeta->index_attnos[0] == 1);
	ScanKeyEntryInitialize(&skey[0],
						   SK_SEARCHARRAY,
						   fpmeta->index_attnos[0],
						   fpmeta->strats[0],
						   fpmeta->subtypes[0],
						   idx_rel->rd_indcollation[fpmeta->index_attnos[0] - 1],
						   fpmeta->regops[0],
						   PointerGetDatum(arr));

	index_rescan(scandesc, skey, 1, NULL, 0);

	/*
	 * Walk all matches.  The index AM returns them in index order.  For each
	 * match, find which batch item(s) it satisfies.
	 */
	while (table_index_getnext_slot(scandesc, ForwardScanDirection, pk_slot))
	{
		Datum		found_val;
		bool		found_null;

		/*
		 * No key recheck is needed here, so we have no use for
		 * concurrently_updated.  Unlike ri_FastPathProbeOne(), which takes
		 * the index scan's word for it that the tuple matches, this path
		 * compares the key against every buffered FK value below, and it does
		 * so using found_val, which is read out of the version we actually
		 * locked.  A concurrent key update is therefore caught by that
		 * comparison: the batch item that led us to this tuple is left
		 * unmatched and reported as a violation.
		 */
		if (!ri_LockPKTuple(pk_rel, pk_slot, snapshot, NULL))
			continue;

		/*
		 * Extract the PK value from the matched and locked tuple.
		 *
		 * A foreign key may reference a nullable unique column, not just a
		 * NOT NULL primary key.  If ri_LockPKTuple() chased an update chain
		 * to a version whose referenced key is now NULL, that version cannot
		 * equal any buffered (non-null) FK value, so skip it.  This mirrors
		 * the SPI path, where the requalifying "pkatt = $n" yields NULL and
		 * the row is not returned.
		 */
		found_val = slot_getattr(pk_slot, riinfo->pk_attnums[0], &found_null);
		if (found_null)
			continue;

		/*
		 * Linear scan to mark all batch items matching this PK value.
		 * O(batch_size) per match, O(batch_size^2) worst case -- fine for the
		 * current batch size of 64.
		 */
		for (int i = 0; i < nvals; i++)
		{
			if (!matched[i] &&
				DatumGetBool(FunctionCall2Coll(eq_opr_finfo,
											   idx_rel->rd_indcollation[0],
											   found_val,
											   search_vals[i])))
				matched[i] = true;
		}
	}

	/* Report first unmatched row */
	for (int i = 0; i < nvals; i++)
		if (!matched[i])
			return i;

	/* All pass. */
	return -1;
}

/*
 * ri_FastPathProbeOne
 *		Probe the PK index for one set of scan keys, lock the matching
/* …（同文件无关代码省略）… */
static bool
ri_FastPathProbeOne(Relation pk_rel, Relation idx_rel,
					IndexScanDesc scandesc, TupleTableSlot *slot,
					Snapshot snapshot, const RI_ConstraintInfo *riinfo,
					ScanKeyData *skey, int nkeys)
{
	bool		found = false;

	index_rescan(scandesc, skey, nkeys, NULL, 0);

	if (table_index_getnext_slot(scandesc, ForwardScanDirection, slot))
	{
		bool		concurrently_updated;

		if (ri_LockPKTuple(pk_rel, slot, snapshot,
						   &concurrently_updated))
		{
			if (concurrently_updated)
				found = recheck_matched_pk_tuple(idx_rel, skey, nkeys, slot);
			else
				found = true;
		}
	}

	return found;
}
/* …（同文件无关代码省略）… */
 * Calls table_tuple_lock() directly with handling specific to RI checks.
 * Returns true if the tuple was successfully locked.
 *
 * If concurrently_updated is not NULL, sets *concurrently_updated to true
 * if the locked tuple was reached by following an update chain
 * (tmfd.traversed), indicating the caller should recheck the key.  Callers
 * that compare the locked tuple's key against the value they were looking
 * for anyway can pass NULL.
 */
static bool
ri_LockPKTuple(Relation pk_rel, TupleTableSlot *slot, Snapshot snap,
			   bool *concurrently_updated)
{
	TM_FailureData tmfd;
	TM_Result	result;
	int			lockflags = TUPLE_LOCK_FLAG_LOCK_UPDATE_IN_PROGRESS;

	if (concurrently_updated)
		*concurrently_updated = false;

	if (!IsolationUsesXactSnapshot())
		lockflags |= TUPLE_LOCK_FLAG_FIND_LAST_VERSION;

	result = table_tuple_lock(pk_rel, &slot->tts_tid, snap,
							  slot, GetCurrentCommandId(false),
							  LockTupleKeyShare, LockWaitBlock,
							  lockflags, &tmfd);

	switch (result)
	{
		case TM_Ok:
			if (tmfd.traversed && concurrently_updated)
				*concurrently_updated = true;
			return true;

		case TM_Deleted:
			if (IsolationUsesXactSnapshot())
				ereport(ERROR,
						(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
						 errmsg("could not serialize access due to concurrent delete")));
			return false;

		case TM_Updated:
			if (IsolationUsesXactSnapshot())
				ereport(ERROR,
						(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
						 errmsg("could not serialize access due to concurrent update")));

			/*
			 * In READ COMMITTED, FIND_LAST_VERSION should have chased the
			 * chain rather than returning TM_Updated.  As in ExecLockRows(),
			 * treat this as an unexpected result.
			 */
			elog(ERROR, "unexpected table_tuple_lock status: %u", result);
			break;

		case TM_SelfModified:

			/*
			 * As in ExecLockRows(), ignore a tuple updated or deleted by the
			 * current command or a later command in this transaction.
			 */
			return false;

		case TM_Invisible:
			elog(ERROR, "attempted to lock invisible tuple");
			break;

		default:
			elog(ERROR, "unrecognized table_tuple_lock status: %u", result);
			break;
	}

	return false;				/* keep compiler quiet */
}
/* …（同文件无关代码省略）… */
static bool
ri_fastpath_is_applicable(const RI_ConstraintInfo *riinfo)
{
	/*
	 * Partitioned referenced tables are skipped for simplicity, since they
	 * require routing the probe through the correct partition using
	 * PartitionDirectory.
	 */
	if (riinfo->pk_is_partitioned)
		return false;

	/*
	 * Temporal foreign keys use range overlap and containment semantics (&&,
	 * <@, range_agg()) that inherently involve aggregation and multiple-row
	 * reasoning, so they stay on the SPI path.
	 */
	if (riinfo->hasperiod)
		return false;

	return riinfo->fastpath_state != RI_FASTPATH_UNUSABLE;
}
/* …（同文件无关代码省略）… */
static bool
ri_check_fastpath_index(RI_ConstraintInfo *riinfo,
						Relation pk_rel, Relation idx_rel)
{
	/* Opening the index can have processed further invalidations. */
	if (!riinfo->valid)
		riinfo = ri_LoadConstraintInfo(riinfo->constraint_id);

	if (riinfo->fastpath_state != RI_FASTPATH_UNKNOWN)
		return riinfo->fastpath_state == RI_FASTPATH_USABLE;

	/*
	 * Unique indexes provided by other access methods can support FKs, but
	 * the direct probe and SK_SEARCHARRAY implementation assume btree.
	 */
	if (idx_rel->rd_rel->relam != BTREE_AM_OID)
	{
		riinfo->fastpath_state = RI_FASTPATH_UNUSABLE;
		return false;
	}

	/*
	 * Leave comparisons with a different index and referenced-column
	 * collation to SPI.  Map index keys to table attributes because the FK
	 * columns need not be listed in index order.  Ignore INCLUDE columns.
	 */
	for (int i = 0; i < idx_rel->rd_index->indnkeyatts; i++)
	{
		AttrNumber	attnum = idx_rel->rd_index->indkey.values[i];

		if (idx_rel->rd_indcollation[i] != RIAttCollation(pk_rel, attnum))
		{
			riinfo->fastpath_state = RI_FASTPATH_UNUSABLE;
			return false;
		}
	}

	/*
	 * The equality operator stored in pg_constraint must still be an equality
	 * member of the index opfamily.  When it is not, the direct fast-path
	 * probe errors, so mark the fast path unusable and fall back to SPI,
	 * which uses the same operator in a query where the planner simply
	 * declines the index.
	 */
	for (int i = 0; i < riinfo->nkeys; i++)
	{
		int			idx_col;

		for (idx_col = 0; idx_col < idx_rel->rd_index->indnkeyatts; idx_col++)
		{
			if (idx_rel->rd_index->indkey.values[idx_col] ==
				riinfo->pk_attnums[i])
				break;
		}
		Assert(idx_col < idx_rel->rd_index->indnkeyatts);

		if (get_op_opfamily_strategy(riinfo->pf_eq_oprs[i],
									 idx_rel->rd_opfamily[idx_col]) != BTEqualStrategyNumber)
		{
			riinfo->fastpath_state = RI_FASTPATH_UNUSABLE;
			return false;
		}
	}

	riinfo->fastpath_state = RI_FASTPATH_USABLE;
	return true;
}
/* …（同文件无关代码省略）… */
static void
ri_CheckPermissions(const RI_ConstraintInfo *riinfo, Relation query_rel)
{
	AclResult	aclresult;
	AclMode		requiredPerms = ACL_SELECT | ACL_SELECT_FOR_UPDATE;
	RangeTblEntry *rte;
	RTEPermissionInfo *perminfo;

	/* USAGE on schema. */
	aclresult = object_aclcheck(NamespaceRelationId,
								RelationGetNamespace(query_rel),
								GetUserId(), ACL_USAGE);
	if (aclresult != ACLCHECK_OK)
		aclcheck_error(aclresult, OBJECT_SCHEMA,
					   get_namespace_name(RelationGetNamespace(query_rel)));

	/*
	 * SELECT is needed only on the referenced key columns.  FOR KEY SHARE
	 * also needs UPDATE privilege, which may be granted on any column; leave
	 * updatedCols empty as the SPI query does.
	 */
	perminfo = makeNode(RTEPermissionInfo);
	perminfo->relid = RelationGetRelid(query_rel);
	perminfo->requiredPerms = requiredPerms;
	for (int i = 0; i < riinfo->nkeys; i++)
	{
		int			attno = riinfo->pk_attnums[i] - FirstLowInvalidHeapAttributeNumber;

		perminfo->selectedCols = bms_add_member(perminfo->selectedCols, attno);
	}

	rte = makeNode(RangeTblEntry);
	rte->rtekind = RTE_RELATION;
	rte->relid = RelationGetRelid(query_rel);
	rte->relkind = query_rel->rd_rel->relkind;
	rte->rellockmode = RowShareLock;
	rte->perminfoindex = 1;

	(void) ExecCheckPermissions(list_make1(rte), list_make1(perminfo), true);
}
/* …（同文件无关代码省略）… */
static bool
recheck_matched_pk_tuple(Relation idxrel, ScanKeyData *skeys, int nkeys,
						 TupleTableSlot *new_slot)
{
	/*
	 * TODO: BuildIndexInfo does a syscache lookup + palloc on every call.
	 * This only fires on the concurrent-update path (tmfd.traversed), which
	 * should be rare, so the cost is acceptable for now.  If profiling shows
	 * otherwise, cache the IndexInfo in FastPathMeta.
	 */
	IndexInfo  *indexInfo = BuildIndexInfo(idxrel);
	Datum		values[INDEX_MAX_KEYS];
	bool		isnull[INDEX_MAX_KEYS];
	bool		matched = true;

	/* PK indexes never have these. */
	Assert(indexInfo->ii_Expressions == NIL &&
		   indexInfo->ii_ExclusionOps == NULL);

	/* Form the index values and isnull flags given the table tuple. */
	Assert(nkeys == indexInfo->ii_NumIndexKeyAttrs);
	FormIndexDatum(indexInfo, new_slot, NULL, values, isnull);
	for (int i = 0; i < nkeys; i++)
	{
		ScanKeyData *skey = &skeys[i];

		/*
		 * A foreign key may reference a nullable unique column, so the
		 * version we chased the update chain to may have a NULL in a key
		 * column.  A NULL never equals the value we searched for, so treat it
		 * as no match, as the SPI path's requalification would.
		 */
		if (isnull[i] ||
			!DatumGetBool(FunctionCall2Coll(&skey->sk_func,
											skey->sk_collation,
											values[i],
											skey->sk_argument)))
		{
			matched = false;
			break;
		}
	}

	return matched;
}
/* …（同文件无关代码省略）… */
static void
ri_CheckFunctionPermissions(const RI_ConstraintInfo *riinfo,
							const FastPathMeta *fpmeta)
{
	for (int i = 0; i < riinfo->nkeys; i++)
	{
		Oid			funcs[2] = {fpmeta->regops[i], fpmeta->cast_func_finfo[i].fn_oid};

		for (int j = 0; j < lengthof(funcs); j++)
		{
			AclResult	aclresult;

			if (!OidIsValid(funcs[j]))
				continue;
			aclresult = object_aclcheck(ProcedureRelationId, funcs[j],
										GetUserId(), ACL_EXECUTE);
			if (aclresult != ACLCHECK_OK)
				aclcheck_error(aclresult, OBJECT_FUNCTION,
							   get_func_name(funcs[j]));
			InvokeFunctionExecuteHook(funcs[j]);
		}
	}
}
/* …（同文件无关代码省略）… */
static void
build_index_scankeys(const RI_ConstraintInfo *riinfo,
					 FastPathMeta *fpmeta,
					 Relation idx_rel, Datum *pk_vals,
					 char *pk_nulls, ScanKey skeys)
{
	Assert(fpmeta);

	/*
	 * May need to cast each of the individual values of the foreign key to
	 * the corresponding PK column's type if the equality operator demands it.
	 */
	for (int i = 0; i < riinfo->nkeys; i++)
	{
		if (pk_nulls[i] != 'n' &&
			OidIsValid(fpmeta->cast_func_finfo[i].fn_oid))
			pk_vals[i] = FunctionCall3(&fpmeta->cast_func_finfo[i],
									   pk_vals[i],
									   Int32GetDatum(-1),	/* typmod */
									   BoolGetDatum(false));	/* implicit coercion */
	}

	/*
	 * Set up ScanKeys for the index scan. This is essentially how
	 * ExecIndexBuildScanKeys() sets them up.  Use the cached index_attnos and
	 * the corresponding collation since FK columns may be in a different
	 * order than PK index columns.  Place each scan key at the array position
	 * corresponding to its index column, since btree requires keys to be
	 * ordered by attribute number.
	 */
	for (int i = 0; i < riinfo->nkeys; i++)
	{
		AttrNumber	pkattrno = fpmeta->index_attnos[i];
		int			skey_pos = pkattrno - 1;	/* 0-based array position */

		ScanKeyEntryInitialize(&skeys[skey_pos], 0, pkattrno,
							   fpmeta->strats[i], fpmeta->subtypes[i],
							   idx_rel->rd_indcollation[skey_pos], fpmeta->regops[i],
							   pk_vals[i]);
	}
}
/* …（同文件无关代码省略）… */
static void
ri_populate_fastpath_metadata(RI_ConstraintInfo *riinfo,
							  Relation fk_rel, Relation idx_rel)
{
	FastPathMeta *fpmeta;
	MemoryContext oldcxt = MemoryContextSwitchTo(TopMemoryContext);

	Assert(riinfo != NULL && riinfo->valid);
	Assert(riinfo->fpmeta == NULL);

	fpmeta = palloc_object(FastPathMeta);
	fpmeta->next_dead = NULL;

	/* Scratch context for the cached FmgrInfos' fn_mcxt; see FastPathMeta. */
	fpmeta->scratch_cxt = AllocSetContextCreate(TopMemoryContext,
												"RI fast-path finfo scratch",
												ALLOCSET_SMALL_SIZES);
	for (int i = 0; i < riinfo->nkeys; i++)
	{
		Oid			eq_opr = riinfo->pf_eq_oprs[i];
		Oid			typeid = RIAttType(fk_rel, riinfo->fk_attnums[i]);
		Oid			lefttype;
		RI_CompareHashEntry *entry = ri_HashCompareOp(eq_opr, typeid);
		int			idx_col;

		/*
		 * Find the index column position for this constraint key.  The FK
		 * constraint may reference columns in a different order than they
		 * appear in the PK index, so we must map pk_attnums[i] to the
		 * corresponding index column position.
		 */
		for (idx_col = 0; idx_col < riinfo->nkeys; idx_col++)
		{
			if (idx_rel->rd_index->indkey.values[idx_col] == riinfo->pk_attnums[i])
				break;
		}
		Assert(idx_col < riinfo->nkeys);

		/* 1-based attribute number */
		fpmeta->index_attnos[i] = idx_col + 1;

		fmgr_info_copy(&fpmeta->cast_func_finfo[i], &entry->cast_func_finfo,
					   fpmeta->scratch_cxt);
		fmgr_info_copy(&fpmeta->eq_opr_finfo[i], &entry->eq_opr_finfo,
					   fpmeta->scratch_cxt);
		fpmeta->regops[i] = get_opcode(eq_opr);

		get_op_opfamily_properties(eq_opr,
								   idx_rel->rd_opfamily[idx_col],
								   false,
								   &fpmeta->strats[i],
								   &lefttype,
								   &fpmeta->subtypes[i]);
	}

	riinfo->fpmeta = fpmeta;
	MemoryContextSwitchTo(oldcxt);
}
/* …（同文件无关代码省略）… */
static void
ri_ExtractValues(Relation rel, TupleTableSlot *slot,
				 const RI_ConstraintInfo *riinfo, bool rel_is_pk,
				 Datum *vals, char *nulls)
{
	const int16 *attnums;
	bool		isnull;

	if (rel_is_pk)
		attnums = riinfo->pk_attnums;
	else
		attnums = riinfo->fk_attnums;

	for (int i = 0; i < riinfo->nkeys; i++)
	{
		vals[i] = slot_getattr(slot, attnums[i], &isnull);
		nulls[i] = isnull ? 'n' : ' ';
	}
}
/* …（同文件无关代码省略）… */
static void
ri_ReportViolation(const RI_ConstraintInfo *riinfo,
				   Relation pk_rel, Relation fk_rel,
				   TupleTableSlot *violatorslot, TupleDesc tupdesc,
				   int queryno, bool is_restrict, bool partgone)
{
	StringInfoData key_names;
	StringInfoData key_values;
	bool		onfk;
	const int16 *attnums;
	Oid			rel_oid;
	AclResult	aclresult;
	bool		has_perm = true;

	/*
	 * Determine which relation to complain about.  If tupdesc wasn't passed
	 * by caller, assume the violator tuple came from there.
	 */
	onfk = (queryno == RI_PLAN_CHECK_LOOKUPPK);
	if (onfk)
	{
		attnums = riinfo->fk_attnums;
		rel_oid = fk_rel->rd_id;
		if (tupdesc == NULL)
			tupdesc = fk_rel->rd_att;
	}
	else
	{
		attnums = riinfo->pk_attnums;
		rel_oid = pk_rel->rd_id;
		if (tupdesc == NULL)
			tupdesc = pk_rel->rd_att;
	}

	/*
	 * Check permissions- if the user does not have access to view the data in
	 * any of the key columns then we don't include the errdetail() below.
	 *
	 * Check if RLS is enabled on the relation first.  If so, we don't return
	 * any specifics to avoid leaking data.
	 *
	 * Check table-level permissions next and, failing that, column-level
	 * privileges.
	 *
	 * When a partition at the referenced side is being detached/dropped, we
	 * needn't check, since the user must be the table owner anyway.
	 */
	if (partgone)
		has_perm = true;
	else if (check_enable_rls(rel_oid, InvalidOid, true) != RLS_ENABLED)
	{
		aclresult = pg_class_aclcheck(rel_oid, GetUserId(), ACL_SELECT);
		if (aclresult != ACLCHECK_OK)
		{
			/* Try for column-level permissions */
			for (int idx = 0; idx < riinfo->nkeys; idx++)
			{
				aclresult = pg_attribute_aclcheck(rel_oid, attnums[idx],
												  GetUserId(),
												  ACL_SELECT);

				/* No access to the key */
				if (aclresult != ACLCHECK_OK)
				{
					has_perm = false;
					break;
				}
			}
		}
	}
	else
		has_perm = false;

	if (has_perm)
	{
		/* Get printable versions of the keys involved */
		initStringInfo(&key_names);
		initStringInfo(&key_values);
		for (int idx = 0; idx < riinfo->nkeys; idx++)
		{
			int			fnum = attnums[idx];
			Form_pg_attribute att = TupleDescAttr(tupdesc, fnum - 1);
			char	   *name,
					   *val;
			Datum		datum;
			bool		isnull;

			name = NameStr(att->attname);

			datum = slot_getattr(violatorslot, fnum, &isnull);
			if (!isnull)
			{
				Oid			foutoid;
				bool		typisvarlena;

				getTypeOutputInfo(att->atttypid, &foutoid, &typisvarlena);
				val = OidOutputFunctionCall(foutoid, datum);
			}
			else
				val = "null";

			if (idx > 0)
			{
				appendStringInfoString(&key_names, ", ");
				appendStringInfoString(&key_values, ", ");
			}
			appendStringInfoString(&key_names, name);
			appendStringInfoString(&key_values, val);
		}
	}

	if (partgone)
		ereport(ERROR,
				(errcode(ERRCODE_FOREIGN_KEY_VIOLATION),
				 errmsg("removing partition \"%s\" violates foreign key constraint \"%s\"",
						RelationGetRelationName(pk_rel),
						NameStr(riinfo->conname)),
				 errdetail("Key (%s)=(%s) is still referenced from table \"%s\".",
						   key_names.data, key_values.data,
						   RelationGetRelationName(fk_rel)),
				 errtableconstraint(fk_rel, NameStr(riinfo->conname))));
	else if (onfk)
		ereport(ERROR,
				(errcode(ERRCODE_FOREIGN_KEY_VIOLATION),
				 errmsg("insert or update on table \"%s\" violates foreign key constraint \"%s\"",
						RelationGetRelationName(fk_rel),
						NameStr(riinfo->conname)),
				 has_perm ?
				 errdetail("Key (%s)=(%s) is not present in table \"%s\".",
						   key_names.data, key_values.data,
						   RelationGetRelationName(pk_rel)) :
				 errdetail("Key is not present in table \"%s\".",
						   RelationGetRelationName(pk_rel)),
				 errtableconstraint(fk_rel, NameStr(riinfo->conname))));
	else if (is_restrict)
		ereport(ERROR,
				(errcode(ERRCODE_RESTRICT_VIOLATION),
				 errmsg("update or delete on table \"%s\" violates RESTRICT setting of foreign key constraint \"%s\" on table \"%s\"",
						RelationGetRelationName(pk_rel),
						NameStr(riinfo->conname),
						RelationGetRelationName(fk_rel)),
				 has_perm ?
				 errdetail("Key (%s)=(%s) is referenced from table \"%s\".",
						   key_names.data, key_values.data,
						   RelationGetRelationName(fk_rel)) :
				 errdetail("Key is referenced from table \"%s\".",
						   RelationGetRelationName(fk_rel)),
				 errtableconstraint(fk_rel, NameStr(riinfo->conname))));
	else
		ereport(ERROR,
				(errcode(ERRCODE_FOREIGN_KEY_VIOLATION),
				 errmsg("update or delete on table \"%s\" violates foreign key constraint \"%s\" on table \"%s\"",
						RelationGetRelationName(pk_rel),
						NameStr(riinfo->conname),
						RelationGetRelationName(fk_rel)),
				 has_perm ?
				 errdetail("Key (%s)=(%s) is still referenced from table \"%s\".",
						   key_names.data, key_values.data,
						   RelationGetRelationName(fk_rel)) :
				 errdetail("Key is still referenced from table \"%s\".",
						   RelationGetRelationName(fk_rel)),
				 errtableconstraint(fk_rel, NameStr(riinfo->conname))));
}
/* …（同文件无关代码省略）… */
static void
ri_InitHashTables(void)
{
	HASHCTL		ctl;

	ctl.keysize = sizeof(Oid);
	ctl.entrysize = sizeof(RI_ConstraintInfo);
	ri_constraint_cache = hash_create("RI constraint cache",
									  RI_INIT_CONSTRAINTHASHSIZE,
									  &ctl, HASH_ELEM | HASH_BLOBS);

	/* Arrange to flush cache on pg_constraint or pg_amop changes */
	CacheRegisterSyscacheCallback(CONSTROID,
								  InvalidateConstraintCacheCallBack,
								  (Datum) 0);
	CacheRegisterSyscacheCallback(AMOPOPID,
								  InvalidateConstraintCacheCallBack,
								  (Datum) 0);

	ctl.keysize = sizeof(RI_QueryKey);
	ctl.entrysize = sizeof(RI_QueryHashEntry);
	ri_query_cache = hash_create("RI query cache",
								 RI_INIT_QUERYHASHSIZE,
								 &ctl, HASH_ELEM | HASH_BLOBS);

	ctl.keysize = sizeof(RI_CompareKey);
	ctl.entrysize = sizeof(RI_CompareHashEntry);
	ri_compare_cache = hash_create("RI compare cache",
								   RI_INIT_QUERYHASHSIZE,
								   &ctl, HASH_ELEM | HASH_BLOBS);
}
/* …（同文件无关代码省略）… */
static RI_CompareHashEntry *
ri_HashCompareOp(Oid eq_opr, Oid typeid)
{
	RI_CompareKey key;
	RI_CompareHashEntry *entry;
	bool		found;

	/*
	 * On the first call initialize the hashtable
	 */
	if (!ri_compare_cache)
		ri_InitHashTables();

	/*
	 * Find or create a hash entry.  Note we're assuming RI_CompareKey
	 * contains no struct padding.
	 */
	key.eq_opr = eq_opr;
	key.typeid = typeid;
	entry = (RI_CompareHashEntry *) hash_search(ri_compare_cache,
												&key,
												HASH_ENTER, &found);
	if (!found)
		entry->valid = false;

	/*
	 * If not already initialized, do so.  Since we'll keep this hash entry
	 * for the life of the backend, put any subsidiary info for the function
	 * cache structs into TopMemoryContext.
	 */
	if (!entry->valid)
	{
		Oid			lefttype,
					righttype,
					castfunc;
		CoercionPathType pathtype;

		/* We always need to know how to call the equality operator */
		fmgr_info_cxt(get_opcode(eq_opr), &entry->eq_opr_finfo,
					  TopMemoryContext);

		/*
		 * If we chose to use a cast from FK to PK type, we may have to apply
		 * the cast function to get to the operator's input type.
		 *
		 * XXX eventually it would be good to support array-coercion cases
		 * here and in ri_CompareWithCast().  At the moment there is no point
		 * because cases involving nonidentical array types will be rejected
		 * at constraint creation time.
		 *
		 * XXX perhaps also consider supporting CoerceViaIO?  No need at the
		 * moment since that will never be generated for implicit coercions.
		 */
		op_input_types(eq_opr, &lefttype, &righttype);

		/*
		 * pf_eq_oprs (used by the fast path) can be cross-type when the FK
		 * and PK columns differ in type, e.g. int48eq for int4 PK / int8 FK.
		 * If the FK column's type, or the base type of a domain over it,
		 * already matches what the operator expects as its right-hand input,
		 * no cast is needed.
		 */
		if (getBaseType(typeid) == righttype)
			castfunc = InvalidOid;	/* simplest case */
		else
		{
			pathtype = find_coercion_pathway(lefttype, typeid,
											 COERCION_IMPLICIT,
											 &castfunc);
			if (pathtype != COERCION_PATH_FUNC &&
				pathtype != COERCION_PATH_RELABELTYPE)
			{
				/*
				 * The declared input type of the eq_opr might be a
				 * polymorphic type such as ANYARRAY or ANYENUM, or other
				 * special cases such as RECORD; find_coercion_pathway
				 * currently doesn't subsume these special cases.
				 */
				if (!IsBinaryCoercible(typeid, lefttype))
					elog(ERROR, "no conversion function from %s to %s",
						 format_type_be(typeid),
						 format_type_be(lefttype));
			}
		}
		if (OidIsValid(castfunc))
			fmgr_info_cxt(castfunc, &entry->cast_func_finfo,
						  TopMemoryContext);
		else
			entry->cast_func_finfo.fn_oid = InvalidOid;
		entry->valid = true;
	}

	return entry;
}
/* …（同文件无关代码省略）… */
	return RI_TRIGGER_NONE;
}

/*
 * ri_FastPathEndBatch
 *		Flush remaining rows and tear down cached state.
 *
 * Registered as an AfterTriggerBatchCallback.  Note: the flush can
 * do real work (CCI, security context switch, index probes) and can
 * throw ERROR on a constraint violation.  If that happens,
 * ri_FastPathTeardown never runs; ResourceOwner releases the cached
 * relations and AtEOXact_RI() resets the static state on the abort path.
 */
static void
ri_FastPathEndBatch(void *arg)
{
	HASH_SEQ_STATUS status;
	RI_FastPathEntry *entry;
	int			my_depth = (int) (intptr_t) arg;

	if (ri_fastpath_cache == NULL)
		return;

	/*
	 * Set a flag for the duration of the scan so that any FK check triggered
	 * by user cast or operator code during a flush takes the per-row path
	 * instead of adding a new entry to the cache we are iterating.  A new
	 * entry could land in an already-scanned bucket and then be torn down
	 * unflushed below.
	 *
	 * The flush can throw ERROR (a reported constraint violation, or an error
	 * from the user code it runs).  In that case ri_FastPathTeardown below is
	 * skipped; the ResourceOwner and the transaction-end callback handle
	 * resource cleanup on the abort path.  The PG_FINALLY only resets the
	 * flag and deliberately does not attempt teardown.
	 */
	Assert(!ri_fastpath_flushing);
	ri_fastpath_flushing = true;
	PG_TRY();
	{
		hash_seq_init(&status, ri_fastpath_cache);
		while ((entry = hash_seq_search(&status)) != NULL)
		{
			/* Flush only entries created in the cycle now ending. */
			if (entry->key.query_depth == my_depth && entry->batch_count > 0)
			{
				Relation	fk_rel = table_open(entry->fk_relid, AccessShareLock);
				RI_ConstraintInfo *riinfo;

				riinfo = ri_LoadConstraintInfo(entry->key.conoid);

				ri_FastPathBatchFlush(entry, fk_rel, riinfo);
				table_close(fk_rel, NoLock);
			}
		}
	}
	PG_FINALLY();
	{
		ri_fastpath_flushing = false;
	}
	PG_END_TRY();

	/*
	 * Release this cycle's entries and remove them from the cache; leave
	 * outer cycles' entries for their own callbacks.  Destroy the cache once
	 * empty.
	 */
	ri_FastPathTeardown(my_depth);
}

/*
 * ri_FastPathTeardown
 *		Release and remove the cached entries of one firing cycle, and drop
 *		the cache once it holds no more entries.
 *
 * Called from ri_FastPathEndBatch() with the depth of the cycle that is
 * ending: it releases only that cycle's entries, leaving an outer cycle's
 * still-live entries for their own callbacks.  The cache (and its static
 * pointer) go away once the last entry is removed.
 */
static void
ri_FastPathTeardown(int depth)
{
	HASH_SEQ_STATUS status;
	RI_FastPathEntry *entry;

	if (ri_fastpath_cache == NULL)
		return;

	hash_seq_init(&status, ri_fastpath_cache);
	while ((entry = hash_seq_search(&status)) != NULL)
	{
		if (entry->key.query_depth != depth)
			continue;
		if (entry->idx_rel)
			index_close(entry->idx_rel, NoLock);
		if (entry->pk_rel)
			table_close(entry->pk_rel, NoLock);
		if (entry->pk_slot)
			ExecDropSingleTupleTableSlot(entry->pk_slot);
		if (entry->fk_slot)
			ExecDropSingleTupleTableSlot(entry->fk_slot);
		if (entry->flush_cxt)
			MemoryContextDelete(entry->flush_cxt);
		hash_search(ri_fastpath_cache, &entry->key, HASH_REMOVE, NULL);
	}

	if (hash_get_num_entries(ri_fastpath_cache) == 0)
	{
		hash_destroy(ri_fastpath_cache);
		ri_fastpath_cache = NULL;
		ri_fastpath_flushing = false;
	}
}

/*
 * AtEOXact_RI
 *		Reset fast-path batching state at end of transaction.
 *
 * Called from CommitTransaction() and PrepareTransaction() with isCommit
 * true, and from AbortTransaction() with isCommit false.
 *
 * By the time we get here on a clean commit or prepare, the fast-path cache
 * has already been flushed and torn down by ri_FastPathEndBatch() (an
 * AfterTriggerBatchCallback fired from AfterTriggerFireDeferred(), well before
 * this point), so the static pointers are already clear and the reset below is
 * a no-op.  A surviving cache at commit means a trigger batch was never
 * flushed, which would have silently skipped FK checks, so we complain.
 *
 * On abort, ri_FastPathEndBatch()/ri_FastPathTeardown() may not have run (a
 * flush can error out partway): the ResourceOwner releases the cached
 * relations and the TopTransactionContext reset frees the cache memory, but
 * the process-local static pointers below would dangle into the next
 * transaction.  This resets them so they don't.
 *
 * The reset touches only backend-local static state (no relations, locks,
 * buffers or catalog access), so it has no ordering dependency on the
 * surrounding ResourceOwnerRelease() / AtEOXact_* steps.
 */
void
AtEOXact_RI(bool isCommit)
{
	/*
	 * The cache must be empty on a clean commit or prepare; a survivor means
	 * a trigger batch went unflushed.  Assert for assert-enabled builds and,
	 * since the transaction is already committed by now and FK checks may
	 * have been skipped, also warn in production builds.
	 */
	Assert(ri_fastpath_cache == NULL || !isCommit);
	if (isCommit && ri_fastpath_cache != NULL)
		elog(WARNING, "RI fast-path cache not flushed at end of transaction");

	/*
	 * Clear the static pointers/flags.  The cache memory lives in
	 * TopTransactionContext and is freed by the end-of-transaction
	 * memory-context reset; here we only drop the references to it.
	 */
	ri_fastpath_cache = NULL;

	/*
	 * Also clear the in-flush flag.  ri_FastPathEndBatch() already clears it
	 * via PG_FINALLY, so this is just defensive: it keeps a stale flag from
	 * surviving into the next transaction should any future path leave it
	 * set.
	 */
	ri_fastpath_flushing = false;

	/*
	 * Release fast-path metadata detached during this transaction by
	 * InvalidateConstraintCacheCallBack().  We are past every RI check that
	 * could still hold a pointer into one of these, so freeing here is safe
	 * on both the commit and the abort path.
	 */
	while (ri_fpmeta_dead_list != NULL)
	{
		FastPathMeta *dead = ri_fpmeta_dead_list;
/* …（同文件无关代码省略）… */
		pfree(dead);
	}
}

/*
 * AtEOSubXact_RI
 *		Reset fast-path batching state at subtransaction end.
 *
 * Called from CommitSubTransaction() with isCommit true and from
 * AbortSubTransaction() with isCommit false, in both cases after the
 * subtransaction's ResourceOwnerRelease().
 *
 * Fast-path cache entries are normally flushed and removed at the end of
 * their trigger-firing cycle, and the cache is destroyed when its last entry
 * is removed.  Thus, at a normal subtransaction boundary this is a no-op.
 *
 * The exception is a batch flush that errors out partway and is caught by this
 * subtransaction (e.g. a PL/pgSQL EXCEPTION block): ri_FastPathEndBatch()'s
 * teardown was skipped, so the cache still contains entries whose relations
 * were opened under this subtransaction's resource owner.  That owner has
 * just released those relations, making the entries stale.  Remove those
 * entries so a later firing cycle cannot reuse them.  Entries belonging to
 * outer subtransactions remain valid and are preserved.
 *
 * The remaining slot storage and per-entry flush contexts are reclaimed when
 * TopTransactionContext is reset at top-level transaction end.
 */
void
AtEOSubXact_RI(bool isCommit, SubTransactionId mySubid,
			   SubTransactionId parentSubid)
{
	HASH_SEQ_STATUS status;
	RI_FastPathEntry *entry;
	long		remaining;

	if (ri_fastpath_cache == NULL)
		return;

	/* Process only entries belonging to the ending subtransaction. */
	hash_seq_init(&status, ri_fastpath_cache);
	while ((entry = hash_seq_search(&status)) != NULL)
	{
		if (entry->subid != mySubid)
			continue;

		if (isCommit)
		{
			/*
			 * A committing subxact's entry should already have been flushed
			 * and torn down at its statement's end (ri_FastPathEndBatch()),
			 * so we don't expect to find one here.  If we do, reassign it to
			 * the parent so it's still cleaned up rather than left under a
			 * subxact id that no longer exists.
			 */
			Assert(false);
			entry->subid = parentSubid;
		}
		else
			hash_search(ri_fastpath_cache, &entry->key, HASH_REMOVE, NULL);
	}

	/* If that emptied the cache, drop it so the next batch starts clean. */
	remaining = hash_get_num_entries(ri_fastpath_cache);
	if (remaining == 0)
	{
		hash_destroy(ri_fastpath_cache);
		ri_fastpath_cache = NULL;
		ri_fastpath_flushing = false;
	}
}

/*
 * ri_FastPathGetEntry
 *		Look up or create a per-batch cache entry for the given constraint.
 *
 * On first call for a constraint within a batch: opens pk_rel and the index,
 * allocates slots for both FK row and the looked up PK row, and registers the
 * cleanup callback.
 *
 * On subsequent calls: returns the existing entry.
 *
 * Return NULL if the index is unsuitable for the fast path.
 */
static RI_FastPathEntry *
ri_FastPathGetEntry(RI_ConstraintInfo *riinfo, Relation fk_rel)
{
	RI_FastPathKey key;
	RI_FastPathEntry *entry;
	bool		found;
	int			cur_depth = AfterTriggerCurrentQueryDepth();

	key.conoid = riinfo->constraint_id;
	key.query_depth = cur_depth;

	/* Create hash table on first use in this batch */
	if (ri_fastpath_cache == NULL)
	{
		HASHCTL		ctl;

		ctl.keysize = sizeof(RI_FastPathKey);
		ctl.entrysize = sizeof(RI_FastPathEntry);
		ctl.hcxt = TopTransactionContext;
		ri_fastpath_cache = hash_create("RI fast-path cache",
										16,
										&ctl,
										HASH_ELEM | HASH_BLOBS | HASH_CONTEXT);
	}

	entry = hash_search(ri_fastpath_cache, &key,
						HASH_ENTER, &found);

	if (!found)
	{
		MemoryContext oldcxt;

		/*
		 * Zero out non-key fields so ri_FastPathTeardown is safe if we error
		 * out during partial initialization below.
		 */
		memset(((char *) entry) + offsetof(RI_FastPathEntry, pk_rel), 0,
			   sizeof(RI_FastPathEntry) - offsetof(RI_FastPathEntry, pk_rel));

		oldcxt = MemoryContextSwitchTo(TopTransactionContext);

		entry->fk_relid = RelationGetRelid(fk_rel);

		/*
		 * Open PK table and its unique index.
		 *
		 * RowShareLock on pk_rel matches what the SPI path's SELECT ... FOR
		 * KEY SHARE would acquire as a relation-level lock. AccessShareLock
		 * on the index is standard for index scans.
		 *
		 * We don't release these locks until end of transaction, matching SPI
		 * behavior.
		 */

		INJECTION_POINT("ri-before-pk-lock", NULL);

		entry->pk_rel = table_open(riinfo->pk_relid, RowShareLock);

		/*
		 * conindid may have been read before we took that lock, and REINDEX
		 * CONCURRENTLY moves a constraint to a new index.  Re-read it now:
		 * LockRelationOid() processes invalidation messages after acquiring
		 * the lock, so we either see the new index, or an old one that cannot
		 * be marked dead or dropped until this transaction ends.
		 */
		riinfo = ri_LoadConstraintInfo(riinfo->constraint_id);

		entry->idx_rel = index_open(riinfo->conindid, AccessShareLock);

		if (!ri_check_fastpath_index(riinfo, entry->pk_rel, entry->idx_rel))
		{
			/* No rows or slots yet, and no callback for this entry. */
			index_close(entry->idx_rel, NoLock);
			table_close(entry->pk_rel, NoLock);
			hash_search(ri_fastpath_cache, &key, HASH_REMOVE, NULL);
			MemoryContextSwitchTo(oldcxt);

			/* An empty cache has no callback to destroy it. */
			if (hash_get_num_entries(ri_fastpath_cache) == 0)
			{
				hash_destroy(ri_fastpath_cache);
				ri_fastpath_cache = NULL;
			}
			return NULL;
		}

		entry->pk_slot = table_slot_create(entry->pk_rel, NULL);

		/*
		 * Must be TTSOpsHeapTuple because ExecStoreHeapTuple() is used to
		 * load entries from batch[] into this slot for value extraction.
		 */
		entry->fk_slot = MakeSingleTupleTableSlot(RelationGetDescr(fk_rel),
												  &TTSOpsHeapTuple);

		entry->flush_cxt = AllocSetContextCreate(TopTransactionContext,
												 "RI fast path flush temporary context",
												 ALLOCSET_SMALL_SIZES);
		MemoryContextSwitchTo(oldcxt);

		/*
		 * Register an end-of-batch callback once per firing cycle, passing
		 * the query depth so the callback flushes only entries belonging to
		 * that cycle.
		 */
		{
			bool		depth_registered = false;
			HASH_SEQ_STATUS reg_status;
			RI_FastPathEntry *other;

			/*
			 * An existing entry at this depth means its callback is already
			 * registered.  Ignore the just-created entry, which is already in
			 * the hash.
			 */
			hash_seq_init(&reg_status, ri_fastpath_cache);
			while ((other = hash_seq_search(&reg_status)) != NULL)
			{
				if (other != entry && other->key.query_depth == cur_depth)
				{
					depth_registered = true;
					hash_seq_term(&reg_status);
					break;
				}
			}

			if (!depth_registered)
				RegisterAfterTriggerBatchCallback(ri_FastPathEndBatch,
												  (void *) (intptr_t) cur_depth);
		}

		entry->flushing = false;
		entry->batch_count = 0;
		entry->subid = GetCurrentSubTransactionId();
	}
	else
	{
		/*
		 * Invalidation can reset the cached eligibility while an entry is
		 * still in use.  Its held index remains usable, even if REINDEX
		 * CONCURRENTLY has replaced it with an equivalent new index.
		 */
		bool		usable;

		usable = ri_check_fastpath_index(riinfo, entry->pk_rel, entry->idx_rel);
		Assert(usable);
		if (!usable)
			return NULL;
	}

	return entry;
}
