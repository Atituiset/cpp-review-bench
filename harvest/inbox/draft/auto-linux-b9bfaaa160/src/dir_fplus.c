// AUTO-DRAFT from torvalds/linux PR #9d66f26fefacebfeed15de31c1123451a17ff119
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>

static unsigned int adfs_fplus_offset(const struct adfs_bigdirheader *h,
				      unsigned int pos)
{
	return offsetof(struct adfs_bigdirheader, bigdirname) +
	       ALIGN(le32_to_cpu(h->bigdirnamelen), 4) +
	       pos * sizeof(struct adfs_bigdirentry);
}
/* …（同文件无关代码省略）… */
	obj->indaddr  = le32_to_cpu(bde.bigdirindaddr);
	obj->attr     = le32_to_cpu(bde.bigdirattr);
	obj->name_len = le32_to_cpu(bde.bigdirobnamelen);

	offset = adfs_fplus_offset(h, le32_to_cpu(h->bigdirentries));
	offset += le32_to_cpu(bde.bigdirobnameptr);
