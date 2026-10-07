// AUTO-DRAFT from postgres/postgres PR #cc33773482e879ea746256cc344ab029df1243d9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
 * The message is sent if 'force' is set, if enough time has passed since the
 * last update to reach wal_receiver_status_interval, or if WAL locations have
 * advanced since the previous status update. If wal_receiver_status_interval
 * is disabled and 'force' is false, this function does nothing. Set 'force' to
 * send the message unconditionally.
 *
 * Whether WAL locations are considered "advanced" depends on 'checkApply'.
 * If 'checkApply' is false, only the write and flush locations are checked.
/* …（同文件无关代码省略）… */
	TimestampTz now;

	/*
	 * If the user doesn't want status to be reported to the primary, be sure
	 * to exit before doing anything at all.
	 */
	if (!force && wal_receiver_status_interval <= 0)
		return;

	/* Get current timestamp. */
