// AUTO-DRAFT from postgres/postgres PR #7d47e41238004b6b8bce076d4bb76d858257b58d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR

		fdwroutine = GetFdwRoutineForRelation(onerel, false);

		if (fdwroutine->ImportForeignStatistics != NULL &&
			fdwroutine->ImportForeignStatistics(onerel, va_cols, elevel))
			stats_imported = true;
		else
		{
			bool		ok = false;
