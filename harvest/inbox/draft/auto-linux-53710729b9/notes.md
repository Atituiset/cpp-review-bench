# auto-linux-53710729b9

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#e191b05f321b064a973399b7cdc003c0de89e5b9](https://github.com/torvalds/linux/commit/e191b05f321b064a973399b7cdc003c0de89e5b9) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-29 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 62 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e191b05f321b064a973399b7cdc003c0de89e5b9 (https://github.com/torvalds/linux/commit/e191b05f321b064a973399b7cdc003c0de89e5b9)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 682；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e191b05f321b064a973399b7cdc003c0de89e5b9 Merge tag 'phy-for-7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/phy/linux-phy :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3,6 +3,7 @@
  * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  */
 
+#include <linux/bits.h>
 #include <linux/clk.h>
 #include <linux/clk-provider.h>
 #include <linux/delay.h>
@@ -22,6 +23,11 @@
 #include <linux/slab.h>
 
 #include "phy-qcom-qmp.h"
+#include "phy-qcom-qmp-common.h"
+#include "phy-qcom-qmp-pcs-v6_40.h"
+#include "phy-qcom-qmp-pcs-pcie-v6_40.h"
+#include "phy-qcom-qmp-qserdes-com-pcie-v6_40.h"
+#include "phy-qcom-qmp-qserdes-txrxz-pcie-v6_40.h"
 
 #define PHY_INIT_COMPLETE_TIMEOUT_US		10000
 
@@ -30,25 +36,68 @@ enum qmp_pcie_glymur_link_mode {
 	QMP_PCIE_GLYMUR_MODE_X4X4,
 };
 
+enum qmp_pcie_nord_link_mode {
+	QMP_PCIE_NORD_MODE_X16,
+	QMP_PCIE_NORD_MODE_X8_X8,
+	QMP_PCIE_NORD_MODE_X8_X4_X4,
+	QMP_PCIE_NORD_MODE_X8_X4_X2_X2
+};
+
 enum qphy_reg_layout {
 	QPHY_PCS_STATUS,
+	QPHY_PCS_START_CONTROL,
+	QPHY_PCS_SW_RESET,
+	QPHY_PCS_POWER_DOWN_CONTROL,
+	QPHY_COM_BIAS_EN_CLKBUFLR_EN,
 	QPHY_LAYOUT_SIZE
 };
 
 static const unsigned int pciephy_v8_50_regs_layout[QPHY_LAYOUT_SIZE] = {
 	[QPHY_PCS_STATUS]		= QPHY_V8_50_PCS_STATUS1,
 };
 
+static const unsigned int nord_pciephy_v6_40_regs_layout[QPHY_LAYOUT_SIZE] = {
+	[QPHY_PCS_STATUS]		= QPHY_V6_40_PCS_STATUS1,
+	[QPHY_PCS_START_CONTROL]	= QPHY_V6_40_PCS_START_CONTROL,
+	[QPHY_PCS_SW_RESET]		= QPHY_V6_40_PCS_SW_RESET,
+	[QPHY_PCS_POWER_DOWN_CONTROL]	= QPHY_V6_40_PCS_POWER_DOWN_CONTROL,
+	[QPHY_COM_BIAS_EN_CLKBUFLR_EN]	= QSERDES_PCIE_V6_40_COM_BIAS_EN_CLKBUFLR_EN,
+};
+
 struct qmp_pcie_offsets {
 	u16 pcs;
+	u16 pll;
+	u16 txrxz;
+	u16 pcs_lanez;
+};
+
+struct qmp_phy_cfg_tbls {
+	const struct qmp_phy_init_tbl *pll_common;
+	int pll_common_num;
+	const struct qmp_phy_init_tbl *txrxz;
+	int txrxz_num;
+	const struct qmp_phy_init_tbl *pcs;
+	int pcs_num;
+	const struct qmp_phy_init_tbl *pcs_lanez;
+	int pcs_lanez_num;
+	const struct qmp_phy_init_tbl *port_split_pre_pwrup;
+	int port_split_pre_pwrup_num;
+	const struct qmp_phy_init_tbl *port_split_post_cfg;
+	int port_split_post_cfg_num;
 };
 
 struct qmp_phy_cfg {
 	const struct qmp_pcie_offsets *offsets;
+	const struct qmp_phy_cfg_tbls tbls;
+	const struct qmp_phy_init_tbl * const *pll_tbls;
+	const int *pll_nums;
+	bool port_split;
 	const char * const *reg_names;
 	int num_regs;
 	const char * const *pd_names;
 	int num_pds;
+	const char * const *reset_list;
+	int num_resets;
 	const char * const *nocsr_reset_list;
 	int num_nocsr_resets;
 	const char * const *vreg_list;
@@ -64,9 +113,11 @@ struct qmp_phy_cfg {
 struct qmp_pcie {
 	struct device *dev;
 	const struct qmp_phy_cfg *cfg;
+	bool skip_init;
 	void __iomem **base;
 	struct clk_bulk_data *clks;
 	struct clk_bulk_data *pipe_clks;
+	struct reset_control_bulk_data *resets;
 	struct reset_control_bulk_data *nocsr_resets;
 	struct regulator_bulk_data *vregs;
 	struct device **pd_devs;
@@ -89,6 +140,10 @@ struct qmp_pcie_match_data {
 	u32 num_modes;
 };
 
+/* QSERDES_PCIE_V6_40_COM_BIAS_EN_CLKBUFLR_EN bits */
+#define COM_BIAS_EN				BIT(0)
+#define COM_BIAS_EN_MUX				BIT(1)
+
 static const char * const glymur_pciephy_clk_l[] = {
 	"aux", "cfg_ahb", "ref", "rchng", "phy_b_aux",
 };
@@ -113,39 +168,27 @@ static const char * const glymur_vreg_l[] = {
 	"vdda-phy", "vdda-pll", "vdda-refgen0p9", "vdda-refgen1p2",
 };
 
-static const char * const glymur_pciephy_a_reg_l[] = {
+static const char * const pciephy_port_a_reg_l[] = {
 	"port_a",
 };
 
-static const char * const glymur_pciephy_b_reg_l[] = {
+static const char * const pciephy_port_b_reg_l[] = {
 	"port_b",
 };
 
-static const char * const glymur_pciephy_reg_l[] = {
+static const char * const pciephy_port_ab_reg_l[] = {
 	"port_a", "port_b",
 };
 
-static const char * const glymur_pciephy_a_pd_l[] = {
-	"port_a",
-};
-
-static const char * const glymur_pciephy_b_pd_l[] = {
-	"port_b",
-};
-
-static const char * const glymur_pciephy_pd_l[] = {
-	"port_a", "port_b",
-};
-
-static const char * const glymur_pciephy_a_nocsr_reset_l[] = {
+static const char * const pciephy_port_a_nocsr_rese
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`clk_bulk_disable_unprepare`
- 外部函数：`clk_bulk_prepare_enable`
- 外部函数：`dev_err`
- 外部函数：`dev_err_probe`
- 外部函数：`dev_pm_domain_attach_by_name`
- 外部函数：`dev_pm_domain_detach`
- 外部函数：`devm_add_action_or_reset`
- 外部函数：`devm_clk_bulk_get_optional`
- 外部函数：`devm_kzalloc`
- 外部函数：`devm_phy_create`
- 外部函数：`devm_platform_ioremap_resource_byname`
- 外部函数：`devm_regulator_bulk_get`
- 外部函数：`devm_reset_control_bulk_get_exclusive`
- 外部函数：`module_platform_driver`
- 外部函数：`of_device_get_match_data`
- 外部函数：`of_property_count_strings`
- 外部函数：`phy_get_drvdata`
- 外部函数：`phy_set_drvdata`
- 外部函数：`qmp_pcie_get_mmio`
- 外部函数：`qmp_pcie_pd_detach`
- 外部函数：`regmap_read`
- 外部函数：`regulator_bulk_disable`
- 外部函数：`regulator_bulk_enable`
- 外部函数：`reset_control_bulk_assert`
- 外部函数：`syscon_regmap_lookup_by_phandle_args`
- 外部函数：`to_platform_device`
- 外部函数：`usleep_range`
- 大写宏：`ARRAY_SIZE`
- 大写宏：`EINVAL`
- 大写宏：`ENODATA`
- 大写宏：`ENOMEM`
- 大写宏：`GFP_KERNEL`
- 大写宏：`GPL`
- 大写宏：`IS_ERR`
- 大写宏：`IS_ERR_OR_NULL`
- 大写宏：`MODULE_AUTHOR`
- 大写宏：`MODULE_DESCRIPTION`
- 大写宏：`MODULE_DEVICE_TABLE`
- 大写宏：`MODULE_LICENSE`
- 大写宏：`PHY`
- 大写宏：`PTR_ERR`
- 大写宏：`QMP`
- 大写宏：`QMP_PCIE_GLYMUR_MODE_X4X4`
- 大写宏：`QPHY_LAYOUT_SIZE`
- 大写宏：`QPHY_PCS_STATUS`
- 大写宏：`QPHY_V8_50_PCS_STATUS1`
- 外部类型：`Glymur`
- 外部类型：`Inc`
- 外部类型：`Multi`
- 外部类型：`PCIe`
- 外部类型：`Qiang`
- 外部类型：`Qualcomm`
- 外部类型：`Technologies`
- 外部类型：`Yu`
- 外部类型：`clk_bulk_data`
- 外部类型：`device`
- 外部类型：`device_node`
- 外部类型：`phy`
- 外部类型：`platform_device`
- 外部类型：`regmap`
- 外部类型：`regulator_bulk_data`
- 外部类型：`reset_control_bulk_data`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-linux-53710729b9` → 本草稿移入 `cases/defect/auto-linux-53710729b9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
