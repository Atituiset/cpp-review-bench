// AUTO-DRAFT from postgres/postgres PR #142fd8ff13dee8ff6665615027386908e9768d43
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
 * This duplicates the logic in examine_attribute() but it will not skip the
 * attribute if the attstattarget is 0.
 *
 * This information, retrieved from pg_attribute and pg_type with some
 * specific handling for index expressions, is a prerequisite to calling
 * any of the other statatt_*() functions.
 */
void
statatt_get_type(Oid reloid, AttrNumber attnum,
				 Oid *atttypid, int32 *atttypmod,
				 char *atttyptype, Oid *atttypcoll,
				 Oid *eq_opr, Oid *lt_opr)
{
	Relation	rel = relation_open(reloid, AccessShareLock);
	Form_pg_attribute attr;
	HeapTuple	atup;
	Node	   *expr;
	TypeCacheEntry *typcache;

	atup = SearchSysCache2(ATTNUM, ObjectIdGetDatum(reloid),
						   Int16GetDatum(attnum));
/* …（同文件无关代码省略）… */
	ReleaseSysCache(atup);

	/* finds the right operators even if atttypid is a domain */
	typcache = lookup_type_cache(*atttypid, TYPECACHE_LT_OPR | TYPECACHE_EQ_OPR);
	*atttyptype = typcache->typtype;
	*eq_opr = typcache->eq_opr;
	*lt_opr = typcache->lt_opr;

	/*
	 * Special case: collation for tsvector is DEFAULT_COLLATION_OID. See
	 * compute_tsvector_stats().
	 */
	if (*atttypid == TSVECTOROID)
		*atttypcoll = DEFAULT_COLLATION_OID;

	relation_close(rel, NoLock);
}

/*
 * Derive element type information from the attribute type.  This information
 * is needed when the given type is one that contains elements of other types.
 *
 * The atttypid and atttyptype should be derived from a previous call to
 * statatt_get_type().
 */
bool
statatt_get_elem_type(Oid atttypid, char atttyptype,
					  Oid *elemtypid, Oid *elem_eq_opr)
{
	TypeCacheEntry *elemtypcache;

	if (atttypid == TSVECTOROID)
	{
		/*
		 * Special case: element type for tsvector is text. See
/* …（同文件无关代码省略）… */
	}
	else
	{
		/* find underlying element type through any domain */
		*elemtypid = get_base_element_type(atttypid);
	}

	if (!OidIsValid(*elemtypid))
/* …（同文件无关代码省略）… */
	return true;
}

/*
 * Build an array with element type typid from a text datum, used as
 * value of an attribute in a tuple to-be-inserted into pg_statistic.
