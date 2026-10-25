// AUTO-DRAFT from torvalds/linux PR #168151cdb0f93d26557f9f3c7aae659b8d6b1863
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include <linux/spinlock.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#include "core.h"
	/* Add gadget debugfs nodes */
	dwc2_hsotg_create_debug(hsotg);

	hsotg->regset = devm_kzalloc(hsotg->dev, sizeof(*hsotg->regset),
								GFP_KERNEL);
	if (!hsotg->regset) {
		ret = -ENOMEM;
		goto err;
{
	debugfs_remove_recursive(hsotg->debug_root);
	hsotg->debug_root = NULL;
}
