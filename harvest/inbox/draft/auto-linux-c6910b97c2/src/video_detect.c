// AUTO-DRAFT from torvalds/linux PR #c339f4010dea8b0d754f84ace70cd1b089878ef8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
	return AE_OK;
}

/* This depends on ACPI_WMI which is X86 only */
#ifdef CONFIG_X86
static bool nvidia_wmi_ec_supported(void)
{
	struct wmi_brightness_args args = {
/* …（同文件无关代码省略）… */
	 */
	return args.ret == WMI_BRIGHTNESS_SOURCE_EC;
}
#else
static bool nvidia_wmi_ec_supported(void)
{
	return false;
}
#endif

/* Force to use vendor driver when the ACPI device is known to be
 * buggy */
