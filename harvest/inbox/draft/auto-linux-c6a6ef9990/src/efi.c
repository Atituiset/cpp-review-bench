// AUTO-DRAFT from torvalds/linux PR #89d2d7bf0698b7fdfc914bfd219e739e9308d332
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdio.h>
  // <<< BUG ANCHOR
#define EFI_FIELD(var) efi_ ## var

#define EFI_ATTR_SHOW(name) \
static ssize_t name##_show(struct kobject *kobj, \
				struct kobj_attribute *attr, char *buf) \
{ \
	return sprintf(buf, "0x%lx\n", EFI_FIELD(name)); \
}

EFI_ATTR_SHOW(fw_vendor);
EFI_ATTR_SHOW(runtime);
EFI_ATTR_SHOW(config_table);

struct kobj_attribute efi_attr_fw_vendor = __ATTR_RO(fw_vendor);
struct kobj_attribute efi_attr_runtime = __ATTR_RO(runtime);
struct kobj_attribute efi_attr_config_table = __ATTR_RO(config_table);

umode_t efi_attr_is_visible(struct kobject *kobj, struct attribute *attr, int n)
{
	if (attr == &efi_attr_fw_vendor.attr) {
		if (efi_enabled(EFI_PARAVIRT) ||
