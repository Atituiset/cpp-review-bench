// AUTO-DRAFT from postgres/postgres PR #96c99955b62994b2cb47380d84f463456cf16ee5
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
	/* Outer loop over blocks to sample */
	while (table_scan_analyze_next_block(scan, stream))
	{
		vacuum_delay_point(true);

		while (table_scan_analyze_next_tuple(scan, &liverows, &deadrows, slot))
		{
			/*
/* …（同文件无关代码省略）… */

		pgstat_progress_update_param(PROGRESS_ANALYZE_BLOCKS_DONE,
									 ++blksdone);
	}

	read_stream_end(stream);
