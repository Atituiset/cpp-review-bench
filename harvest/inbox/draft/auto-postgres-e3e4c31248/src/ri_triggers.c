// AUTO-DRAFT from postgres/postgres PR #964d88fac464a9b5f55c3d0c9c2ec72bcde5847d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#define RI_MAX_NUMKEYS					INDEX_MAX_KEYS
/* …（同文件无关代码省略）… */
#define RI_INIT_CONSTRAINTHASHSIZE		64
#define RI_INIT_QUERYHASHSIZE			(RI_INIT_CONSTRAINTHASHSIZE * 4)
/* …（同文件无关代码省略）… */
#define RIAttType(rel, attnum)	attnumTypeId(rel, attnum)
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

typedef struct RI_CompareHashEntry RI_CompareHashEntry;

/* Fast-path metadata for RI checks on foreign key referencing tables */
typedef struct FastPathMeta
{
	FmgrInfo	eq_opr_finfo[RI_MAX_NUMKEYS];
	FmgrInfo	cast_func_finfo[RI_MAX_NUMKEYS];
	RegProcedure regops[RI_MAX_NUMKEYS];
	Oid			subtypes[RI_MAX_NUMKEYS];
	int			strats[RI_MAX_NUMKEYS];
	AttrNumber	index_attnos[RI_MAX_NUMKEYS];	/* index column positions */

	/*
	 * fn_mcxt for the cached FmgrInfos above.  Cast and equality functions
	 * (e.g. record_eq()) use fn_mcxt as scratch space, caching state there
	 * and keeping a pointer to it in FmgrInfo.fn_extra.  Give them a context
	 * of their own, created with this struct and destroyed with it in
	 * AtEOXact_RI().
	 *
	 * Note this context must not be reset while the FmgrInfos remain in use,
	 * since that would free the state fn_extra still points at.
	 */
	MemoryContext scratch_cxt;

	/* Link in ri_fpmeta_dead_list while awaiting deferred release */
	struct FastPathMeta *next_dead;
} FastPathMeta;
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
static void ri_CheckFunctionPermissions(const RI_ConstraintInfo *riinfo,
										const FastPathMeta *fpmeta);
static void build_index_scankeys(const RI_ConstraintInfo *riinfo,
								 FastPathMeta *fpmeta,
								 Relation idx_rel, Datum *pk_vals,
								 char *pk_nulls, ScanKey skeys);
static void ri_populate_fastpath_metadata(RI_ConstraintInfo *riinfo,
										  Relation fk_rel, Relation idx_rel);
static void ri_ExtractValues(Relation rel, TupleTableSlot *slot,
							 const RI_ConstraintInfo *riinfo, bool rel_is_pk,
							 Datum *vals, char *nulls);
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
	riinfo->pk_relid =
