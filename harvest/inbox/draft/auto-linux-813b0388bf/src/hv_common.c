// AUTO-DRAFT from torvalds/linux PR #4da9dedf3fc59137fd86d664693776aa38cb97f5
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>

#include <linux/panic_notifier.h>
#include <linux/ptrace.h>
#include <linux/random.h>
#include <linux/efi.h>
#include <linux/kdebug.h>
#include <linux/kmsg_dump.h>
/* …（同文件无关代码省略）… */
u8 * __percpu *hv_synic_eventring_tail;
EXPORT_SYMBOL_GPL(hv_synic_eventring_tail);

/*
 * Hyper-V specific initialization and shutdown code that is
 * common across all architectures.  Called from architecture
/* …（同文件无关代码省略）… */

void __init hv_common_free(void)
{
	unregister_sysctl_table(hv_ctl_table_hdr);
	hv_ctl_table_hdr = NULL;

/* …（同文件无关代码省略）… */
int __init hv_common_init(void)
{
	int i;
	union hv_hypervisor_version_info version;

	/* Get information about the Microsoft Hypervisor version */
/* …（同文件无关代码省略）… */
	for (i = 0; i < nr_cpu_ids; i++)
		hv_vp_index[i] = VP_INVAL;

	return 0;
}

/* …（同文件无关代码省略）… */
struct hv_status_info {
	char *string;
	int errno;
	u16 code;
};
/* …（同文件无关代码省略）… */
#define _STATUS_INFO(status, errno) { #status, (errno), (status) }
/* …（同文件无关代码省略）… */
	_STATUS_INFO(HV_STATUS_PROCESSOR_FEATURE_NOT_SUPPORTED,	-EOPNOTSUPP),
	_STATUS_INFO(HV_STATUS_INVALID_LP_INDEX,		-EINVAL),
	_STATUS_INFO(HV_STATUS_INVALID_REGISTER_VALUE,		-EINVAL),
	_STATUS_INFO(HV_STATUS_OPERATION_FAILED,		-EIO),
	_STATUS_INFO(HV_STATUS_TIME_OUT,			-EIO),
	_STATUS_INFO(HV_STATUS_CALL_PENDING,			-EIO),
/* …（同文件无关代码省略）… */
static inline const struct hv_status_info *find_hv_status_info(u64 hv_status)
{
	int i;
	u16 code = hv_result(hv_status);

	for (i = 0; i < ARRAY_SIZE(hv_status_infos); ++i) {
		const struct hv_status_info *info = &hv_status_infos[i];

		if (info->code == code)
			return info;
	}

	return NULL;
}
/* …（同文件无关代码省略）… */
const char *hv_result_to_string(u64 status)
{
	const struct hv_status_info *info;

	if (unlikely(status == U64_MAX))
		return "Hypercall page missing!";

	info = find_hv_status_info(status);
	if (info)
		return info->string;

	return "Unknown";
}
EXPORT_SYMBOL_GPL(hv_result_to_string);
