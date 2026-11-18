// AUTO-DRAFT from postgres/postgres PR #9b937d1d1d0c54e24280967ba1dd11aff66d1486
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	 * chosen the same as jsonb_typeof uses.
	 */
	if (type != WJB_BEGIN_ARRAY)
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("wrong flag type, only arrays and scalars are allowed")));

	while ((type = JsonbIteratorNext(&it, &v, false)) == WJB_ELEM)
	{
/* …（同文件无关代码省略）… */

	/* expect end of array now */
	if (type != WJB_END_ARRAY)
		elog(ERROR, "unexpected end of flag array");

	/* get final WJB_DONE and free iterator */
	type = JsonbIteratorNext(&it, &v, false);
	if (type != WJB_DONE)
		elog(ERROR, "unexpected end of flag array");

	return flags;
}
