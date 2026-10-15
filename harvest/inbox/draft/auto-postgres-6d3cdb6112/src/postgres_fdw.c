// AUTO-DRAFT from postgres/postgres PR #cc9a8eb112fa9cf5eb868306955198adb98e497e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
						  int attrcnt)
{
	PGresult   *res;
	NullableDatum args[ATTSTATS_NUM_FIELDS];

	/* Set the 'version' parameter, which is common to both statistics. */
	args[0].value = Int32GetDatum(remstats->version);
	args[0].isnull = false;

	/*
	 * We import attribute statistics first, if any, because those are more
/* …（同文件无关代码省略）… */
		{
			int			row = remattrmap[mapidx].res_index;
			AttrNumber	attnum = remattrmap[mapidx].local_attnum;

			/* All mappings should have been assigned a result set row. */
			Assert(row >= 0);
/* …（同文件无关代码省略）… */
			/* Clear existing attribute statistics. */
			delete_attribute_statistics(relation, attnum, false);

			/* Set the remaining parameters. */
			set_float_arg(&args[1],
						  get_opt_value(res, row, ATTSTATS_NULL_FRAC));
			set_int32_arg(&args[2],
						  get_opt_value(res, row, ATTSTATS_AVG_WIDTH));
			set_float_arg(&args[3],
						  get_opt_value(res, row, ATTSTATS_N_DISTINCT));
			set_text_arg(&args[4],
						 get_opt_value(res, row, ATTSTATS_MOST_COMMON_VALS));
			set_floatarr_arg(&args[5],
							 get_opt_value(res, row, ATTSTATS_MOST_COMMON_FREQS));
			set_text_arg(&args[6],
						 get_opt_value(res, row, ATTSTATS_HISTOGRAM_BOUNDS));
			set_float_arg(&args[7],
						  get_opt_value(res, row, ATTSTATS_CORRELATION));
			set_text_arg(&args[8],
						 get_opt_value(res, row, ATTSTATS_MOST_COMMON_ELEMS));
			set_floatarr_arg(&args[9],
							 get_opt_value(res, row, ATTSTATS_MOST_COMMON_ELEM_FREQS));
			set_floatarr_arg(&args[10],
							 get_opt_value(res, row, ATTSTATS_ELEM_COUNT_HISTOGRAM));
			set_text_arg(&args[11],
						 get_opt_value(res, row, ATTSTATS_RANGE_LENGTH_HISTOGRAM));
			set_float_arg(&args[12],
						  get_opt_value(res, row, ATTSTATS_RANGE_EMPTY_FRAC));
			set_text_arg(&args[13],
						 get_opt_value(res, row, ATTSTATS_RANGE_BOUNDS_HISTOGRAM));

			/* Try to import the statistics. */
			if (!import_attribute_statistics(relation, attnum, false,
											 &args[0], &args[1], &args[2],
											 &args[3], &args[4], &args[5],
											 &args[6], &args[7], &args[8],
											 &args[9], &args[10], &args[11],
											 &args[12], &args[13]))
			{
				ereport(WARNING,
						errmsg("could not import statistics for foreign table \"%s.%s\" --- attribute statistics import failed for column \"%s\" of this foreign table",
/* …（同文件无关代码省略）… */
	Assert(PQnfields(res) == RELSTATS_NUM_FIELDS);
	Assert(PQntuples(res) == 1);

	/* Set the remaining parameters. */
	set_int32_arg(&args[1], get_opt_value(res, 0, RELSTATS_RELPAGES));
	Assert(!args[1].isnull);
	set_float_arg(&args[2], get_opt_value(res, 0, RELSTATS_RELTUPLES));
	Assert(!args[2].isnull);
	/* We don't import relallvisible/relallfrozen. */
	args[3].value = (Datum) 0;
	args[3].isnull = true;
	args[4].value = (Datum) 0;
	args[4].isnull = true;

	/* Try to import the statistics. */
	if (!import_relation_statistics(relation, &args[0], &args[1],
									&args[2], &args[3], &args[4]))
	{
		ereport(WARNING,
				errmsg("could not import statistics for foreign table \"%s.%s\" --- relation statistics import failed for this foreign table",
/* …（同文件无关代码省略）… */
static char *
get_opt_value(PGresult *res, int row, int col)
{
	if (PQgetisnull(res, row, col))
		return NULL;
	return PQgetvalue(res, row, col);
}
/* …（同文件无关代码省略）… */
static void
set_text_arg(NullableDatum *arg, const char *s)
{
	if (s)
	{
		arg->value = CStringGetTextDatum(s);
		arg->isnull = false;
	}
	else
	{
		arg->value = (Datum) 0;
		arg->isnull = true;
	}
}
/* …（同文件无关代码省略）… */
static void
set_int32_arg(NullableDatum *arg, const char *s)
{
	if (s)
	{
		int32		val = pg_strtoint32(s);

		arg->value = Int32GetDatum(val);
		arg->isnull = false;
	}
	else
	{
		arg->value = (Datum) 0;
		arg->isnull = true;
	}
}
/* …（同文件无关代码省略）… */
static void
set_float_arg(NullableDatum *arg, const char *s)
{
	if (s)
	{
		float4		val = float4in_internal((char *) s, NULL, "float", s, NULL);

		arg->value = Float4GetDatum(val);
		arg->isnull = false;
	}
	else
	{
		arg->value = (Datum) 0;
		arg->isnull = true;
	}
}
/* …（同文件无关代码省略）… */
static void
set_floatarr_arg(NullableDatum *arg, const char *s)
{
	if (s)
	{
		FmgrInfo	flinfo;
		Datum		val;

		fmgr_info(F_ARRAY_IN, &flinfo);
		val = InputFunctionCall(&flinfo, s, FLOAT4OID, -1);

		arg->value = val;
		arg->isnull = false;
	}
	else
	{
		arg->value = (Datum) 0;
		arg->isnull = true;
	}
}
