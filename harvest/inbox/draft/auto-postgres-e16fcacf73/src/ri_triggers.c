// AUTO-DRAFT from postgres/postgres PR #1a846a555afb0e5f2008f3aefe8f77acbe6ffba6
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#include "access/tableam.h"
#include "access/xact.h"
#include "catalog/index.h"
#include "catalog/pg_am_d.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_constraint.h"
#include "catalog/pg_index.h"
#include "catalog/pg_namespace.h"
#include "commands/trigger.h"
#include "executor/executor.h"
#include "executor/spi.h"
/* …（同文件无关代码省略）… */
#define RI_MAX_NUMKEYS					INDEX_MAX_KEYS
/* …（同文件无关代码省略）… */
#define RI_INIT_CONSTRAINTHASHSIZE		64
#define RI_INIT_QUERYHASHSIZE			(RI_INIT_CONSTRAINTHASHSIZE * 4)
/* …（同文件无关代码省略）… */
#define RIAttType(rel, attnum)	attnumTypeId(rel, attnum)
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
								Relation query_rel);
static bool recheck_matched_pk_tuple(Relation idxrel, ScanKeyData *skeys,
									 int nkeys, TupleTableSlot *new_slot);
static void build_index_scankeys(const RI_ConstraintInfo *riinfo,
								 FastPathMeta *fpmeta,
								 Relation idx_rel, Datum *pk_vals,
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
		ri_populate_fas
