// AUTO-DRAFT from postgres/postgres PR #5dce44589444704a917b2d2f70c99d9ae357da73
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
static bool
mark_file_as_archived(StreamCtl *stream, const char *fname)
{
	Walfile    *f;
	static char tmppath[MAXPGPATH];

	snprintf(tmppath, sizeof(tmppath), "archive_status/%s.done",
			 fname);

	f = stream->walmethod->ops->open_for_write(stream->walmethod, tmppath,
											   NULL, 0);
	if (f == NULL)
	{
		pg_log_error("could not create archive status file \"%s\": %s",
					 tmppath, GetLastWalMethodError(stream->walmethod));
		return false;
	}

	if (stream->walmethod->ops->close(f, CLOSE_NORMAL) != 0)
	{
		pg_log_error("could not close archive status file \"%s\": %s",
					 tmppath, GetLastWalMethodError(stream->walmethod));
		return false;
	}

	return true;
}
/* …（同文件无关代码省略）… */
static bool
writeTimeLineHistoryFile(StreamCtl *stream, const char *filename, const char *content)
{
	size_t		size = strlen(content);
	char		histfname[MAXFNAMELEN];
	Walfile    *f;

	/*
	 * Check that the server's idea of how timeline history files should be
	 * named matches ours.
	 */
	TLHistoryFileName(histfname, stream->timeline);
	if (strcmp(histfname, filename) != 0)
	{
		pg_log_error("server reported unexpected history file name for timeline %u: %s",
					 stream->timeline, filename);
		return false;
	}

	f = stream->walmethod->ops->open_for_write(stream->walmethod,
											   histfname, ".tmp", 0);
	if (f == NULL)
	{
		pg_log_error("could not create timeline history file \"%s\": %s",
					 histfname, GetLastWalMethodError(stream->walmethod));
		return false;
	}

	if (stream->walmethod->ops->write(f, content, size) != size)
	{
		pg_log_error("could not write timeline history file \"%s\": %s",
					 histfname, GetLastWalMethodError(stream->walmethod));

		/*
		 * If we fail to make the file, delete it to release disk space
		 */
		stream->walmethod->ops->close(f, CLOSE_UNLINK);

		return false;
	}

	if (stream->walmethod->ops->close(f, CLOSE_NORMAL) != 0)
	{
		pg_log_error("could not close file \"%s\": %s",
					 histfname, GetLastWalMethodError(stream->walmethod));
		return false;
	}

	/* Maintain archive_status, check close_walfile() for details. */
	if (stream->mark_done)
	{
		/* writes error message if failed */
		if (!mark_file_as_archived(stream, histfname))
			return false;
	}

	return true;
}
/* …（同文件无关代码省略）… */
			}

			/* Write the history file to disk */
			writeTimeLineHistoryFile(stream,
									 PQgetvalue(res, 0, 0),
									 PQgetvalue(res, 0, 1));

			PQclear(res);
		}
