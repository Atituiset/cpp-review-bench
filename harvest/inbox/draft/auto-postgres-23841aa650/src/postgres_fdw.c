// AUTO-DRAFT from postgres/postgres PR #7d47e41238004b6b8bce076d4bb76d858257b58d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

	}

	/*
	 * Get connection to the foreign server.  Connection manager will
	 * establish new connection if necessary.
	 *
	 * Note that unlike the sampling case, we only query pg_class and
	 * pg_stats, so we do the remote access as the current user.
	 */
	user = GetUserMapping(GetUserId(), table->serverid);
	conn = GetConnection(user, false, NULL);
