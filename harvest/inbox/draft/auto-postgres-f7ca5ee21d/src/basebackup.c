// AUTO-DRAFT from postgres/postgres PR #c97931c16d89c848696d2110a0f4831e4d6d1b69
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
				char	   *backup_label;
				ControlFileData *controlData;
				bool		crc_ok;
				char		controlFile[PG_CONTROL_FILE_SIZE];

				bbsink_begin_archive(sink, "base.tar");

/* …（同文件无关代码省略）… */
							(errmsg("calculated CRC checksum does not match value stored in file")));

				/* Send the full file, zero-padded as it is on disk */
				memset(controlFile, 0, PG_CONTROL_FILE_SIZE);
				memcpy(controlFile, controlData, sizeof(ControlFileData));
				pfree(controlData);

				sendFileWithContent(sink, XLOG_CONTROL_FILE,
									controlFile, PG_CONTROL_FILE_SIZE,
									&manifest);
			}
			else
			{
/* …（同文件无关代码省略）… */
static void
sendFileWithContent(bbsink *sink, const char *filename, const char *content,
					int len, backup_manifest_info *manifest)
{
	struct stat statbuf;
	int			bytes_done = 0;
	pg_checksum_context checksum_ctx;

	if (pg_checksum_init(&checksum_ctx, manifest->checksum_type) < 0)
		elog(ERROR, "could not initialize checksum of file \"%s\"",
			 filename);

	if (len < 0)
		len = strlen(content);

	/*
	 * Construct a stat struct for the file we're injecting in the tar.
	 */

	/* Windows doesn't have the concept of uid and gid */
#ifdef WIN32
	statbuf.st_uid = 0;
	statbuf.st_gid = 0;
#else
	statbuf.st_uid = geteuid();
	statbuf.st_gid = getegid();
#endif
	statbuf.st_mtime = time(NULL);
	statbuf.st_mode = pg_file_create_mode;
	statbuf.st_size = len;

	_tarWriteHeader(sink, filename, NULL, &statbuf, false);

	if (pg_checksum_update(&checksum_ctx, (const uint8 *) content, len) < 0)
		elog(ERROR, "could not update checksum of file \"%s\"",
			 filename);

	while (bytes_done < len)
	{
		size_t		remaining = len - bytes_done;
		size_t		nbytes = Min(sink->bbs_buffer_length, remaining);

		memcpy(sink->bbs_buffer, content, nbytes);
		bbsink_archive_contents(sink, nbytes);
		bytes_done += nbytes;
		content += nbytes;
	}

	_tarWritePadding(sink, len);

	AddFileToBackupManifest(manifest, InvalidOid, filename, len,
							(pg_time_t) statbuf.st_mtime, &checksum_ctx);
}
/* …（同文件无关代码省略）… */
static int64
_tarWriteHeader(bbsink *sink, const char *filename, const char *linktarget,
				struct stat *statbuf, bool sizeonly)
{
	enum tarError rc;

	if (!sizeonly)
	{
		/*
		 * As of this writing, the smallest supported block size is 1kB, which
		 * is twice TAR_BLOCK_SIZE. Since the buffer size is required to be a
		 * multiple of BLCKSZ, it should be safe to assume that the buffer is
		 * large enough to fit an entire tar block. We double-check by means
		 * of these assertions.
		 */
		StaticAssertDecl(TAR_BLOCK_SIZE <= BLCKSZ,
						 "BLCKSZ too small for tar block");
		Assert(sink->bbs_buffer_length >= TAR_BLOCK_SIZE);

		rc = tarCreateHeader(sink->bbs_buffer, filename, linktarget,
							 statbuf->st_size, statbuf->st_mode,
							 statbuf->st_uid, statbuf->st_gid,
							 statbuf->st_mtime);

		switch (rc)
		{
			case TAR_OK:
				break;
			case TAR_NAME_TOO_LONG:
				ereport(ERROR,
						(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
						 errmsg("file name too long for tar format: \"%s\"",
								filename)));
				break;
			case TAR_SYMLINK_TOO_LONG:
				ereport(ERROR,
						(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
						 errmsg("symbolic link target too long for tar format: "
								"file name \"%s\", target \"%s\"",
								filename, linktarget)));
				break;
			default:
				elog(ERROR, "unrecognized tar error: %d", rc);
		}

		bbsink_archive_contents(sink, TAR_BLOCK_SIZE);
	}

	return TAR_BLOCK_SIZE;
}
/* …（同文件无关代码省略）… */
static void
_tarWritePadding(bbsink *sink, int len)
{
	int			pad = tarPaddingBytesRequired(len);

	/*
	 * As in _tarWriteHeader, it should be safe to assume that the buffer is
	 * large enough that we don't need to do this in multiple chunks.
	 */
	Assert(sink->bbs_buffer_length >= TAR_BLOCK_SIZE);
	Assert(pad <= TAR_BLOCK_SIZE);

	if (pad > 0)
	{
		MemSet(sink->bbs_buffer, 0, pad);
		bbsink_archive_contents(sink, pad);
	}
}
