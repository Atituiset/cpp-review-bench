// AUTO-DRAFT from torvalds/linux PR #36aad21d50d1af3f6954bb0c988257a908202477
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
				  int *conv_len)
{
	char *conv;
	int  sz = min(4 * d_info->name_len, PATH_MAX);

	if (!sz)
		return NULL;

	conv = kmalloc(sz, KSMBD_DEFAULT_GFP);
	if (!conv)
		return NULL;

/* …（同文件无关代码省略）… */
				      d_info->name_len, local_nls, 0);
	*conv_len *= 2;

	/* We allocate buffer twice bigger than needed. */
	conv[*conv_len] = 0x00;
	conv[*conv_len + 1] = 0x00;
	return conv;
