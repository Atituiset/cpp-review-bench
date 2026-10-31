// AUTO-DRAFT from postgres/postgres PR #5e4966e9d23931e49f914770e663dbc316ed568f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
		/* see equivalent call for relations above */
		XLogFlush(lsn);

		pgstat_execute_transactional_drops(parsed->nstats, parsed->stats, true);
	}

	/*
/* …（同文件无关代码省略）… */
		/* see equivalent call for relations above */
		XLogFlush(lsn);

		pgstat_execute_transactional_drops(parsed->nstats, parsed->stats, true);
	}
}
