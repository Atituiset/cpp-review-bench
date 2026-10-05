// AUTO-DRAFT from torvalds/linux PR #942e4a0e46bfd0efd3f87f70bf186c54aaaeb138
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

		clear_thread_flag(TIF_HRTIMER_REARM);
		__hrtimer_rearm_deferred();
		/* Don't go into the loop if HRTIMER_REARM was the only flag */
		*tif_work &= ~TIF_HRTIMER_REARM;
		return !*tif_work;
	}
	return false;
/* …（同文件无关代码省略）… */
static __always_inline void __hrtimer_rearm_deferred(void) { }
