// AUTO-DRAFT from postgres/postgres PR #514bfe2c6dffba2d7a6b7ea7cf029e04a7c41563
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
	}
	else
	{
		appendStringInfoString(&buf, "AS ");

		tmp = SysCacheGetAttr(PROCOID, proctup, Anum_pg_proc_probin, &isnull);
/* …（同文件无关代码省略）… */
		 * Since the user is likely to be editing the function body string, we
		 * shouldn't use a short delimiter that he might easily create a
		 * conflict with.  Hence prefer "$function$"/"$procedure$", but extend
		 * if needed.
		 */
		initStringInfo(&dq);
		appendStringInfoChar(&dq, '$');
		appendStringInfoString(&dq, (isfunction ? "function" : "procedure"));
		while (strstr(prosrc, dq.data) != NULL)
			appendStringInfoChar(&dq, 'x');
		appendStringInfoChar(&dq, '$');
