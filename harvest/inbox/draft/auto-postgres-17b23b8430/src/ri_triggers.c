// AUTO-DRAFT from postgres/postgres PR #e5f6a7a28c4ab82a80fd0fa89d9f77b80ae97c07
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
#define RI_MAX_NUMKEYS					INDEX_MAX_KEYS
/* …（同文件无关代码省略）… */
#define RI_INIT_CONSTRAINTHASHSIZE		64
#define RI_INIT_QUERYHASHSIZE			(RI_INIT_CONSTRAINTHASHSIZE * 4)
/* …（同文件无关代码省略）… */
#define RI_TRIGTYPE_INSERT 1
#define RI_TRIGTYPE_UPDATE 2
#define RI_TRIGTYPE_DELETE 3
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
typedef struct RI_CompareHashEntry
{
	RI_CompareKey key;
	bool		valid;			/* successfully initialized? */
	FmgrInfo	eq_opr_finfo;	/* call info for equality fn */
	FmgrInfo	cast_func_finfo;	/* in case we must coerce input */
} RI_CompareHashEntry;
/* …（同文件无关代码省略）… */
 */
static FastPathMeta *ri_fpmeta_dead_list = NULL;

/*
 * Local function prototypes
 */
/* …（同文件无关代码省略）… */
											  uint32 hashvalue);
static SPIPlanPtr ri_FetchPreparedPlan(RI_QueryKey *key);
static void ri_HashPreparedPlan(RI_QueryKey *key, SPIPlanPtr plan);
static RI_CompareHashEntry *ri_HashCompareOp(Oid eq_opr, Oid typeid);

static void ri_CheckTrigger(FunctionCallInfo fcinfo, const char *funcname,
/* …（同文件无关代码省略）… */
static void
ri_CheckTrigger(FunctionCallInfo fcinfo, const char *funcname, int tgkind)
{
	TriggerData *trigdata = (TriggerData *) fcinfo->context;

	if (!CALLED_AS_TRIGGER(fcinfo))
		ereport(ERROR,
				(errcode(ERRCODE_E_R_I_E_TRIGGER_PROTOCOL_VIOLATED),
				 errmsg("function \"%s\" was not called by trigger manager", funcname)));

	/*
	 * Check proper event
	 */
	if (!TRIGGER_FIRED_AFTER(trigdata->tg_event) ||
		!TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
		ereport(ERROR,
				(errcode(ERRCODE_E_R_I_E_TRIGGER_PROTOCOL_VIOLATED),
				 errmsg("function \"%s\" must be fired AFTER ROW", funcname)));

	switch (tgkind)
	{
		case RI_TRIGTYPE_INSERT:
			if (!TRIGGER_FIRED_BY_INSERT(trigdata->tg_event))
				ereport(ERROR,
						(errcode(ERRCODE_E_R_I_E_TRIGGER_PROTOCOL_VIOLATED),
						 errmsg("function \"%s\" must be fired for INSERT", funcname)));
			break;
		case RI_TRIGTYPE_UPDATE:
			if (!TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
				ereport(ERROR,
						(errcode(ERRCODE_E_R_I_E_TRIGGER_PROTOCOL_VIOLATED),
						 errmsg("function \"%s\" must be fired for UPDATE", funcname)));
			break;
		case RI_TRIGTYPE_DELETE:
			if (!TRIGGER_FIRED_BY_DELETE(trigdata->tg_event))
				ereport(ERROR,
						(errcode(ERRCODE_E_R_I_E_TRIGGER_PROTOCOL_VIOLATED),
						 errmsg("function \"%s\" must be fired for DELETE", funcname)));
			break;
	}
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
	 * Likewise for pg_cast: whether a constraint's FK values are
	 * binary-coercible to the operator's input type, which decides fast-path
	 * eligibility, can depend on a binary cast, which may be replaced.
	 */
	if (cacheid == CASTSOURCETARGET)
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
			 * copies riinfo->fpmeta into a local and then does catalog
			 * lookups, index scans, tuple locking, and calls equality
			 * functions, all of which can accept invalidation messages and
			 * reach this callback.  Freeing now would leave it reading freed
			 * memory.  Queue it instead; AtEOXact_RI() releases it once no RI
			 * check can be running.
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
	Snapshot	crosscheck_snapshot;
	int			limit;
	int			spi_result;
	Oid			save_userid;
	int			save_sec_context;
	Datum		vals[RI_MAX_NUMKEYS * 2];
/* …（同文件无关代码省略）… */
	 * Set fire_triggers to false to ensure that AFTER triggers are queued in
	 * the outer query's after-trigger context and fire after all RI updates
	 * on the same row are complete, rather than immediately.
	 */
	spi_result = SPI_execute_snapshot(qplan,
									  vals, nulls,
									  test_snapshot, crosscheck_snapshot,
									  false, false, limit);

	/* Restore UID and security context */
	SetUserIdAndSecContext(save_userid, save_sec_context);

	/* Check result */
	if (spi_result < 0)
		elog(ERROR, "SPI_execute_snapshot returned %s", SPI_result_code_string(spi_result));
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

	/* Arrange to flush cache on pg_constraint, pg_amop or pg_cast changes */
	CacheRegisterSyscacheCallback(CONSTROID,
								  InvalidateConstraintCacheCallBack,
								  (Datum) 0);
	CacheRegisterSyscacheCallback(AMOPOPID,
								  InvalidateConstraintCacheCallBack,
								  (Datum) 0);
	CacheRegisterSyscacheCallback(CASTSOURCETARGET,
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
static SPIPlanPtr
ri_FetchPreparedPlan(RI_QueryKey *key)
{
	RI_QueryHashEntry *entry;
	SPIPlanPtr	plan;

	/*
	 * On the first call initialize the hashtable
	 */
	if (!ri_query_cache)
		ri_InitHashTables();

	/*
	 * Lookup for the key
	 */
	entry = (RI_QueryHashEntry *) hash_search(ri_query_cache,
											  key,
											  HASH_FIND, NULL);
	if (entry == NULL)
		return NULL;

	/*
	 * Check whether the plan is still valid.  If it isn't, we don't want to
	 * simply rely on plancache.c to regenerate it; rather we should start
	 * from scratch and rebuild the query text too.  This is to cover cases
	 * such as table/column renames.  We depend on the plancache machinery to
	 * detect possible invalidations, though.
	 *
	 * CAUTION: this check is only trustworthy if the caller has already
	 * locked both FK and PK rels.
	 */
	plan = entry->plan;
	if (plan && SPI_plan_is_valid(plan))
		return plan;

	/*
	 * Otherwise we might as well flush the cached plan now, to free a little
	 * memory space before we make a new one.
	 */
	entry->plan = NULL;
	if (plan)
		SPI_freeplan(plan);

	return NULL;
}
/* …（同文件无关代码省略）… */
static void
ri_HashPreparedPlan(RI_QueryKey *key, SPIPlanPtr plan)
{
	RI_QueryHashEntry *entry;
	bool		found;

	/*
	 * On the first call initialize the hashtable
	 */
	if (!ri_query_cache)
		ri_InitHashTables();

	/*
	 * Add the new plan.  We might be overwriting an entry previously found
	 * invalid by ri_FetchPreparedPlan.
	 */
	entry = (RI_QueryHashEntry *) hash_search(ri_query_cache,
											  key,
											  HASH_ENTER, &found);
	Assert(!found || entry->plan == NULL);
	entry->plan = plan;
}


/*
 * ri_KeysEqual -
 *
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
		Assert(lefttype == righttype);
		if (typeid == lefttype)
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
 * AtEOXact_RI
 *		End-of-transaction cleanup for referential integrity.
 *
 * Currently this only releases fast-path metadata detached during the
 * transaction.  InvalidateConstraintCacheCallBack() cannot free a
 * FastPathMeta when it detaches one, because an RI check further up the
 * stack may still hold a pointer into it.  It queues them on
 * ri_fpmeta_dead_list instead, and we release them here, where no such
 * reference can exist.  isCommit is accepted for consistency with the
 * other AtEOXact_* routines but is not used: the release is the same on
 * the commit and the abort path.
 *
 * There is no AtEOSubXact_RI() counterpart.  Nothing here is scoped to a
 * subtransaction: a detached FastPathMeta stays reachable from the dead
 * list whichever subtransaction detached it, and a check holding a pointer
 * into one may be running at an outer level, so releasing at subtransaction
 * end would be unsafe as well as unnecessary.
 */
void
AtEOXact_RI(bool isCommit)
{
	while (ri_fpmeta_dead_list != NULL)
	{
		FastPathMeta *dead = ri_fpmeta_dead_list;

		ri_fpmeta_dead_list = dead->next_dead;
		pfree(dead);
	}
}
