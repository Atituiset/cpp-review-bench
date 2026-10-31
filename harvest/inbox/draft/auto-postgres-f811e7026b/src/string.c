// AUTO-DRAFT from postgres/postgres PR #426551c8cf4902b6164cde1104df3783bfc50483
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stdlib.h>

#include "postgres_fe.h"
#endif

#include "common/string.h"


/* …（同文件无关代码省略）… */

/*
 * strtoint --- just like strtol, but returns int not long
 */
int
strtoint(const char *pg_restrict str, char **pg_restrict endptr, int base)
{
	long		val;

	val = strtol(str, endptr, base);
	if (val != (int) val)
		errno = ERANGE;
	return (int) val;
}
