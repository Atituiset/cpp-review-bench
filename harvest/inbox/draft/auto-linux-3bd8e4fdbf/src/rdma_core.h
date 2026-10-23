// AUTO-DRAFT from torvalds/linux PR #fcb92ecd733e11f37c32e7485da976a40d878e72
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR

void setup_ufile_idr_uobject(struct ib_uverbs_file *ufile);

#if IS_ENABLED(CONFIG_INFINIBAND_USER_ACCESS)
struct ib_udata *uverbs_get_cleared_udata(struct uverbs_attr_bundle *attrs);
#else
static inline struct ib_udata *uverbs_get_cleared_udata(struct uverbs_attr_bundle *attrs)
{
	return NULL;
}
#endif

/*
 * This is the runtime description of the uverbs API, used by the syscall
