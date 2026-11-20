// AUTO-DRAFT from postgres/postgres PR #0cb41ee8070fb0930d2ead13e19304d11d405636
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	}

error:
	if (walfile != NULL && stream->walmethod->ops->close(walfile, CLOSE_NO_RENAME) != 0)
		pg_log_error("could not close file \"%s\": %s",
					 walfile->pathname, GetLastWalMethodError(stream->walmethod));
	walfile = NULL;
	return false;
}
