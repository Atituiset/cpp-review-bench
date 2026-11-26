// AUTO-DRAFT from postgres/postgres PR #3b249c728fd126c684be77ae834cb7eaa38d0962
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <time.h>

#define MAXNUMMESSAGES 4096
/* …（同文件无关代码省略）… */
#define CLEANUP_QUANTUM (MAXNUMMESSAGES / 16)
/* …（同文件无关代码省略）… */
 * backends as "reset" in the array, and sending PROCSIG_CATCHUP_INTERRUPT
 * to some backend that seems to be getting too far behind.  We signal at
 * most one backend at a time, for reasons explained at the top of the file.
 *
 * Caution: because we transiently release write lock when we have to signal
 * some other backend, it is NOT guaranteed that there are still minFree
/* …（同文件无关代码省略）… */
	else
		segP->nextThreshold = (numMsgs / CLEANUP_QUANTUM + 1) * CLEANUP_QUANTUM;

	/*
	 * Lastly, signal anyone who needs a catchup interrupt.  Since
	 * SendProcSignal() might not be fast, we don't want to hold locks while
