// AUTO-DRAFT from torvalds/linux PR #4843f6873f7c4835e62f2c73ba0fd538e57a2f22
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
static ulong sysctl_jiffies_to_clock_t(const ulong val)
{
	return jiffies_to_clock_t(val);
}
/* …（同文件无关代码省略）… */
	return proc_int_k2u_conv_kop(u_ptr, k_ptr, negp, sysctl_jiffies_to_clock_t);
}

static ulong sysctl_msecs_to_jiffies(const ulong val)
{
	if (val > jiffies_to_msecs(MAX_JIFFY_OFFSET))
		return MAX_JIFFY_OFFSET;
	return msecs_to_jiffies(val);
}

static int sysctl_u2k_int_conv_ms(const bool *negp, const ulong *u_ptr, int *k_ptr)
/* …（同文件无关代码省略）… */

static ulong sysctl_jiffies_to_msecs(const ulong val)
{
	return jiffies_to_msecs(val);
}

static int sysctl_k2u_int_conv_ms(bool *negp, ulong *u_ptr, const int *k_ptr)
{
	return proc_int_k2u_conv_kop(u_ptr, k_ptr, negp, sysctl_jiffies_to_msecs);
}
