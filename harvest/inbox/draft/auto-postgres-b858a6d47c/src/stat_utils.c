// AUTO-DRAFT from postgres/postgres PR #2656f664616dac41e99495b39e2bc3bc1d81b1e7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
	{
		ereport(WARNING,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("argument \"%s\" must not be a multidimensional array",
						arginfo[argnum].argname)));
		return false;
	}
/* …（同文件无关代码省略）… */
 * If an error is encountered, capture it and throw a WARNING, with "ok" set
 * to false.  If the resulting array contains NULLs, raise a WARNING and
 * set "ok" to false.  When the operation succeeds, set "ok" to true.
 */
Datum
statatt_build_stavalues(const char *staname, FmgrInfo *array_in, Datum d, Oid typid,
						int32 typmod, bool *ok)
{
	char	   *s;
	Datum		result;
	ErrorSaveContext escontext = {T_ErrorSaveContext};
/* …（同文件无关代码省略）… */

	pfree(s);

	if (ARR_NDIM(DatumGetArrayTypeP(result)) != 1)
	{
		ereport(WARNING,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("\"%s\" must be a one-dimensional array", staname)));
		*ok = false;
		return (Datum) 0;
	}
