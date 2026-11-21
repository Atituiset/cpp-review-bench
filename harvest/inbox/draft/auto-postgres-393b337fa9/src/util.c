// AUTO-DRAFT from postgres/postgres PR #32dd742b075e999fa358570bc13778b93f8025d4
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdio.h>
#include <string.h>
  // <<< BUG ANCHOR
#include <signal.h>

#include "common/username.h"
#include "pg_upgrade.h"

LogOpts		log_opts;
/* …（同文件无关代码省略）… */
static void
pg_log_v(eLogType type, const char *fmt, va_list ap)
{
	char		message[QUERY_ALLOC];

	/* No incoming message should end in newline; we add that here. */
	Assert(fmt);
	Assert(fmt[0] == '\0' || fmt[strlen(fmt) - 1] != '\n');

	vsnprintf(message, sizeof(message), _(fmt), ap);

	/* PG_VERBOSE and PG_STATUS are only output in verbose mode */
	/* fopen() on log_opts.internal might have failed, so check it */
/* …（同文件无关代码省略）… */

			/* No default:, we want a warning for omitted cases */
	}
	fflush(stdout);
}
