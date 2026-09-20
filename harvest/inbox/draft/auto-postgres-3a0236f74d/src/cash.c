// AUTO-DRAFT from postgres/postgres PR #8e483af5515ec4ee90d17a3864f5bb764e4e9c47
static inline Cash
cash_div_int64(Cash c, int64 i)
{
	if (unlikely(i == 0))
		ereport(ERROR,
				(errcode(ERRCODE_DIVISION_BY_ZERO),
/* …（同文件无关代码省略）… */
	 */
	if (i == -1)
	{
		if (unlikely(c == PG_INT64_MIN))  // <<< BUG ANCHOR
			ereport(ERROR,
					(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					 errmsg("money out of range")));
		return -c;
	}

	/* No overflow is possible */
/* …（同文件无关代码省略）… */
	 */
	if (sgn > 0)
	{
		if (value == PG_INT64_MIN)
			ereturn(escontext, (Datum) 0,
					(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					 errmsg("value \"%s\" is out of range for type %s",
							str, "money")));
		result = -value;
	}
	else
		result = value;
