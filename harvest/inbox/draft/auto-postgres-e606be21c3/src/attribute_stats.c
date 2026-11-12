// AUTO-DRAFT from postgres/postgres PR #2656f664616dac41e99495b39e2bc3bc1d81b1e7
														&array_in_fn,
														PG_GETARG_DATUM(MOST_COMMON_VALS_ARG),
														atttypid, atttypmod,
														&converted);  // <<< BUG ANCHOR

		if (converted)
		{
/* …（同文件无关代码省略）… */
											&array_in_fn,
											PG_GETARG_DATUM(HISTOGRAM_BOUNDS_ARG),
											atttypid, atttypmod,
											&converted);

		if (converted)
		{
/* …（同文件无关代码省略）… */
											&array_in_fn,
											PG_GETARG_DATUM(MOST_COMMON_ELEMS_ARG),
											elemtypid, atttypmod,
											&converted);

		if (converted)
		{
/* …（同文件无关代码省略）… */
											&array_in_fn,
											PG_GETARG_DATUM(RANGE_BOUNDS_HISTOGRAM_ARG),
											bounds_typid, atttypmod,
											&converted);

		if (converted &&
			statatt_check_bounds_histogram(stavalues))
/* …（同文件无关代码省略）… */
		stavalues = statatt_build_stavalues("range_length_histogram",
											&array_in_fn,
											PG_GETARG_DATUM(RANGE_LENGTH_HISTOGRAM_ARG),
											FLOAT8OID, 0, &converted);

		if (converted)
		{
