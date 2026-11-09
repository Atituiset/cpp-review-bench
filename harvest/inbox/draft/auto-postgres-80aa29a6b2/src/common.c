// AUTO-DRAFT from postgres/postgres PR #8b529705d153f6bc592f874a9f2dd47be8fa0589
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
static void
psql_cancel_callback(void)
{
#ifndef WIN32
	/* if we are waiting for input, longjmp out of it */
	if (sigint_interrupt_enabled)
/* …（同文件无关代码省略）… */
		siglongjmp(sigint_interrupt_jmp, 1);
	}
#endif

	/* else, set cancel flag to stop any long-running loops */
	cancel_pressed = true;
}

void
