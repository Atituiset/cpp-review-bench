// AUTO-DRAFT from postgres/postgres PR #d2adf339b06b8046d69be7974f9f7e490d2c9e84
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

/* A few internal functions return void (which is not the same as NULL!) */
#define PG_RETURN_VOID()	 return (Datum) 0

/* Macros for returning results of standard types */

#define PG_RETURN_DATUM(x)	 return (x)
