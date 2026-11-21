// AUTO-DRAFT from postgres/postgres PR #4a26e36f87f6f30a93bfe1b95264b4b12d6b9ac3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
typedef struct
{
	rewind_source common;		/* common interface functions */

	const char *datadir;		/* path to the source data directory */
} local_source;
/* …（同文件无关代码省略）… */
	src->common.queue_fetch_file = local_queue_fetch_file;
	src->common.queue_fetch_range = local_queue_fetch_range;
	src->common.finish_fetch = local_finish_fetch;
	src->common.get_current_wal_insert_lsn = NULL;
	src->common.destroy = local_destroy;

	src->datadir = datadir;
/* …（同文件无关代码省略）… */
static void
local_queue_fetch_file(rewind_source *source, const char *path, size_t len)
{
	const char *datadir = ((local_source *) source)->datadir;
	PGIOAlignedBlock buf;
	char		srcpath[MAXPGPATH];
	int			srcfd;
	size_t		written_len;

	snprintf(srcpath, sizeof(srcpath), "%s/%s", datadir, path);

	/* Open source file for reading */
	srcfd = open(srcpath, O_RDONLY | PG_BINARY, 0);
	if (srcfd < 0)
		pg_fatal("could not open source file \"%s\": %m",
				 srcpath);

	/* Truncate and open the target file for writing */
	open_target_file(path, true);

	written_len = 0;
	for (;;)
	{
		ssize_t		read_len;

		read_len = read(srcfd, buf.data, sizeof(buf));

		if (read_len < 0)
			pg_fatal("could not read file \"%s\": %m", srcpath);
		else if (read_len == 0)
			break;				/* EOF reached */

		write_target_range(buf.data, written_len, read_len);
		written_len += read_len;
	}

	/*
	 * A local source is not expected to change while we're rewinding, so
	 * check that the size of the file matches our earlier expectation.
	 */
	if (written_len != len)
		pg_fatal("size of source file \"%s\" changed concurrently: %zu bytes expected, %zu copied",
				 srcpath, len, written_len);

	if (close(srcfd) != 0)
		pg_fatal("could not close file \"%s\": %m", srcpath);
}
/* …（同文件无关代码省略）… */
static void
local_queue_fetch_range(rewind_source *source, const char *path, off_t off,
						size_t len)
{
	const char *datadir = ((local_source *) source)->datadir;
	PGIOAlignedBlock buf;
	char		srcpath[MAXPGPATH];
	int			srcfd;
	off_t		begin = off;
	off_t		end = off + len;

	snprintf(srcpath, sizeof(srcpath), "%s/%s", datadir, path);

	srcfd = open(srcpath, O_RDONLY | PG_BINARY, 0);
	if (srcfd < 0)
		pg_fatal("could not open source file \"%s\": %m",
				 srcpath);

	if (lseek(srcfd, begin, SEEK_SET) == -1)
		pg_fatal("could not seek in source file: %m");

	open_target_file(path, false);

	while (end - begin > 0)
	{
		ssize_t		readlen;
		size_t		thislen;

		if (end - begin > sizeof(buf))
			thislen = sizeof(buf);
		else
			thislen = end - begin;

		readlen = read(srcfd, buf.data, thislen);

		if (readlen < 0)
			pg_fatal("could not read file \"%s\": %m", srcpath);
		else if (readlen == 0)
			pg_fatal("unexpected EOF while reading file \"%s\"", srcpath);

		write_target_range(buf.data, begin, readlen);
		begin += readlen;
	}

	if (close(srcfd) != 0)
		pg_fatal("could not close file \"%s\": %m", srcpath);
}
/* …（同文件无关代码省略）… */
static void
local_finish_fetch(rewind_source *source)
{
	/*
	 * Nothing to do, local_queue_fetch_range() copies the ranges immediately.
	 */
}
/* …（同文件无关代码省略）… */
static void
local_destroy(rewind_source *source)
{
	pfree(source);
}
