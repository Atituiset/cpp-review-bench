// AUTO-DRAFT from postgres/postgres PR #50d6e533e4d9a0f70d798c534254007c83c0d428
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	pg_debug(...)  do { fprintf(stderr, __VA_ARGS__); } while (0)
/* …（同文件无关代码省略）… */
#define pg_fatal(...) pg_fatal_impl(__LINE__, __VA_ARGS__)
pg_noreturn static void
pg_fatal_impl(int line, const char *fmt, ...)
{
	va_list		args;

	fflush(stdout);

	fprintf(stderr, "\n%s:%d: ", progname, line);
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	Assert(fmt[strlen(fmt) - 1] != '\n');
	fprintf(stderr, "\n");
	exit(1);
}
/* …（同文件无关代码省略）… */
static PGresult *
confirm_result_status_impl(int line, PGconn *conn, ExecStatusType status)
{
	PGresult   *res;

	res = PQgetResult(conn);
	if (res == NULL)
		pg_fatal_impl(line, "PQgetResult returned null unexpectedly: %s",
					  PQerrorMessage(conn));
	if (PQresultStatus(res) != status)
		pg_fatal_impl(line, "PQgetResult returned status %s, expected %s: %s",
					  PQresStatus(PQresultStatus(res)),
					  PQresStatus(status),
					  PQerrorMessage(conn));
	return res;
}
/* …（同文件无关代码省略）… */
#define consume_query_cancel(conn) consume_query_cancel_impl(__LINE__, conn)
static void
consume_query_cancel_impl(int line, PGconn *conn)
{
	PGresult   *res;

	res = confirm_result_status_impl(line, conn, PGRES_FATAL_ERROR);
	if (strcmp(PQresultErrorField(res, PG_DIAG_SQLSTATE), "57014") != 0)
		pg_fatal_impl(line, "query failed with a different error than cancellation: %s",
					  PQerrorMessage(conn));
	PQclear(res);

	while (PQisBusy(conn))
		PQconsumeInput(conn);
}
/* …（同文件无关代码省略）… */
static void
wait_for_connection_state(int line, PGconn *monitorConn, int procpid,
						  char *state, char *event)
{
	const Oid	paramTypes[] = {INT4OID, TEXTOID};
	const char *paramValues[2];
	char	   *pidstr = psprintf("%d", procpid);

	Assert((state == NULL) ^ (event == NULL));

	paramValues[0] = pidstr;
	paramValues[1] = state ? state : event;

	while (true)
	{
		PGresult   *res;
		char	   *value;

		if (state != NULL)
			res = PQexecParams(monitorConn,
							   "SELECT count(*) FROM pg_stat_activity WHERE "
							   "pid = $1 AND state = $2",
							   2, paramTypes, paramValues, NULL, NULL, 0);
		else
			res = PQexecParams(monitorConn,
							   "SELECT count(*) FROM pg_stat_activity WHERE "
							   "pid = $1 AND wait_event = $2",
							   2, paramTypes, paramValues, NULL, NULL, 0);

		if (PQresultStatus(res) != PGRES_TUPLES_OK)
			pg_fatal_impl(line, "could not query pg_stat_activity: %s", PQerrorMessage(monitorConn));
		if (PQntuples(res) != 1)
			pg_fatal_impl(line, "unexpected number of rows received: %d", PQntuples(res));
		if (PQnfields(res) != 1)
			pg_fatal_impl(line, "unexpected number of columns received: %d", PQnfields(res));
		value = PQgetvalue(res, 0, 0);
		if (strcmp(value, "0") != 0)
		{
			PQclear(res);
			break;
		}
		PQclear(res);

		/* wait 10ms before polling again */
		pg_usleep(10000);
	}

	pfree(pidstr);
}
/* …（同文件无关代码省略）… */
#define send_cancellable_query(conn, monitorConn) \
	send_cancellable_query_impl(__LINE__, conn, monitorConn)
static void
send_cancellable_query_impl(int line, PGconn *conn, PGconn *monitorConn)
{
	const char *env_wait;
	const Oid	paramTypes[1] = {INT4OID};

	/*
	 * Wait for the connection to be idle, so that our check for an active
	 * connection below is reliable, instead of possibly seeing an outdated
	 * state.
	 */
	wait_for_connection_state(line, monitorConn, PQbackendPID(conn), "idle", NULL);

	env_wait = getenv("PG_TEST_TIMEOUT_DEFAULT");
	if (env_wait == NULL)
		env_wait = "180";

	if (PQsendQueryParams(conn, "SELECT pg_sleep($1)", 1, paramTypes,
						  &env_wait, NULL, NULL, 0) != 1)
		pg_fatal_impl(line, "failed to send query: %s", PQerrorMessage(conn));

	/*
	 * Wait for the sleep to be active, because if the query is not running
	 * yet, the cancel request that we send won't have any effect.
	 */
	wait_for_connection_state(line, monitorConn, PQbackendPID(conn), NULL, "PgSleep");
}
/* …（同文件无关代码省略）… */
static PGconn *
copy_connection(PGconn *conn)
{
	PGconn	   *copyConn;
	PQconninfoOption *opts = PQconninfo(conn);
	const char **keywords;
	const char **vals;
	int			nopts = 0;
	int			i;

	for (PQconninfoOption *opt = opts; opt->keyword != NULL; ++opt)
		nopts++;
	nopts++;					/* for the NULL terminator */

	keywords = pg_malloc_array(const char *, nopts);
	vals = pg_malloc_array(const char *, nopts);

	i = 0;
	for (PQconninfoOption *opt = opts; opt->keyword != NULL; ++opt)
	{
		if (opt->val)
		{
			keywords[i] = opt->keyword;
			vals[i] = opt->val;
			i++;
		}
	}
	keywords[i] = vals[i] = NULL;

	copyConn = PQconnectdbParams(keywords, vals, false);

	if (PQstatus(copyConn) != CONNECTION_OK)
		pg_fatal("Connection to database failed: %s",
				 PQerrorMessage(copyConn));

	pg_free(keywords);
	pg_free(vals);
	PQconninfoFree(opts);

	return copyConn;
}
/* …（同文件无关代码省略）… */
static void
test_cancel(PGconn *conn)
{
	PGcancel   *cancel;
	PGcancelConn *cancelConn;
	PGconn	   *monitorConn;
	char		errorbuf[256];

	fprintf(stderr, "test cancellations... ");

	if (PQsetnonblocking(conn, 1) != 0)
		pg_fatal("failed to set nonblocking mode: %s", PQerrorMessage(conn));

	/*
	 * Make a separate connection to the database to monitor the query on the
	 * main connection.
	 */
	monitorConn = copy_connection(conn);
	Assert(PQstatus(monitorConn) == CONNECTION_OK);

	/* test PQcancel */
	send_cancellable_query(conn, monitorConn);
	cancel = PQgetCancel(conn);
	if (!PQcancel(cancel, errorbuf, sizeof(errorbuf)))
		pg_fatal("failed to run PQcancel: %s", errorbuf);
	consume_query_cancel(conn);

	/* PGcancel object can be reused for the next query */
	send_cancellable_query(conn, monitorConn);
	if (!PQcancel(cancel, errorbuf, sizeof(errorbuf)))
		pg_fatal("failed to run PQcancel: %s", errorbuf);
	consume_query_cancel(conn);

	PQfreeCancel(cancel);

	/* test PQrequestCancel */
	send_cancellable_query(conn, monitorConn);
	if (!PQrequestCancel(conn))
		pg_fatal("failed to run 
