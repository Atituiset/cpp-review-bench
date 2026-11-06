// AUTO-DRAFT from postgres/postgres PR #d06ac896faa7ff14e6eca5c9bc27a34818adf634
#include "port/pg_bitutils.h"
#include "utils/builtins.h"
#include "utils/date.h"
#include "utils/lsyscache.h"
#include "utils/rangetypes.h"
#include "utils/sortsupport.h"
#include "utils/timestamp.h"
/* …（同文件无关代码省略）… */
 *
 * Note that subtype_diff does return the difference, not the absolute value
 * of the difference, and it must take care to avoid overflow.
 * (numrange_subdiff is at some risk there ...)
 *----------------------------------------------------------
 */
  // <<< BUG ANCHOR
/* …（同文件无关代码省略）… */
Datum
numrange_subdiff(PG_FUNCTION_ARGS)
{
	Datum		v1 = PG_GETARG_DATUM(0);
	Datum		v2 = PG_GETARG_DATUM(1);
	Datum		numresult;
	float8		floatresult;

	numresult = DirectFunctionCall2(numeric_sub, v1, v2);

	floatresult = DatumGetFloat8(DirectFunctionCall1(numeric_float8,
													 numresult));

	PG_RETURN_FLOAT8(floatresult);
}
