// AUTO-DRAFT from torvalds/linux PR #42e225323067a222230068651cf886bd7fbb1fe2
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

static __always_inline bool system_capabilities_finalized(void)
{
	return alternative_has_cap_likely(ARM64_ALWAYS_SYSTEM);
}
/* …（同文件无关代码省略）… */
static __always_inline bool cpus_have_final_cap(int num)
{
	if (system_capabilities_finalized())
		return alternative_has_cap_unlikely(num);
	else
		BUG();
}
/* …（同文件无关代码省略）… */
	return cpus_have_final_cap(ARM64_HAS_LPA2);
}

static inline bool system_supports_poe(void)
{
	return alternative_has_cap_unlikely(ARM64_HAS_S1POE);
