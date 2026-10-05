// AUTO-DRAFT from postgres/postgres PR #d6393fc40a59830a88a79bdf08e962122fa9c631
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
typedef Oid ConnCacheKey;
/* …（同文件无关代码省略）… */
typedef struct ConnCacheEntry
{
	ConnCacheKey key;			/* hash key (must be first) */
	PGconn	   *conn;			/* connection to foreign server, or NULL */
	/* Remaining fields are invalid when conn is NULL: */
	int			xact_depth;		/* 0 = no xact open, 1 = main xact open, 2 =
								 * one level of subxact open, etc */
	bool		xact_read_only; /* xact r/o state */
	bool		have_prep_stmt; /* have we prepared any stmts in this xact? */
	bool		have_error;		/* have any subxacts aborted in this xact? */
	bool		changing_xact_state;	/* xact state change in process */
	bool		parallel_commit;	/* do we commit (sub)xacts in parallel? */
	bool		parallel_abort; /* do we abort (sub)xacts in parallel? */
	bool		invalidated;	/* true if reconnect is pending */
	bool		keep_connections;	/* setting value of keep_connections
									 * server option */
	Oid			serverid;		/* foreign server OID used to get server name */
	uint32		server_hashvalue;	/* hash value of foreign server OID */
	uint32		mapping_hashvalue;	/* hash value of user mapping OID */
	PgFdwConnState state;		/* extra per-connection state */
} ConnCacheEntry;
/* …（同文件无关代码省略）… */
 */
#define RETRY_CANCEL_TIMEOUT	1000

/* Macro for constructing abort command to be sent */
#define CONSTRUCT_ABORT_COMMAND(sql, entry, toplevel) \
	do { \
/* …（同文件无关代码省略）… */
void
do_sql_command(PGconn *conn, const char *sql)
{
	do_sql_command_begin(conn, sql);
	do_sql_command_end(conn, sql, false);
}
/* …（同文件无关代码省略）… */
static void
do_sql_command_begin(PGconn *conn, const char *sql)
{
	if (!PQsendQuery(conn, sql))
		pgfdw_report_error(NULL, conn, sql);
}
/* …（同文件无关代码省略）… */
static void
do_sql_command_end(PGconn *conn, const char *sql, bool consume_input)
{
	PGresult   *res;

	/*
	 * If requested, consume whatever data is available from the socket. (Note
	 * that if all data is available, this allows pgfdw_get_result to call
	 * PQgetResult without forcing the overhead of WaitLatchOrSocket, which
	 * would be large compared to the overhead of PQconsumeInput.)
	 */
	if (consume_input && !PQconsumeInput(conn))
		pgfdw_report_error(NULL, conn, sql);
	res = pgfdw_get_result(conn);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
		pgfdw_report_error(res, conn, sql);
	PQclear(res);
}
/* …（同文件无关代码省略）… */
			appendStringInfoString(&sql, "REPEATABLE READ");
		if (ro)
			appendStringInfoString(&sql, " READ ONLY");
		if (XactDeferrable)
			appendStringInfoString(&sql, " DEFERRABLE");
		entry->changing_xact_state = true;
		do_sql_command(entry->conn, sql.data);
/* …（同文件无关代码省略）… */
		if (entry->xact_depth == read_only_level)
		{
			entry->changing_xact_state = true;
			do_sql_command(entry->conn, "SET transaction_read_only = on");
			entry->xact_read_only = true;
			entry->changing_xact_state = false;
		}
/* …（同文件无关代码省略）… */
		initStringInfo(&sql);
		appendStringInfo(&sql, "SAVEPOINT s%d", entry->xact_depth + 1);
		if (ro)
			appendStringInfoString(&sql, "; SET transaction_read_only = on");
		entry->changing_xact_state = true;
		do_sql_command(entry->conn, sql.data);
		entry->xact_depth++;
/* …（同文件无关代码省略）… */
PGresult *
pgfdw_get_result(PGconn *conn)
{
	return libpqsrv_get_result_last(conn, pgfdw_we_get_result);
}
/* …（同文件无关代码省略）… */
void
pgfdw_report_error(PGresult *res, PGconn *conn, const char *sql)
{
	pgfdw_report_internal(ERROR, res, conn, sql);
	pg_unreachable();
}
/* …（同文件无关代码省略）… */
static void
pgfdw_report_internal(int elevel, PGresult *res, PGconn *conn,
					  const char *sql)
{
	char	   *diag_sqlstate = PQresultErrorField(res, PG_DIAG_SQLSTATE);
	char	   *message_primary = PQresultErrorField(res, PG_DIAG_MESSAGE_PRIMARY);
	char	   *message_detail = PQresultErrorField(res, PG_DIAG_MESSAGE_DETAIL);
	char	   *message_hint = PQresultErrorField(res, PG_DIAG_MESSAGE_HINT);
	char	   *message_context = PQresultErrorField(res, PG_DIAG_CONTEXT);
	int			sqlstate;

	if (diag_sqlstate)
		sqlstate = MAKE_SQLSTATE(diag_sqlstate[0],
								 diag_sqlstate[1],
								 diag_sqlstate[2],
								 diag_sqlstate[3],
								 diag_sqlstate[4]);
	else
		sqlstate = ERRCODE_CONNECTION_FAILURE;

	/*
	 * If we don't get a message from the PGresult, try the PGconn.  This is
	 * needed because for connection-level failures, PQgetResult may just
	 * return NULL, not a PGresult at all.
	 */
	if (message_primary == NULL)
		message_primary = pchomp(PQerrorMessage(conn));

	ereport(elevel,
			(errcode(sqlstate),
			 (message_primary != NULL && message_primary[0] != '\0') ?
			 errmsg_internal("%s", message_primary) :
			 errmsg("could not obtain message string for remote error"),
			 message_detail ? errdetail_internal("%s", message_detail) : 0,
			 message_hint ? errhint("%s", message_hint) : 0,
			 message_context ? errcontext("%s", message_context) : 0,
			 sql ? errcontext("remote SQL command: %s", sql) : 0));
	PQclear(res);
}
/* …（同文件无关代码省略）… */
	if (!xact_got_connection)
		return;

	/*
	 * Scan all connection cache entries to find open remote transactions, and
	 * close them.
/* …（同文件无关代码省略）… */
		/* If it has an open remote transaction, try to close it */
		if (entry->xact_depth > 0)
		{
			elog(DEBUG3, "closing remote transaction on connection %p",
				 entry->conn);

/* …（同文件无关代码省略）… */
					pgfdw_reject_incomplete_xact_state_change(entry);

					/* Commit all remote transactions during pre-commit */
					entry->changing_xact_state = true;
					if (entry->parallel_commit)
					{
						do_sql_command_begin(entry->conn, "COMMIT TRANSACTION");
						pending_entries = lappend(pending_entries, entry);
						continue;
					}
					do_sql_command(entry->conn, "COMMIT TRANSACTION");
					entry->changing_xact_state = false;

					/*
/* …（同文件无关代码省略）… */
static void
pgfdw_reject_incomplete_xact_state_change(ConnCacheEntry *entry)
{
	ForeignServer *server;

	/* nothing to do for inactive entries and entries of sane state */
	if (entry->conn == NULL || !entry->changing_xact_state)
		
