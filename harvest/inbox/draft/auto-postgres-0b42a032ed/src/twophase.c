// AUTO-DRAFT from postgres/postgres PR #a5ccf20801c821b72bb04d56b7320d52149a1ce5
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#include "access/xlogutils.h"
#include "catalog/pg_type.h"
#include "catalog/storage.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "pg_trace.h"
/* …（同文件无关代码省略）… */
#define TWOPHASE_DIR "pg_twophase"
/* …（同文件无关代码省略）… */
static inline int
TwoPhaseFilePath(char *path, FullTransactionId fxid)
{
	return snprintf(path, MAXPGPATH, TWOPHASE_DIR "/%08X%08X",
					EpochFromFullTransactionId(fxid),
					XidFromFullTransactionId(fxid));
}
/* …（同文件无关代码省略）… */
RecreateTwoPhaseFile(FullTransactionId fxid, const void *content, size_t len)
{
	char		path[MAXPGPATH];
	pg_crc32c	statefile_crc;
	int			fd;

/* …（同文件无关代码省略）… */

	TwoPhaseFilePath(path, fxid);

	fd = OpenTransientFile(path,
						   O_CREAT | O_TRUNC | O_WRONLY | PG_BINARY);
	if (fd < 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not recreate file \"%s\": %m", path)));

	/* Write content and CRC */
	errno = 0;
/* …（同文件无关代码省略）… */
			errno = ENOSPC;
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not write file \"%s\": %m", path)));
	}
	if (write(fd, &statefile_crc, sizeof(pg_crc32c)) != sizeof(pg_crc32c))
	{
/* …（同文件无关代码省略）… */
			errno = ENOSPC;
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not write file \"%s\": %m", path)));
	}
	pgstat_report_wait_end();

/* …（同文件无关代码省略）… */
	if (pg_fsync(fd) != 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not fsync file \"%s\": %m", path)));
	pgstat_report_wait_end();

	if (CloseTransientFile(fd) != 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not close file \"%s\": %m", path)));
}

/*
/* …（同文件无关代码省略）… */
	cldir = AllocateDir(TWOPHASE_DIR);
	while ((clde = ReadDir(cldir, TWOPHASE_DIR)) != NULL)
	{
		if (strlen(clde->d_name) == 16 &&
			strspn(clde->d_name, "0123456789ABCDEF") == 16)
		{
