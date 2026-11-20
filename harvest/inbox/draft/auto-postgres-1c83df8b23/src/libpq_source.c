// AUTO-DRAFT from postgres/postgres PR #4a26e36f87f6f30a93bfe1b95264b4b12d6b9ac3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#define MAX_CHUNK_SIZE (1024 * 1024)
#define MAX_CHUNKS_PER_QUERY 1000
/* …（同文件无关代码省略）… */
typedef struct
{
	const char *path;			/* path relative to data directory root */
	off_t		offset;
	size_t		length;
} fetch_range_request;
/* …（同文件无关代码省略）… */
typedef struct
{
	rewind_source common;		/* common interface functions */

	PGconn	   *conn;

	/*
	 * Queue of chunks that have been requested with the queue_fetch_range()
	 * function, but have not been fetched from the remote server yet.
	 */
	int			num_requests;
	fetch_range_request request_queue[MAX_CHUNKS_PER_QUERY];

	/* temporary space for process_queued_fetch_requests() */
	StringInfoData paths;
	StringInfoData offsets;
	StringInfoData lengths;
} libpq_source;
/* …（同文件无关代码省略）… */
static void libpq_finish_fetch(rewind_source *source);
static char *libpq_fetch_file(rewind_source *source, const char *path,
							  size_t *filesize);
static XLogRecPtr libpq_get_current_wal_insert_lsn(rewind_source *source);
static void libpq_destroy(rewind_source *source);

/*
/* …（同文件无关代码省略）… */
	src->common.queue_fetch_file = libpq_queue_fetch_file;
	src->common.queue_fetch_range = libpq_queue_fetch_range;
	src->common.finish_fetch = libpq_finish_fetch;
	src->common.get_current_wal_insert_lsn = libpq_get_current_wal_insert_lsn;
	src->common.destroy = libpq_destroy;

	src->conn = conn;
/* …（同文件无关代码省略）… */
static char *
run_simple_query(PGconn *conn, const char *sql)
{
	PGresult   *res;
	char	   *result;

	res = PQexec(conn, sql);

	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		pg_fatal("error running query (%s) on source server: %s",
				 sql, PQresultErrorMessage(res));

	/* sanity check the result set */
	if (PQnfields(res) != 1 || PQntuples(res) != 1 || PQgetisnull(res, 0, 0))
		pg_fatal("unexpected result set from query");

	result = pg_strdup(PQgetvalue(res, 0, 0));

	PQclear(res);

	return result;
}
/* …（同文件无关代码省略）… */
}

/*
 * Call the pg_current_wal_insert_lsn() function in the remote system.
 */
static XLogRecPtr
libpq_get_current_wal_insert_lsn(rewind_source *source)
{
	PGconn	   *conn = ((libpq_source *) source)->conn;
	XLogRecPtr	result;
	char	   *val;

	val = run_simple_query(conn, "SELECT pg_current_wal_insert_lsn()");

	if (!pg_parse_lsn(val, &result))
		pg_fatal("unrecognized result \"%s\" for current WAL insert location", val);

	pg_free(val);

/* …（同文件无关代码省略）… */
static void
libpq_queue_fetch_file(rewind_source *source, const char *path, size_t len)
{
	/*
	 * Truncate the target file immediately, and queue a request to fetch it
	 * from the source. If the file is small, smaller than MAX_CHUNK_SIZE,
	 * request fetching a full-sized chunk anyway, so that if the file has
	 * become larger in the source system, after we scanned the source
	 * directory, we still fetch the whole file. This only works for files up
	 * to MAX_CHUNK_SIZE, but that's good enough for small configuration files
	 * and such that are changed every now and then, but not WAL-logged. For
	 * larger files, we fetch up to the original size.
	 *
	 * Even with that mechanism, there is an inherent race condition if the
	 * file is modified at the same instant that we're copying it, so that we
	 * might copy a torn version of the file with one half from the old
	 * version and another half from the new. But pg_basebackup has the same
	 * problem, and it hasn't been a problem in practice.
	 *
	 * It might seem more natural to truncate the file later, when we receive
	 * it from the source server, but then we'd need to track which
	 * fetch-requests are for a whole file.
	 */
	open_target_file(path, true);
	libpq_queue_fetch_range(source, path, 0, Max(len, MAX_CHUNK_SIZE));
}
/* …（同文件无关代码省略）… */
static void
libpq_queue_fetch_range(rewind_source *source, const char *path, off_t off,
						size_t len)
{
	libpq_source *src = (libpq_source *) source;

	/*
	 * Does this request happen to be a continuation of the previous chunk? If
	 * so, merge it with the previous one.
	 *
	 * XXX: We use pointer equality to compare the path. That's good enough
	 * for our purposes; the caller always passes the same pointer for the
	 * same filename. If it didn't, we would fail to merge requests, but it
	 * wouldn't affect correctness.
	 */
	if (src->num_requests > 0)
	{
		fetch_range_request *prev = &src->request_queue[src->num_requests - 1];

		if (prev->offset + prev->length == off &&
			prev->length < MAX_CHUNK_SIZE &&
			prev->path == path)
		{
			/*
			 * Extend the previous request to cover as much of this new
			 * request as possible, without exceeding MAX_CHUNK_SIZE.
			 */
			size_t		thislen;

			thislen = Min(len, MAX_CHUNK_SIZE - prev->length);
			prev->length += thislen;

			off += thislen;
			len -= thislen;

			/*
			 * Fall through to create new requests for any remaining 'len'
			 * that didn't fit in the previous chunk.
			 */
		}
	}

	/* Divide the request into pieces of MAX_CHUNK_SIZE bytes each */
	while (len > 0)
	{
		int32		thislen;

		/* if the queue is full, perform all the work queued up so far */
		if (src->num_requests == MAX_CHUNKS_PER_QUERY)
			process_queued_fetch_requests(src);

		thislen = Min(len, MAX_CHUNK_SIZE);
		src->request_queue[src->num_requests].path = path;
		src->request_queue[src->num_requests].offset = off;
		src->request_queue[src->num_requests].length = thislen;
		src->num_requests++;

		off += thislen;
		len -= thislen;
	}
}
/* …（同文件无关代码省略）… */
static void
libpq_finish_fetch(rewind_source *source)
{
	process_queued_fetch_requests((libpq_source *) source);
}
/* …（同文件无关代码省略）… */
static void
process_queued_fetch_requests(libpq_source *src)
{
	const char *params[3];
	PGresult   *res;
	int			chunkno;

	if (src->num_requests == 0)
		return;

	pg_log_debug("getting %d file chunks", src->num_requests);

	/*
	 * The prepared statement, 'fetch_chunks_stmt', takes three arrays with
	 * the same length as parameters: paths, offset
