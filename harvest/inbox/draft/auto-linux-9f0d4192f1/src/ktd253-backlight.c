// AUTO-DRAFT from torvalds/linux PR #d135ab453a7400a34930bdda1ef4bdc8acfac80e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <time.h>

	 *
	 * Architectures do not always support ndelay() and we will get a few us
	 * instead. If we get to a critical time limit an interrupt has likely
	 * occured in the low part of the loop and we need to restart from the
	 * top so we have the backlight in a known state.
	 */
	u64 ns;
