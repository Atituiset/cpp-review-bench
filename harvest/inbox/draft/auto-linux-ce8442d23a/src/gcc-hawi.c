// AUTO-DRAFT from torvalds/linux PR #0c2669a9f4a1d607e7591ae50ccf3c432a0aff08
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
	0x67084, /* GCC_PCIE_1_RSC_CORE_CLK */
	0x43014, /* GCC_PCIE_LINK_XO_CLK */
	0x6b088, /* GCC_PCIE_RSC_CORE_CLK */
	0x52010, /* GCC_PCIE_RSCC_CFG_AHB_CLK */
	0x52010, /* GCC_PCIE_RSCC_XO_CLK */
	0x32004, /* GCC_VIDEO_AHB_CLK */
	0x32028, /* GCC_VIDEO_XO_CLK */
};
/* …（同文件无关代码省略）… */

static void clk_hawi_regs_configure(struct device *dev, struct regmap *regmap)
{
	/* FORCE_MEM_CORE_ON for ufs phy ice core clocks */
	qcom_branch_set_force_mem_core(regmap, gcc_ufs_phy_ice_core_clk, true);
}
