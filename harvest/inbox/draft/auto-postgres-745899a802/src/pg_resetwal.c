// AUTO-DRAFT from postgres/postgres PR #1f7013fe089d0e0db5b5e4a20d84acecf0f128a1
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
					pg_log_error_hint("Try \"%s --help\" for more information.", progname);
					exit(1);
				}
				newest_commit_ts_xid_val = strtoul(endptr + 1, &endptr2, 0);
				if (endptr2 == endptr + 1 || *endptr2 != '\0' || errno != 0)
				{
					pg_log_error("invalid argument for option %s", "-c");
/* …（同文件无关代码省略）… */

	if (commit_ts_xids_given)
	{
		ControlFile.checkPointCopy.oldestCommitTsXid = oldest_commit_ts_xid_val;
		ControlFile.checkPointCopy.newestCommitTsXid = newest_commit_ts_xid_val;
	}

	if (next_oid_given)
