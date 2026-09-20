// AUTO-DRAFT from postgres/postgres PR #8e483af5515ec4ee90d17a3864f5bb764e4e9c47
int4um(PG_FUNCTION_ARGS)
{
	int32		arg = PG_GETARG_INT32(0);
  // <<< BUG ANCHOR
	if (unlikely(arg == PG_INT32_MIN))
		ereport(ERROR,
				(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
				 errmsg("integer out of range")));
	PG_RETURN_INT32(-arg);
}

Datum
/* …（同文件无关代码省略）… */
	 */
	if (arg2 == -1)
	{
		if (unlikely(arg1 == PG_INT32_MIN))
			ereport(ERROR,
					(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					 errmsg("integer out of range")));
		result = -arg1;
		PG_RETURN_INT32(result);
	}

/* …（同文件无关代码省略）… */
	 */
	if (arg2 == -1)
	{
		if (unlikely(arg1 == PG_INT32_MIN))
			ereport(ERROR,
					(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					 errmsg("integer out of range")));
		result = -arg1;
		PG_RETURN_INT32(result);
	}
