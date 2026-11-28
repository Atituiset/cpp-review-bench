// AUTO-DRAFT from postgres/postgres PR #cf9d92d1f7550063aefbb219e1160e58ddb760ac
#include "btree_utils_var.h"
#include "nodes/miscnodes.h"
#include "utils/builtins.h"
#include "utils/float.h"
#include "utils/numeric.h"
#include "utils/rel.h"
#include "utils/sortsupport.h"
  // <<< BUG ANCHOR
/* numeric.h should offer this functionality... */
#define get_numeric_infinity() \
	DatumGetNumeric(DirectFunctionCall1(float8_numeric, \
										Float8GetDatum(get_float8_infinity())))

/* GiST support functions */
PG_FUNCTION_INFO_V1(gbt_numeric_compress);
PG_FUNCTION_INFO_V1(gbt_numeric_union);
/* …（同文件无关代码省略）… */
Datum
gbt_numeric_compress(PG_FUNCTION_ARGS)
{
	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);

	PG_RETURN_POINTER(gbt_var_compress(entry, &tinfo));
}
/* …（同文件无关代码省略）… */
Datum
gbt_numeric_union(PG_FUNCTION_ARGS)
{
	GistEntryVector *entryvec = (GistEntryVector *) PG_GETARG_POINTER(0);
	int32	   *size = (int *) PG_GETARG_POINTER(1);

	PG_RETURN_POINTER(gbt_var_union(entryvec, size, PG_GET_COLLATION(),
									&tinfo, fcinfo->flinfo));
}
