// AUTO-DRAFT from postgres/postgres PR #51d7bf5edcaf4f8a95bb10f96b7af76d00a2f83f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
					 backup_manifest_info *manifest, Oid spcoid,
					 IncrementalBackupInfo *ib);
static bool sendFile(bbsink *sink, const char *readfilename, const char *tarfilename,
					 struct stat *statbuf, bool missing_ok,
					 Oid dboid, Oid spcoid, RelFileNumber relfilenumber,
					 unsigned segno,
					 backup_manifest_info *manifest,
/* …（同文件无关代码省略）… */
 * If the whole file is to be sent, then incremental_blocks should be NULL,
 * and num_incremental_blocks can have any value, as it will be ignored.
 *
 * Returns true if the file was successfully sent, false if 'missing_ok',
 * and the file did not exist.
 */
static bool
sendFile(bbsink *sink, const char *readfilename, const char *tarfilename,
		 struct stat *statbuf, bool missing_ok, Oid dboid, Oid spcoid,
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

	_tarWriteHeader(sink, tarfilename, NULL, statbuf, false);

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
			size_t		remaining = statbuf->st_size - bytes_done;

			/*
			 * If we've read the required number of bytes, then it's time to
			 * stop.
			 */
			if (bytes_done >= statbuf->st_size)
				break;

			/*
			 * Read as many bytes as will fit in the buffer, or however many
			 * are left to read, whichever is less.
			 */
			cnt = read_file_data_into_buffer(sink, readfilename, fd,
											 bytes_done, remaining,
											 blkno + segno * RELSEG_SIZE,
											 verify_checksum,
											 &checksum_failures);
		}
		else
		{
			BlockNumber relative_blkno;

			/*
			 * If we've read all the blocks, then it's time to stop.
			 */
			if (ibindex >= num_incremental_blocks)
				break;

			/*
			 * Read just one block, whichever one is the next that we're
			 * supposed to include.
			 */
			relative_blkno = incremental_blocks[ibindex++];
			cnt = read_file_data_into_buffer(sink, readfilename, fd,
											 relative_blkno * BLCKSZ,
											 BLCKSZ,
											 relative_blkno + segno * RELSEG_SIZE,
											 verify_checksum,
											 &checksum_failures);

			/*
			 * If we get a partial read, that must mean that the relation is
			 * being truncated. Ultimately, it should be truncated to a
