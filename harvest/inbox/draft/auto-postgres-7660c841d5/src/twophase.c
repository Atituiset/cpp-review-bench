// AUTO-DRAFT from postgres/postgres PR #5e4966e9d23931e49f914770e663dbc316ed568f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
	DropRelationFiles(delrels, ndelrels, false);

	if (isCommit)
		pgstat_execute_transactional_drops(hdr->ncommitstats, commitstats, false);
	else
		pgstat_execute_transactional_drops(hdr->nabortstats, abortstats, false);

	/*
	 * Handle cache invalidation messages.
