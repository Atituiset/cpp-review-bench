// AUTO-DRAFT from torvalds/linux PR #9b6f2eabc8371dea951c244f4aa9e013f36de9f8
#include <linux/array_size.h>
#include <linux/bitfield.h>
#include <linux/bits.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/dmi.h>
/* …（同文件无关代码省略）… */
  // <<< BUG ANCHOR
static int amd_pmc_verify_czn_rtc(struct amd_pmc_dev *pdev, u32 *arg)
{
	struct rtc_device *rtc_device;
	time64_t then, now, duration;
	struct rtc_wkalrm alarm;
	struct rtc_time tm;
/* …（同文件无关代码省略）… */
	if (pdev->major < 64 || (pdev->major == 64 && pdev->minor < 53))
		return 0;

	rtc_device = rtc_class_open("rtc0");
	if (!rtc_device)
		return 0;
	rc = rtc_read_next_alarm(rtc_device, &alarm);
