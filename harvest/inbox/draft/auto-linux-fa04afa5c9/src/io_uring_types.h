// AUTO-DRAFT from torvalds/linux PR #7cd8495c0f16275c4f9abe1cef09a7b5fcd6d046
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdlib.h>


	/* ctx exit and cancelation */
	struct work_struct		exit_work;
	struct completion		ref_comp;

	/* io-wq management, e.g. thread count */
