// AUTO-DRAFT from postgres/postgres PR #142fd8ff13dee8ff6665615027386908e9768d43
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR

	Oid			atttypid = InvalidOid;
	int32		atttypmod;
	char		atttyptype;
	Oid			atttypcoll = InvalidOid;
	Oid			eq_opr = InvalidOid;
	Oid			lt_opr = InvalidOid;

	Oid			elemtypid = InvalidOid;
	Oid			elem_eq_opr = InvalidOid;

	FmgrInfo	array_in_fn;

	bool		do_mcv = !PG_ARGISNULL(MOST_COMMON_FREQS_ARG) &&
/* …（同文件无关代码省略）… */
	/* derive information from attribute */
	statatt_get_type(reloid, attnum,
					 &atttypid, &atttypmod,
					 &atttyptype, &atttypcoll,
					 &eq_opr, &lt_opr);

	/* if needed, derive element type */
	if (do_mcelem || do_dechist)
	{
		if (!statatt_get_elem_type(atttypid, atttyptype,
								   &elemtypid, &elem_eq_opr))
		{
			ereport(WARNING,
					(errmsg("could not determine element type of column \"%s\"", attname),
/* …（同文件无关代码省略）… */

	/* only range types can have range stats */
	if ((do_range_length_histogram || do_bounds_histogram) &&
		!(atttyptype == TYPTYPE_RANGE || atttyptype == TYPTYPE_MULTIRANGE))
	{
		ereport(WARNING,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
/* …（同文件无关代码省略）… */
	{
		bool		converted = false;
		Datum		stavalues;
		Oid			bounds_typid = atttypid;

		/*
		 * If it's a multirange, step down to the range type, as is done by
		 * multirange_typanalyze().
		 */
		if (type_is_multirange(atttypid))
			bounds_typid = get_multirange_range(atttypid);

		stavalues = statatt_build_stavalues("range_bounds_histogram",
											&array_in_fn,
