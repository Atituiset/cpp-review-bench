// AUTO-DRAFT from postgres/postgres PR #3ee29a60253c9d68ebe057fc154f54f9cddeb51c
  // <<< BUG ANCHOR
#include "btree_gist.h"
#include "btree_utils_var.h"
#include "utils/builtins.h"
#include "utils/numeric.h"
#include "utils/rel.h"
#include "utils/sortsupport.h"

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
/* …（同文件无关代码省略）… */
	GBT_VARKEY_R rk,
				ok,
				uk;

	rk = gbt_var_key_readable(org);
	uni = PointerGetDatum(gbt_var_key_copy(&rk));
	gbt_var_bin_union(&uni, newe, PG_GET_COLLATION(), &tinfo, fcinfo->flinfo);
	ok = gbt_var_key_readable(org);
	uk = gbt_var_key_readable((GBT_VARKEY *) DatumGetPointer(uni));

	us = DatumGetNumeric(DirectFunctionCall2(numeric_sub,
											 PointerGetDatum(uk.upper),
											 PointerGetDatum(uk.lower)));

	os = DatumGetNumeric(DirectFunctionCall2(numeric_sub,
											 PointerGetDatum(ok.upper),
											 PointerGetDatum(ok.lower)));

	ds = DatumGetNumeric(DirectFunctionCall2(numeric_sub,
											 NumericGetDatum(us),
											 NumericGetDatum(os)));

	if (numeric_is_nan(us))
	{
		if (numeric_is_nan(os))
			*result = 0.0;
		else
			*result = 1.0;
	}
	else
	{
		Numeric		nul = int64_to_numeric(0);

		*result = 0.0;

		if (DatumGetBool(DirectFunctionCall2(numeric_gt, NumericGetDatum(ds), NumericGetDatum(nul))))
