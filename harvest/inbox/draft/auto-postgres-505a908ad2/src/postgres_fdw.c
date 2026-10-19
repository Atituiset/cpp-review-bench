// AUTO-DRAFT from postgres/postgres PR #2db51901c48e44369f5e6f209a65983c206075da
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
{
	if (s)
	{
		float4		val = float4in_internal((char *) s, NULL, "float", s, NULL);

		arg->value = Float4GetDatum(val);
		arg->isnull = false;
