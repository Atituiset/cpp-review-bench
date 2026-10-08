// AUTO-DRAFT from torvalds/linux PR #0f2732620d6ec49d3e6c06831575eaec6bc99d42
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
  // <<< BUG ANCHOR
#define ALIGN_MASK 7
static inline size_t align_val(size_t val)
{
	return (val + ALIGN_MASK) & ~ALIGN_MASK;
}
static inline void *align_ptr(void *ptr)
{
	return (void *)align_val((size_t)ptr);
}
/* …（同文件无关代码省略）… */
{
	param->data_start = align_ptr(param + 1) - (void *) param;

	if (param->data_start < param_size)
		*len = param_size - param->data_start;
	else
		*len = 0;

	return ((void *) param) + param->data_start;
}
/* …（同文件无关代码省略）… */
	/* Do we need to load a new map ? */
	if (new_map) {
		sector_t old_size, new_size;

		dm_ima_context_table_op(md, ima_context, DM_IMA_TABLE_SAVE);
		/* Suspend if it isn't already suspended */
/* …（同文件无关代码省略）… */
			}
		}

		old_size = dm_get_size(md);
		old_map = dm_swap_table(md, new_map);
		if (IS_ERR(old_map)) {
/* …（同文件无关代码省略）… */
		if (old_size && new_size && old_size != new_size)
			need_resize_uevent = true;

		if (dm_table_get_mode(new_map) & BLK_OPEN_WRITE)
			set_disk_ro(dm_disk(md), 0);
		else
			set_disk_ro(dm_disk(md), 1);
