// AUTO-DRAFT from torvalds/linux PR #7d1471e50943196f909674d2d22eb4196c7df48e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
				  const char *dev_name)
{
	struct fscache_volume *vcookie;
	char *name, *p;

	name = kasprintf(GFP_KERNEL, "9p,%s,%s",
			 dev_name, v9ses->cachetag ?: v9ses->aname);
	if (!name)
		return -ENOMEM;

	for (p = name; *p; p++)
		if (*p == '/')
			*p = ';';

	vcookie = fscache_acquire_volume(name, NULL, NULL, 0);
	p9_debug(P9_DEBUG_FSC, "session %p get volume %p (%s)\n",
		 v9ses, vcookie, name);
