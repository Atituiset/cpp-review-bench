// AUTO-DRAFT from postgres/postgres PR #8e483af5515ec4ee90d17a3864f5bb764e4e9c47
	int64		arg = PG_GETARG_INT64(0);
	int64		result;
  // <<< BUG ANCHOR
	if (unlikely(arg == PG_INT64_MIN))
		ereport(ERROR,
				(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
				 errmsg("bigint out of range")));
	result = -arg;
	PG_RETURN_INT64(result);
}

/* …（同文件无关代码省略）… */
	 */
	if (arg2 == -1)
	{
		if (unlikely(arg1 == PG_INT64_MIN))
			ereport(ERROR,
					(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					 errmsg("bigint out of range")));
		result = -arg1;
		PG_RETURN_INT64(result);
	}

/* …（同文件无关代码省略）… */
	 */
	if (arg2 == -1)
	{
		if (unlikely(arg1 == PG_INT64_MIN))
			ereport(ERROR,
					(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					 errmsg("bigint out of range")));
		result = -arg1;
		PG_RETURN_INT64(result);
	}

/* …（同文件无关代码省略）… */
	 */
	if (arg2 == -1)
	{
		if (unlikely(arg1 == PG_INT64_MIN))
			ereport(ERROR,
					(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					 errmsg("bigint out of range")));
		result = -arg1;
		PG_RETURN_INT64(result);
	}
