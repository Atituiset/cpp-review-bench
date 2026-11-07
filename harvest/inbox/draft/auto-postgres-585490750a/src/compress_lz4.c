// AUTO-DRAFT from postgres/postgres PR #4a934833fb02ac2a18d0a3f86b67b493d7fe8979
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

	 * LZ4Stream_read_internal returning 0 or -1 means that it was either an
	 * EOF or an error, but gets_func is defined to return NULL in either case
	 * so we can treat both the same here. LZ4Stream_read_internal is expected
	 * to have performed relevent error logging already.
	 */
	if (ret <= 0)
		return NULL;
