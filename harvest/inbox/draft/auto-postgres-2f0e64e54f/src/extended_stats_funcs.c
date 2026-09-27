// AUTO-DRAFT from postgres/postgres PR #142fd8ff13dee8ff6665615027386908e9768d43
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
static char *
jbv_string_get_cstr(JsonbValue *jval)
{
	char	   *s;

	Assert(jval->type == jbvString);

	s = palloc0(jval->val.string.len + 1);
	memcpy(s, jval->val.string.val, jval->val.string.len);

	return s;
}
/* …（同文件无关代码省略）… */
					bool *pg_statistic_ok)
{
	const char *argname = extarginfo[EXPRESSIONS_ARG].argname;
	TypeCacheEntry *typcache;
	Datum		values[Natts_pg_statistic];
	bool		nulls[Natts_pg_statistic];
	bool		replaces[Natts_pg_statistic];
	HeapTuple	pgstup = NULL;
	Datum		pgstdat = (Datum) 0;
	Oid			elemtypid = InvalidOid;
	Oid			elemeqopr = InvalidOid;
	bool		found[NUM_ATTRIBUTE_STATS_ELEMS] = {0};
	JsonbValue	val[NUM_ATTRIBUTE_STATS_ELEMS] = {0};

/* …（同文件无关代码省略）… */
	}

	/* This finds the right operators even if atttypid is a domain */
	typcache = lookup_type_cache(typid, TYPECACHE_LT_OPR | TYPECACHE_EQ_OPR);

	statatt_init_empty_tuple(InvalidOid, InvalidAttrNumber, false,
							 values, nulls, replaces);
/* …（同文件无关代码省略）… */
	 * Special case: collation for tsvector is DEFAULT_COLLATION_OID. See
	 * compute_tsvector_stats().
	 */
	if (typid == TSVECTOROID)
		typcoll = DEFAULT_COLLATION_OID;

	/*
/* …（同文件无关代码省略）… */
	 */
	if (found[MOST_COMMON_ELEMS_ELEM] || found[ELEM_COUNT_HISTOGRAM_ELEM])
	{
		if (!statatt_get_elem_type(typid, typcache->typtype,
								   &elemtypid, &elemeqopr))
		{
			ereport(WARNING,
					errcode(ERRCODE_INVALID_PARAMETER_VALUE),
/* …（同文件无关代码省略）… */
		found[RANGE_EMPTY_FRAC_ELEM] ||
		found[RANGE_BOUNDS_HISTOGRAM_ELEM])
	{
		if (typcache->typtype != TYPTYPE_RANGE &&
			typcache->typtype != TYPTYPE_MULTIRANGE)
		{
			ereport(WARNING,
					errcode(ERRCODE_INVALID_PARAMETER_VALUE),
/* …（同文件无关代码省略）… */

			statatt_set_slot(values, nulls, replaces,
							 STATISTIC_KIND_MCV,
							 typcache->eq_opr, typcoll,
							 stanumbers, false, stavalues, false);
		}
		else
/* …（同文件无关代码省略）… */
		if (val_ok)
			statatt_set_slot(values, nulls, replaces,
							 STATISTIC_KIND_HISTOGRAM,
							 typcache->lt_opr, typcoll,
							 0, true, stavalues, false);
		else
			goto pg_statistic_error;
/* …（同文件无关代码省略）… */

			statatt_set_slot(values, nulls, replaces,
							 STATISTIC_KIND_CORRELATION,
							 typcache->lt_opr, typcoll,
							 stanumbers, false, 0, true);
		}
		else
/* …（同文件无关代码省略）… */
		Datum		stavalues;
		bool		val_ok = false;
		char	   *s;
		Oid			rtypid = typid;

		/*
		 * If it's a multirange, step down to the range type, as is done by
		 * multirange_typanalyze().
		 */
		if (type_is_multirange(typid))
			rtypid = get_multirange_range(typid);

		s = jbv_string_get_cstr(&val[RANGE_BOUNDS_HISTOGRAM_ELEM]);
