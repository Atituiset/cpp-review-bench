// AUTO-DRAFT from postgres/postgres PR #ba6d9ee259cdb2a6c862a4741b16194106e00a9a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdio.h>
#include <time.h>
  // <<< BUG ANCHOR
#include <sys/time.h>

#include "common/connect.h"
#include "funcapi.h"
#include "libpq-fe.h"
#include "libpq/libpq-be-fe-helpers.h"
/* …（同文件无关代码省略）… */
	/* Column 2 is the server's current WAL flush position */
	if (server_lsn)
	{
		uint32		hi,
					lo;

		if (sscanf(PQgetvalue(res, 0, 2), "%X/%X", &hi, &lo) != 2)
			ereport(ERROR,
					(errcode(ERRCODE_PROTOCOL_VIOLATION),
					 errmsg("could not parse WAL location \"%s\"",
							PQgetvalue(res, 0, 2))));
		*server_lsn = ((uint64) hi) << 32 | lo;
	}

	PQclear(res);
