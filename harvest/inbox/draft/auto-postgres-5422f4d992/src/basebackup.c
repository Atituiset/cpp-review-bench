// AUTO-DRAFT from postgres/postgres PR #4a92c8c29bd0068ecc747cca5d77949660b5f0a8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#include "backup/basebackup_incremental.h"
#include "backup/basebackup_sink.h"
#include "backup/basebackup_target.h"
#include "catalog/pg_tablespace_d.h"
#include "commands/defrem.h"
#include "common/compression.h"
#include "common/file_perm.h"
#include "common/file_utils.h"
#include "lib/stringinfo.h"
/* …（同文件无关代码省略）… */
#include "storage/checksum.h"
#include "storage/dsm_impl.h"
#include "storage/ipc.h"
#include "storage/reinit.h"
#include "utils/builtins.h"
#include "utils/guc.h"
/* …（同文件无关代码省略）… */

			if (ti->path == NULL)
			{
				struct stat statbuf;
				bool		sendtblspclinks = true;
				char	   *backup_label;

				bbsink_begin_archive(sink, "base.tar");

/* …（同文件无关代码省略）… */
						sendtblspclinks, &manifest, InvalidOid, ib);

				/* ... and pg_control after everything else. */
				if (lstat(XLOG_CONTROL_FILE, &statbuf) != 0)
					ereport(ERROR,
							(errcode_for_file_access(),
							 errmsg("could not stat file \"%s\": %m",
									XLOG_CONTROL_FILE)));
				sendFile(sink, XLOG_CONTROL_FILE, XLOG_CONTROL_FILE, &statbuf,
						 false, InvalidOid, InvalidOid,
						 InvalidRelFileNumber, 0, &manifest, 0, NULL, 0);
			}
			else
			{
/* …（同文件无关代码省略）… */
static bool
sendFile(bbsink *sink, const char *readfilename, const char *tarfilename,
		 const struct stat *statbuf, bool missing_ok, Oid dboid, Oid spcoid,
		 RelFileNumber relfilenumber, unsigned segno,
		 backup_manifest_info *manifest, unsigned num_incremental_blocks,
		 BlockNumber *incremental_blocks, unsigned truncation_block_length)
{
	int			fd;
	BlockNumber blkno = 0;
	int			checksum_failures = 0;
	off_t		cnt;
	pgoff_t		bytes_done = 0;
	bool		verify_checksum = false;
	pg_checksum_context checksum_ctx;
	int			ibindex = 0;
	struct stat filestat;

	if (pg_checksum_init(&checksum_ctx, manifest->checksum_type) < 0)
		elog(ERROR, "could not initialize checksum of file \"%s\"",
			 readfilename);

	fd = OpenTransientFile(readfilename, O_RDONLY | PG_BINARY);
	if (fd < 0)
	{
		if (errno == ENOENT && missing_ok)
			return false;
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open file \"%s\": %m", readfilename)));
	}

	/*
	 * Non-relation files may be replaced by a rename(), making irrelevant the
	 * caller's stat data.  Reuse fstat() on the opened file.
	 */
	filestat = *statbuf;

	if (!RelFileNumberIsValid(relfilenumber) && fstat(fd, &filestat) != 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not stat file \"%s\": %m", readfilename)));

	_tarWriteHeader(sink, tarfilename, NULL, &filestat, false);

	/*
	 * Checksums are verified in multiples of BLCKSZ, so the buffer length
	 * should be a multiple of the block size as well.
	 */
	Assert((sink->bbs_buffer_length % BLCKSZ) == 0);

	/*
	 * Verify checksums unless the client requested otherwise, but only for
	 * relation files, and only while checksums have been continuously enabled
	 * since the checkpoint this backup started from.  Checksums can still be
	 * disabled while the backup runs, thus we check at each point where we
	 * could be validating a checksum.
	 */
	if (!noverify_checksums && RelFileNumberIsValid(relfilenumber) &&
		backup_checksums_verifiable(sink->bbs_state->startptr))
		verify_checksum = true;

	/*
	 * If we're sending an incremental file, write the file header.
	 */
	if (incremental_blocks != NULL)
	{
		unsigned	magic = INCREMENTAL_MAGIC;
		size_t		header_bytes_done = 0;
		char		padding[BLCKSZ];
		size_t		paddinglen;

		/* Emit header data. */
		push_to_sink(sink, &checksum_ctx, &header_bytes_done,
					 &magic, sizeof(magic));
		push_to_sink(sink, &checksum_ctx, &header_bytes_done,
					 &num_incremental_blocks, sizeof(num_incremental_blocks));
		push_to_sink(sink, &checksum_ctx, &header_bytes_done,
					 &truncation_block_length, sizeof(truncation_block_length));
		push_to_sink(sink, &checksum_ctx, &header_bytes_done,
					 incremental_blocks,
					 sizeof(BlockNumber) * num_incremental_blocks);

		/*
		 * Add padding to align header to a multiple of BLCKSZ, but only if
		 * the incremental file has some blocks, and the alignment is actually
		 * needed (i.e. header is not already a multiple of BLCKSZ). If there
		 * are no blocks we don't want to make the file unnecessarily large,
		 * as that might make some filesystem optimizations impossible.
		 */
		if ((num_incremental_blocks > 0) && (header_bytes_done % BLCKSZ != 0))
		{
			paddinglen = (BLCKSZ - (header_bytes_done % BLCKSZ));

			memset(padding, 0, paddinglen);
			bytes_done += paddinglen;

			push_to_sink(sink, &checksum_ctx, &header_bytes_done,
						 padding, paddinglen);
		}

		/* Flush out any data still in the buffer so it's again empty. */
		if (header_bytes_done > 0)
		{
			bbsink_archive_contents(sink, header_bytes_done);
			if (pg_checksum_update(&checksum_ctx,
								   (uint8 *) sink->bbs_buffer,
								   header_bytes_done) < 0)
				elog(ERROR, "could not update checksum of base backup");
		}

		/* Update our notion of file position. */
		bytes_done += sizeof(magic);
		bytes_done += sizeof(num_incremental_blocks);
		bytes_done += sizeof(truncation_block_length);
		bytes_done += sizeof(BlockNumber) * num_incremental_blocks;
	}

	/*
	 * Loop until we read the amount of data the caller told us to expect. The
	 * file could be longer, if it was extended while we were sending it, but
	 * for a base backup we can ignore such extended data. It will be restored
	 * from WAL.
	 */
	while (1)
	{
		/*
		 * Determine whether we've read all the data that we need, and if not,
		 * read some more.
		 */
		if (incremental_blocks == NULL)
		{
			size_t		remaining = filestat.st_size - bytes_done;

			/*
			 * If we've read the required number of bytes, then it's time to
			 * stop.
			 */
			if (bytes_done >= filestat.st_size)
				break;

			/*
			 * Read as many bytes as will fit in the buffer, or however ma
