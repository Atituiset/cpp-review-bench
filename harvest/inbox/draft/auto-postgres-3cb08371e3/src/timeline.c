// AUTO-DRAFT from postgres/postgres PR #5ffb8c7409385c58e5c1abaa22ee77ba4c1b5c33
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdio.h>
  // <<< BUG ANCHOR
#include "access/xlog_internal.h"
#include "access/xlogarchive.h"
#include "access/xlogdefs.h"
#include "pgstat.h"
#include "storage/fd.h"
#include "utils/wait_event.h"
/* …（同文件无关代码省略）… */
		char		fline[MAXPGPATH];
		char	   *res;
		char	   *ptr;
		TimeLineID	tli;
		uint32		switchpoint_hi;
		uint32		switchpoint_lo;
		int			nfields;

		pgstat_report_wait_start(WAIT_EVENT_TIMELINE_HISTORY_READ);
		res = fgets(fline, sizeof(fline), fd);
/* …（同文件无关代码省略）… */
		if (*ptr == '\0' || *ptr == '#')
			continue;

		nfields = sscanf(fline, "%u\t%X/%08X", &tli, &switchpoint_hi, &switchpoint_lo);

		if (nfields < 1)
		{
			/* expect a numeric timeline ID as first field of line */
			ereport(FATAL,
					(errmsg("syntax error in history file: %s", fline),
					 errhint("Expected a numeric timeline ID.")));
		}
		if (nfields != 3)
			ereport(FATAL,
					(errmsg("syntax error in history file: %s", fline),
					 errhint("Expected a write-ahead log switchpoint location.")));
/* …（同文件无关代码省略）… */
		entry = palloc_object(TimeLineHistoryEntry);
		entry->tli = tli;
		entry->begin = prevend;
		entry->end = ((uint64) (switchpoint_hi)) << 32 | (uint64) switchpoint_lo;
		prevend = entry->end;

		/* Build list with newest item first */
