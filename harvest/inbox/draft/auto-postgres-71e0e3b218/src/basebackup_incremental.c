// AUTO-DRAFT from postgres/postgres PR #cedd2cec7d537ccae01ab5041cdcaf15aa8618e0
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

char *
GetIncrementalFilePath(Oid dboid, Oid spcoid, RelFileNumber relfilenumber,
					   ForkNumber forknum, unsigned segno)
{
	RelPathStr	path;
	char	   *lastslash;
	char	   *ipath;

	path = GetRelationPath(dboid, spcoid, relfilenumber, INVALID_PROC_NUMBER,
						   forknum);

	lastslash = strrchr(path.str, '/');
	Assert(lastslash != NULL);
	*lastslash = '\0';

	if (segno > 0)
		ipath = psprintf("%s/INCREMENTAL.%s.%u", path.str, lastslash + 1, segno);
	else
		ipath = psprintf("%s/INCREMENTAL.%s", path.str, lastslash + 1);

	return ipath;
}
/* …（同文件无关代码省略）… */
		ipath = GetIncrementalFilePath(dboid, spcoid, relfilenumber,
									   forknum, segno);
		if (backup_file_lookup(ib->manifest_files, ipath) == NULL)
			return BACK_UP_FILE_FULLY;
	}

	/*
