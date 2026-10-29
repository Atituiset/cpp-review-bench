// AUTO-DRAFT from postgres/postgres PR #27caf4ca5ae5ecfe66822d5a9c5803e0d1bc2ed0
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
		/*
		 * If needs_backup is true or WAL checking is enabled for current
		 * resource manager, log a full-page write for the current block.
		 */
		include_image = needs_backup || (info & XLR_CHECK_CONSISTENCY) != 0;

		if (include_image)
		{
