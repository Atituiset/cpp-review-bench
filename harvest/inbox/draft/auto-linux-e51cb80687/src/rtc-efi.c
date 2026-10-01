// AUTO-DRAFT from torvalds/linux PR #551c722f40809618230001baccf219193e22fc5a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <string.h>
  // <<< BUG ANCHOR
static void
convert_to_efi_time(struct rtc_time *wtime, efi_time_t *eft)
{
	eft->year	= wtime->tm_year + 1900;
	eft->month	= wtime->tm_mon + 1;
	eft->day	= wtime->tm_mday;
/* …（同文件无关代码省略）… */
	return true;
}

static int efi_read_time(struct device *dev, struct rtc_time *tm)
{
	efi_status_t status;
/* …（同文件无关代码省略）… */
static int efi_set_time(struct device *dev, struct rtc_time *tm)
{
	efi_status_t status;
	efi_time_t eft;

	convert_to_efi_time(tm, &eft);

	status = efi.set_time(&eft);

	return status == EFI_SUCCESS ? 0 : -EINVAL;
}

static int efi_procfs(struct device *dev, struct seq_file *seq)
{
	efi_time_t        eft;
	efi_time_cap_t    cap;

	memset(&eft, 0, sizeof(eft));
	memset(&cap, 0, sizeof(cap));

	efi.get_time(&eft, &cap);

	seq_printf(seq,
		   "Time\t\t: %u:%u:%u.%09u\n"
/* …（同文件无关代码省略）… */
		/* XXX fixme: convert to string? */
		seq_printf(seq, "Timezone\t: %u\n", eft.timezone);

	/*
	 * now prints the capabilities
	 */
/* …（同文件无关代码省略）… */
static const struct rtc_class_ops efi_rtc_ops = {
	.read_time	= efi_read_time,
	.set_time	= efi_set_time,
	.proc		= efi_procfs,
};

/* …（同文件无关代码省略）… */
	struct rtc_device *rtc;
	efi_time_t eft;
	efi_time_cap_t cap;

	/* First check if the RTC is usable */
	if (efi.get_time(&eft, &cap) != EFI_SUCCESS)
/* …（同文件无关代码省略）… */
	platform_set_drvdata(dev, rtc);

	rtc->ops = &efi_rtc_ops;
	clear_bit(RTC_FEATURE_ALARM, rtc->features);

	device_init_wakeup(&dev->dev, true);
