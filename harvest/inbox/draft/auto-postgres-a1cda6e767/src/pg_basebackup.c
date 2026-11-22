// AUTO-DRAFT from postgres/postgres PR #892cfb4cbfe0e37cc492f0e750e80cfa6394d26d
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		pg_fatal("could not initiate base backup: %s",
				 PQerrorMessage(conn));
	if (PQntuples(res) != 1)  // <<< BUG ANCHOR
		pg_fatal("server returned unexpected response to BASE_BACKUP command; got %d rows and %d fields, expected %d rows and %d fields",
				 PQntuples(res), PQnfields(res), 1, 2);

	strlcpy(xlogstart, PQgetvalue(res, 0, 0), sizeof(xlogstart));

/* …（同文件无关代码省略）… */
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		pg_fatal("could not get backup header: %s",
				 PQerrorMessage(conn));
	if (PQntuples(res) < 1)
		pg_fatal("no data returned from server");

	/*
	 * Sum up the total size, for progress reporting
/* …（同文件无关代码省略）… */
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		pg_fatal("backup failed: %s",
				 PQerrorMessage(conn));
	if (PQntuples(res) != 1)
		pg_fatal("no write-ahead log end position returned from server");
	strlcpy(xlogend, PQgetvalue(res, 0, 0), sizeof(xlogend));
	if (verbose && includewal != NO_WAL)
		pg_log_info("write-ahead log end point: %s", xlogend);
