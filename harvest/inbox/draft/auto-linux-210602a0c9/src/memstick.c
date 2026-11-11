// AUTO-DRAFT from torvalds/linux PR #f7c68e9d5d050301548d93ccbccd644151ced269
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR

	if (dev->driver && drv->remove) {
		drv->remove(card);
		card->dev.driver = NULL;
	}

	put_device(dev);
