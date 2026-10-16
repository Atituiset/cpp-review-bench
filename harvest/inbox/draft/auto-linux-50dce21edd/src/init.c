// AUTO-DRAFT from torvalds/linux PR #62f482ff73bf3ae88b0c899115527752cf9cbf13
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
} __attribute__((packed));

bool amd_iommu_dump;
static bool amd_iommu_irq_remap __read_mostly;

enum protection_domain_mode amd_iommu_pgtable = PD_MODE_V1;
/* Virtual address size */
