// AUTO-DRAFT from postgres/postgres PR #e8f4f9e3ce2a7a17790972e04631aa999b72abfd
	 * Error-out if explicitly required one of the TABLESAMPLE methods, but
	 * the server does not support it.
	 */
	if ((server_version_num < 95000) &&  // <<< BUG ANCHOR
		(method == ANALYZE_SAMPLE_SYSTEM ||
		 method == ANALYZE_SAMPLE_BERNOULLI))
		ereport(ERROR,
/* …（同文件无关代码省略）… */
	 */
	if (method == ANALYZE_SAMPLE_AUTO)
	{
		if (server_version_num < 95000)
			method = ANALYZE_SAMPLE_RANDOM;
		else
			method = ANALYZE_SAMPLE_BERNOULLI;
