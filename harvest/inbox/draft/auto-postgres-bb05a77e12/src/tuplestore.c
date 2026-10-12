// AUTO-DRAFT from postgres/postgres PR #c6b8af429b3592b00d3240a24c5a27077ff0dd82
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
static void
tuplestore_updatemax(Tuplestorestate *state)
{
	if (state->status == TSS_INMEM)
		state->maxSpace = Max(state->maxSpace,
							  state->allowedMem - state->availMem);
	else
	{
		state->maxSpace = Max(state->maxSpace,
							  BufFileSize(state->myfile));

		/*
		 * usedDisk never gets set to false again after spilling to disk, even
		 * if tuplestore_clear() is called and new tuples go to memory again.
		 */
		state->usedDisk = true;
	}
}
/* …（同文件无关代码省略）… */
 *		tuplestore_trim() or tuplestore_clear().
 */
void
tuplestore_get_stats(Tuplestorestate *state, char **max_storage_type,
					 int64 *max_space)
{
	tuplestore_updatemax(state);

	if (state->usedDisk)
		*max_storage_type = "Disk";
	else
		*max_storage_type = "Memory";

	*max_space = state->maxSpace;
}
