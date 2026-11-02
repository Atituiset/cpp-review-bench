// AUTO-DRAFT from torvalds/linux PR #9b6f2eabc8371dea951c244f4aa9e013f36de9f8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
 */

static bool use_acpi_alarm;
module_param(use_acpi_alarm, bool, 0444);

static inline int cmos_use_acpi_alarm(void)
{
/* …（同文件无关代码省略）… */
	unsigned char rtc_intr;
	unsigned long flags;


	/*
	 * Always update rtc irq when ACPI is used as RTC Alarm.
	 * Or else, ACPI SCI is enabled during suspend/resume only,
/* …（同文件无关代码省略）… */
	}

	pm_wakeup_hard_event(dev);
	acpi_clear_event(ACPI_EVENT_RTC);
	acpi_disable_event(ACPI_EVENT_RTC, 0);
	return ACPI_INTERRUPT_HANDLED;
}

/* …（同文件无关代码省略）… */
#ifdef CONFIG_X86
static void use_acpi_alarm_quirks(void)
{
	if (acpi_gbl_FADT.flags & ACPI_FADT_FIXED_RTC)
		return;
