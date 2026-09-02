// AUTO-DRAFT from postgres/postgres PR #cc9a8eb112fa9cf5eb868306955198adb98e497e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	[C_NUM_ATTRIBUTE_STATS_ARGS] = {0}
};

static bool attribute_statistics_update(FunctionCallInfo fcinfo);
static bool attribute_statistics_update_internal(Oid reloid,
												 const char *attname,
												 AttrNumber attnum,
												 bool inherited,
												 FunctionCallInfo fcinfo);
static void upsert_pg_statistic(Relation starel, HeapTuple oldtup,
								const Datum *values, const bool *nulls, const bool *replaces);
static bool delete_pg_statistic(Oid reloid, AttrNumber attnum, bool stainherit);
/* …（同文件无关代码省略）… */
 * and other statistic kinds may still be updated.
 */
static bool
attribute_statistics_update(FunctionCallInfo fcinfo)
{
	char	   *nspname;
	char	   *relname;
/* …（同文件无关代码省略）… */
	AttrNumber	attnum;
	bool		inherited;
	Oid			locked_table = InvalidOid;

	stats_check_required_arg(fcinfo, attarginfo, ATTRELSCHEMA_ARG);
	stats_check_required_arg(fcinfo, attarginfo, ATTRELNAME_ARG);

	nspname = TextDatumGetCString(PG_GETARG_DATUM(ATTRELSCHEMA_ARG));
	relname = TextDatumGetCString(PG_GETARG_DATUM(ATTRELNAME_ARG));

	if (RecoveryInProgress())
		ereport(ERROR,
/* …（同文件无关代码省略）… */
									  RangeVarCallbackForStats, &locked_table);

	/* user can specify either attname or attnum, but not both */
	if (!PG_ARGISNULL(ATTNAME_ARG))
	{
		if (!PG_ARGISNULL(ATTNUM_ARG))
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("cannot specify both \"%s\" and \"%s\"", "attname", "attnum")));
		attname = TextDatumGetCString(PG_GETARG_DATUM(ATTNAME_ARG));
		attnum = get_attnum(reloid, attname);
		/* note that this test covers attisdropped cases too: */
		if (attnum == InvalidAttrNumber)
/* …（同文件无关代码省略）… */
					 errmsg("column \"%s\" of relation \"%s\" does not exist",
							attname, relname)));
	}
	else if (!PG_ARGISNULL(ATTNUM_ARG))
	{
		attnum = PG_GETARG_INT16(ATTNUM_ARG);
		attname = get_attname(reloid, attnum, true);
		/* annoyingly, get_attname doesn't check attisdropped */
		if (attname == NULL ||
/* …（同文件无关代码省略）… */
				 errmsg("cannot modify statistics on system column \"%s\"",
						attname)));

	stats_check_required_arg(fcinfo, attarginfo, INHERITED_ARG);
	inherited = PG_GETARG_BOOL(INHERITED_ARG);

	return attribute_statistics_update_internal(reloid, attname, attnum,
												inherited, fcinfo);
}

/*
/* …（同文件无关代码省略）… */
static bool
attribute_statistics_update_internal(Oid reloid,
									 const char *attname, AttrNumber attnum,
									 bool inherited, FunctionCallInfo fcinfo)
{
	Relation	starel;
	HeapTuple	statup;
/* …（同文件无关代码省略）… */

	FmgrInfo	array_in_fn;

	bool		do_mcv = !PG_ARGISNULL(MOST_COMMON_FREQS_ARG) &&
		!PG_ARGISNULL(MOST_COMMON_VALS_ARG);
	bool		do_histogram = !PG_ARGISNULL(HISTOGRAM_BOUNDS_ARG);
	bool		do_correlation = !PG_ARGISNULL(CORRELATION_ARG);
	bool		do_mcelem = !PG_ARGISNULL(MOST_COMMON_ELEMS_ARG) &&
		!PG_ARGISNULL(MOST_COMMON_ELEM_FREQS_ARG);
	bool		do_dechist = !PG_ARGISNULL(ELEM_COUNT_HISTOGRAM_ARG);
	bool		do_bounds_histogram = !PG_ARGISNULL(RANGE_BOUNDS_HISTOGRAM_ARG);
	bool		do_range_length_histogram = !PG_ARGISNULL(RANGE_LENGTH_HISTOGRAM_ARG) &&
		!PG_ARGISNULL(RANGE_EMPTY_FRAC_ARG);

	Datum		values[Natts_pg_statistic] = {0};
	bool		nulls[Natts_pg_statistic] = {0};
/* …（同文件无关代码省略）… */
	 * and skip the corresponding statistics kind, reporting back a failure.
	 */

	if (!stats_check_arg_array(fcinfo, attarginfo, MOST_COMMON_FREQS_ARG))
	{
		do_mcv = false;
		result = false;
	}

	if (!stats_check_arg_array(fcinfo, attarginfo, MOST_COMMON_ELEM_FREQS_ARG))
	{
		do_mcelem = false;
		result = false;
	}
	if (!stats_check_arg_array(fcinfo, attarginfo, ELEM_COUNT_HISTOGRAM_ARG))
	{
		do_dechist = false;
		result = false;
	}

	if (!stats_check_arg_pair(fcinfo, attarginfo,
							  MOST_COMMON_VALS_ARG, MOST_COMMON_FREQS_ARG))
	{
		do_mcv = false;
		result = false;
	}

	if (!stats_check_arg_pair(fcinfo, attarginfo,
							  MOST_COMMON_ELEMS_ARG,
							  MOST_COMMON_ELEM_FREQS_ARG))
	{
		do_mcelem = false;
		result = false;
	}

	if (!stats_check_arg_pair(fcinfo, attarginfo,
							  RANGE_LENGTH_HISTOGRAM_ARG,
							  RANGE_EMPTY_FRAC_ARG))
	{
		do_range_length_histogram = false;
		result = false;
/* …（同文件无关代码省略）… */
								 replaces);

	/* if specified, set to argument values */
	if (!PG_ARGISNULL(NULL_FRAC_ARG))
	{
		values[Anum_pg_statistic_stanullfrac - 1] = PG_GETARG_DATUM(NULL_FRAC_ARG);
		replaces[Anum_pg_statistic_stanullfrac - 1] = true;
	}
	if (!PG_ARGISNULL(AVG_WIDTH_ARG))
	{
		values[Anum_pg_statistic_stawidth - 1] = PG_GETARG_DATUM(AVG_WIDTH_ARG);
		replaces[Anum_pg_statistic_stawidth - 1] = true;
	}
	if (!PG_ARGISNULL(N_DISTINCT_ARG))
	{
		values[Anum_pg_statistic_stadistinct - 1] = PG_GETARG_DATUM(N_DISTINCT_ARG);
		replaces[Anum_pg_statistic_stadistinct - 1] = true;
	}

	/* STATISTIC_KIND_MCV */
	if (do_mcv)
	{
		bool		converted;
		Datum		stanumbers = PG_GETARG_DATUM(MOST_COMMON_FREQS_ARG);
		Datum		stavalues = statatt_build_stavalues("most_common_vals",
														&array_in_fn,
														PG_GETARG_DATUM(MOST_COMMON_VALS_ARG),
														atttypid, atttypmod,
														&converted);

/* …（同文件无关代码省略）… */

		stavalues = statatt_build_stavalues("histogram_bounds",
											&array_in_fn,
											PG_GETARG_DATUM(HISTOGRAM_BOUNDS_ARG),
											atttypid, atttypmod,
											&converted);

/* …（同文件无关代码省略）… */
	/* STATISTIC_KIND_CORRELATION */
	if (do_correlation)
	{
		Datum		elems[] = {PG_GETARG_DATUM(CORRELATION_ARG)};
		ArrayType  *arry = construct_array_builtin(elems, 1, FLOAT4OID);
		Datum		stanumbers = PointerGetDatum(arry);

/* …（同文件无关代码省略）… */
	/* STATISTIC_KIND_MCELEM */
	if (do_mcelem)
	{
		Datum		stanumbers = PG_GETARG_DATUM(MOST_COMMON_ELEM_FREQS_ARG);
		bool		converted = false;
		Datum		stavalues;

		stavalues = statatt_build_stavalues("most_common_elems",
											&array_in_fn,
											PG_GETARG_DATUM(MOST_COMMON_ELEMS_ARG),
											elemtypid, atttypmod,
											&convert
