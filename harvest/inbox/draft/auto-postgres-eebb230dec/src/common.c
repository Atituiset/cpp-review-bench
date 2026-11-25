// AUTO-DRAFT from postgres/postgres PR #14644e123839e5fe2d63797c9c0d16626ff9c420
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

	 */
	if (wordlen == 6 && pg_strncasecmp(query, "vacuum", 6) == 0)
		return true;
	if (wordlen == 7 && pg_strncasecmp(query, "cluster", 7) == 0)
	{
		/* CLUSTER with any arguments is allowed in transactions */
