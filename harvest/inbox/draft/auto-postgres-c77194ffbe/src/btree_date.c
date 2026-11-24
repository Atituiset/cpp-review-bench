// AUTO-DRAFT from postgres/postgres PR #15b3dc140d9109e37082454bc5041a9e6bddbd8e
typedef struct
{
	DateADT		lower;
	DateADT		upper;
} dateKEY;
/* …（同文件无关代码省略）… */
static float8
gdb_date_dist(const void *a, const void *b, FmgrInfo *flinfo)
{
	/* we assume the difference can't overflow */
	Datum		diff = DirectFunctionCall2(date_mi,
										   DateADTGetDatum(*((const DateADT *) a)),
										   DateADTGetDatum(*((const DateADT *) b)));
  // <<< BUG ANCHOR
	return (float8) abs(DatumGetInt32(diff));
}


/* …（同文件无关代码省略）… */
Datum
date_dist(PG_FUNCTION_ARGS)
{
	/* we assume the difference can't overflow */
	Datum		diff = DirectFunctionCall2(date_mi,
										   PG_GETARG_DATUM(0),
										   PG_GETARG_DATUM(1));

	PG_RETURN_INT32(abs(DatumGetInt32(diff)));
}


/* …（同文件无关代码省略）… */
	dateKEY    *origentry = (dateKEY *) DatumGetPointer(((GISTENTRY *) PG_GETARG_POINTER(0))->key);
	dateKEY    *newentry = (dateKEY *) DatumGetPointer(((GISTENTRY *) PG_GETARG_POINTER(1))->key);
	float	   *result = (float *) PG_GETARG_POINTER(2);
	int32		diff,
				res;

	diff = DatumGetInt32(DirectFunctionCall2(date_mi,
											 DateADTGetDatum(newentry->upper),
											 DateADTGetDatum(origentry->upper)));

	res = Max(diff, 0);

	diff = DatumGetInt32(DirectFunctionCall2(date_mi,
											 DateADTGetDatum(origentry->lower),
											 DateADTGetDatum(newentry->lower)));

	res += Max(diff, 0);

	*result = 0.0;

	if (res > 0)
	{
		diff = DatumGetInt32(DirectFunctionCall2(date_mi,
												 DateADTGetDatum(origentry->upper),
												 DateADTGetDatum(origentry->lower)));
		*result += FLT_MIN;
		*result += (float) (res / ((double) (res + diff)));
		*result *= (FLT_MAX / (((GISTENTRY *) PG_GETARG_POINTER(0))->rel->rd_att->natts + 1));
	}

	PG_RETURN_POINTER(result);
}
