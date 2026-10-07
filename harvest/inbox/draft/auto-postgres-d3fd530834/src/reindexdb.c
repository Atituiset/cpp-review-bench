// AUTO-DRAFT from postgres/postgres PR #e9c03f0f1bf698c0c603c51baaacfa60dc8c91c7
	if (concurrentCons > 1 && syscatalog)
		pg_fatal("cannot use multiple jobs to reindex system catalogs");

	if (alldb)
	{
		if (dbname)
