// AUTO-DRAFT from postgres/postgres PR #a936ebd9f9f64b0efb853195d8dcdb8a0453537d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>

#define pg_attribute_target(...)
#endif

/*
 * Append PG_USED_FOR_ASSERTS_ONLY to definitions of variables that are only
 * used in assert-enabled builds, to avoid compiler warnings about unused
/* …（同文件无关代码省略）… */
#define pg_attribute_aligned(a) __attribute__((aligned(a)))
/* …（同文件无关代码省略）… */
typedef PG_INT128_TYPE int128
#if defined(pg_attribute_aligned)
			pg_attribute_aligned(MAXIMUM_ALIGNOF)
#endif
		   ;
