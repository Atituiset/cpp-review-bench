// AUTO-DRAFT from torvalds/linux PR #7641bffc3721f0d1650e52935369ec7f7bf84126
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR

	mutex_lock(&opts->lock);

	config_item_put(item);
	list_del(&xu->list);
	kfree(xu->desc.baSourceID);
	kfree(xu->desc.bmControls);

	mutex_unlock(&opts->lock);
}

/* …（同文件无关代码省略）… */
	NULL,
};

static const struct config_item_type uvcg_frame_type1 = {
	.ct_item_ops	= &uvcg_config_item_ops,
	.ct_attrs	= uvcg_frame_attrs1,
	.ct_owner	= THIS_MODULE,
};

static const struct config_item_type uvcg_frame_type2 = {
	.ct_item_ops    = &uvcg_config_item_ops,
	.ct_attrs       = uvcg_frame_attrs2,
	.ct_owner       = THIS_MODULE,
};
