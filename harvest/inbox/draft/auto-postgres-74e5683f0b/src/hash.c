// AUTO-DRAFT from postgres/postgres PR #96c99955b62994b2cb47380d84f463456cf16ee5
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
		Page		page;
		bool		split_cleanup = false;

		/* Get address of bucket's start page */
		bucket_blkno = BUCKET_TO_BLKNO(cachedmetap, cur_bucket);

/* …（同文件无关代码省略）… */
		bool		retain_pin = false;
		bool		clear_dead_marking = false;

		vacuum_delay_point(false);

		page = BufferGetPage(buf);
		opaque = HashPageGetOpaque(page);
