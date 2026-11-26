// AUTO-DRAFT from postgres/postgres PR #3b249c728fd126c684be77ae834cb7eaa38d0962
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#define MAXINVALMSGS 32
/* …（同文件无关代码省略）… */
	} while (nummsgs == MAXINVALMSGS);

	/*
	 * We are now caught up.  If we received a catchup signal, reset that
	 * flag, and call SICleanupQueue().  This is not so much because we need
	 * to flush dead messages right now, as that we want to pass on the
	 * catchup signal to the next slowest backend.  "Daisy chaining" the
	 * catchup signal this way avoids creating spikes in system load for what
	 * should be just a background maintenance activity.
	 */
	if (catchupInterruptPending)
	{
		catchupInterruptPending = false;
		elog(DEBUG4, "sinval catchup complete, cleaning queue");
		SICleanupQueue(false, 0);
	}
