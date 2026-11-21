// AUTO-DRAFT from postgres/postgres PR #32dd742b075e999fa358570bc13778b93f8025d4
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdio.h>
#include <stdlib.h>
  // <<< BUG ANCHOR

#include "common/connect.h"
#include "fe_utils/string_utils.h"
#include "libpq/pqcomm.h"
#include "pg_upgrade.h"

/* …（同文件无关代码省略）… */
PGresult *
executeQueryOrDie(PGconn *conn, const char *fmt, ...)
{
	static char query[QUERY_ALLOC];
	va_list		args;
	PGresult   *result;
	ExecStatusType status;

	va_start(args, fmt);
	vsnprintf(query, sizeof(query), fmt, args);
	va_end(args);

	pg_log(PG_VERBOSE, "executing: %s", query);
	result = PQexec(conn, query);
	status = PQresultStatus(result);

	if ((status != PGRES_TUPLES_OK) && (status != PGRES_COMMAND_OK))
	{
		pg_log(PG_REPORT, "SQL command failed\n%s\n%s", query,
			   PQerrorMessage(conn));
		PQclear(result);
		PQfinish(conn);
		printf(_("Failure, exiting\n"));
		exit(1);
	}
	else
		return result;
}

