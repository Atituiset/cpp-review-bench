// AUTO-DRAFT from torvalds/linux PR #5d144c294ae21c1bfb275ba06ec73c2d1ab7cf92
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
			     const struct pci_device_id *pci_id)
{
	static int dev;
	struct snd_card *card __free(snd_card_unref) = NULL;
	struct snd_ice1712 *ice;
	int pcm_dev = 0, err;
	const struct snd_ice1712_card_info * const *tbl, *c;
