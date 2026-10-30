// AUTO-DRAFT from postgres/postgres PR #4f89416c9dbc78da5679d32072315af56fbeb8ed
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

PG_FUNCTION_INFO_V1(int_custom_out);
PG_FUNCTION_INFO_V1(int_custom_typanalyze_false);
PG_FUNCTION_INFO_V1(int_custom_typanalyze_invalid);
PG_FUNCTION_INFO_V1(int_custom_eq);
PG_FUNCTION_INFO_V1(int_custom_ne);
PG_FUNCTION_INFO_V1(int_custom_lt);
/* …（同文件无关代码省略）… */
Datum
int_custom_out(PG_FUNCTION_ARGS)
{
	int32		arg1 = PG_GETARG_INT32(0);
	char	   *result = (char *) palloc(12);	/* sign, 10 digits, '\0' */

	pg_ltoa(arg1, result);
	PG_RETURN_CSTRING(result);
}
/* …（同文件无关代码省略）… */
Datum
int_custom_typanalyze_false(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}
/* …（同文件无关代码省略）… */
static void
int_custom_invalid_stats(VacAttrStats *stats, AnalyzeAttrFetchFunc fetchfunc,
						 int samplerows, double totalrows)
{
	/* We are not valid, and do not want to be. */
	stats->stats_valid = false;
}
/* …（同文件无关代码省略）… */
Datum
int_custom_typanalyze_invalid(PG_FUNCTION_ARGS)
{
	VacAttrStats *stats = (VacAttrStats *) PG_GETARG_POINTER(0);

	/* If the attstattarget column is negative, use the default value */
	if (stats->attstattarget < 0)
		stats->attstattarget = default_statistics_target;

	/* Buggy number, no need to care as long as it is positive */
	stats->minrows = 300;

	/* Set callback to compute some invalid stats */
	stats->compute_stats = int_custom_invalid_stats;

	PG_RETURN_BOOL(true);
}

/*
 * Comparison functions for int_custom type
 */
Datum
int_custom_eq(PG_FUNCTION_ARGS)
{
	int32		arg1 = PG_GETARG_INT32(0);
	int32		arg2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(arg1 == arg2);
}
/* …（同文件无关代码省略）… */
Datum
int_custom_ne(PG_FUNCTION_ARGS)
{
	int32		arg1 = PG_GETARG_INT32(0);
	int32		arg2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(arg1 != arg2);
}
/* …（同文件无关代码省略）… */
Datum
int_custom_lt(PG_FUNCTION_ARGS)
{
	int32		arg1 = PG_GETARG_INT32(0);
	int32		arg2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(arg1 < arg2);
}
