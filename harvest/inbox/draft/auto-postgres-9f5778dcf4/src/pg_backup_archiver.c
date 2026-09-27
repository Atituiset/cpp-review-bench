// AUTO-DRAFT from postgres/postgres PR #3c5d9d914fa5b8fb3f371dd97bdece032ca3598d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
int
ahprintf(ArchiveHandle *AH, const char *fmt, ...)
{
	int			save_errno = errno;
	char	   *p;
	size_t		len = 128;		/* initial assumption about buffer size */
	size_t		cnt;

	for (;;)
	{
		va_list		args;

		/* Allocate work buffer. */
		p = (char *) pg_malloc(len);

		/* Try to format the data. */
		errno = save_errno;
		va_start(args, fmt);
		cnt = pvsnprintf(p, len, fmt, args);
		va_end(args);

		if (cnt < len)
			break;				/* success */

		/* Release buffer and loop around to try again with larger len. */
		pg_free(p);
		len = cnt;
	}

	ahwrite(p, 1, cnt, AH);
	pg_free(p);
	return (int) cnt;
}
/* …（同文件无关代码省略）… */
static int
RestoringToDB(ArchiveHandle *AH)
{
	RestoreOptions *ropt = AH->public.ropt;

	return (ropt && ropt->useDB && AH->connection);
}
/* …（同文件无关代码省略）… */
static void
dump_lo_buf(ArchiveHandle *AH)
{
	if (AH->connection)
	{
		int			res;

		res = lo_write(AH->connection, AH->loFd, AH->lo_buf, AH->lo_buf_used);
		pg_log_debug(ngettext("wrote %zu byte of large object data (result = %d)",
							  "wrote %zu bytes of large object data (result = %d)",
							  AH->lo_buf_used),
					 AH->lo_buf_used, res);
		/* We assume there are no short writes, only errors */
		if (res != AH->lo_buf_used)
			warn_or_exit_horribly(AH, "could not write to large object: %s",
								  PQerrorMessage(AH->connection));
	}
	else
	{
		PQExpBuffer buf = createPQExpBuffer();

		appendByteaLiteralAHX(buf,
							  (const unsigned char *) AH->lo_buf,
							  AH->lo_buf_used,
							  AH);

		/* Hack: turn off writingLO so ahwrite doesn't recurse to here */
		AH->writingLO = false;
		ahprintf(AH, "SELECT pg_catalog.lowrite(0, %s);\n", buf->data);
		AH->writingLO = true;

		destroyPQExpBuffer(buf);
	}
	AH->lo_buf_used = 0;
}
/* …（同文件无关代码省略）… */
void
ahwrite(const void *ptr, size_t size, size_t nmemb, ArchiveHandle *AH)
{
	int			bytes_written = 0;

	if (AH->writingLO)
	{
		size_t		remaining = size * nmemb;

		while (AH->lo_buf_used + remaining > AH->lo_buf_size)
		{
			size_t		avail = AH->lo_buf_size - AH->lo_buf_used;

			memcpy((char *) AH->lo_buf + AH->lo_buf_used, ptr, avail);
			ptr = (const char *) ptr + avail;
			remaining -= avail;
			AH->lo_buf_used += avail;
			dump_lo_buf(AH);
		}

		memcpy((char *) AH->lo_buf + AH->lo_buf_used, ptr, remaining);
		AH->lo_buf_used += remaining;

		bytes_written = size * nmemb;
	}
	else if (AH->CustomOutPtr)
		bytes_written = AH->CustomOutPtr(AH, ptr, size * nmemb);

	/*
	 * If we're doing a restore, and it's direct to DB, and we're connected
	 * then send it to the DB.
	 */
	else if (RestoringToDB(AH))
		bytes_written = ExecuteSqlCommandBuf(&AH->public, (const char *) ptr, size * nmemb);
	else
	{
		CompressFileHandle *CFH = (CompressFileHandle *) AH->OF;

		CFH->write_func(ptr, size * nmemb, CFH);
		bytes_written = size * nmemb;
	}

	if (bytes_written != size * nmemb)
		WRITE_ERROR_EXIT;
}
/* …（同文件无关代码省略）… */
void
warn_or_exit_horribly(ArchiveHandle *AH, const char *fmt, ...)
{
	/* Stay quiet if this is a result of our own cancellation. */
	if (!is_cancel_in_progress())
	{
		va_list		ap;

		switch (AH->stage)
		{

			case STAGE_NONE:
				/* Do nothing special */
				break;

			case STAGE_INITIALIZING:
				if (AH->stage != AH->lastErrorStage)
					pg_log_info("while INITIALIZING:");
				break;

			case STAGE_PROCESSING:
				if (AH->stage != AH->lastErrorStage)
					pg_log_info("while PROCESSING TOC:");
				break;

			case STAGE_FINALIZING:
				if (AH->stage != AH->lastErrorStage)
					pg_log_info("while FINALIZING:");
				break;
		}
		if (AH->currentTE != NULL && AH->currentTE != AH->lastErrorTE)
		{
			pg_log_info("from TOC entry %d; %u %u %s %s %s",
						AH->currentTE->dumpId,
						AH->currentTE->catalogId.tableoid,
						AH->currentTE->catalogId.oid,
						AH->currentTE->desc ? AH->currentTE->desc : "(no desc)",
						AH->currentTE->tag ? AH->currentTE->tag : "(no tag)",
						AH->currentTE->owner ? AH->currentTE->owner : "(no owner)");
		}

		va_start(ap, fmt);
		pg_log_generic_v(PG_LOG_ERROR, PG_LOG_PRIMARY, fmt, ap);
		va_end(ap);
	}

	AH->lastErrorStage = AH->stage;
	AH->lastErrorTE = AH->currentTE;

	if (AH->public.exit_on_error)
		exit_nicely(1);
	else
		AH->public.n_errors++;
}
/* …（同文件无关代码省略）… */
		 * Anything added between this line and the following \restrict must
		 * be careful to avoid any possible meta-command injection vectors.
		 */
		ahprintf(AH, "\\unrestrict %s\n", ropt->restrict_key);

		initPQExpBuffer(&connectbuf);
		appendPsqlMetaConnect(&connectbuf, dbname);
		ahprintf(AH, "%s", connectbuf.data);
		termPQExpBuffer(&connectbuf);

		ahprintf(AH, "\\restrict %s\n\n", ropt->restrict_key);
	}

	/*
