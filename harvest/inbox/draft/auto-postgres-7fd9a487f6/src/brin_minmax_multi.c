// AUTO-DRAFT from postgres/postgres PR #c230e14b44d2bc40d0d141861e4cbd3839843df9
#include "catalog/pg_am.h"
#include "catalog/pg_amop.h"
#include "catalog/pg_type.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/date.h"
/* …（同文件无关代码省略）… */
#include "utils/inet.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/pg_lsn.h"
#include "utils/rel.h"
#include "utils/syscache.h"
/* …（同文件无关代码省略）… */
Datum  // <<< BUG ANCHOR
brin_minmax_multi_distance_numeric(PG_FUNCTION_ARGS)
{
	Datum		d;
	Datum		a1 = PG_GETARG_DATUM(0);
	Datum		a2 = PG_GETARG_DATUM(1);

	/*
	 * We know the values are range boundaries, but the range may be collapsed
	 * (i.e. single points), with equal values.
	 */
	Assert(DatumGetBool(DirectFunctionCall2(numeric_le, a1, a2)));

	d = DirectFunctionCall2(numeric_sub, a2, a1);	/* a2 - a1 */

	PG_RETURN_DATUM(DirectFunctionCall1(numeric_float8, d));
}

/*
