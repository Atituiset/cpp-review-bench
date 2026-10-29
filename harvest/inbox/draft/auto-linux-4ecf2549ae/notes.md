# auto-linux-4ecf2549ae

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
| 外部依赖数（dep_count） | 138 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e191b05f321b064a973399b7cdc003c0de89e5b9 (https://github.com/torvalds/linux/commit/e191b05f321b064a973399b7cdc003c0de89e5b9)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 4789；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e191b05f321b064a973399b7cdc003c0de89e5b9 Merge tag 'phy-for-7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/phy/linux-phy :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -22,6 +22,7 @@
 #include <linux/usb/typec.h>
 #include <linux/usb/typec_dp.h>
 #include <linux/usb/typec_mux.h>
+#include <linux/usb/typec_tbt.h>
 
 #include <drm/bridge/aux-bridge.h>
 
@@ -66,10 +67,14 @@
 #define SW_USB3PHY_RESET			BIT(2)
 /* mux to select USB3 PHY reset control, 0:HW control, 1: software reset */
 #define SW_USB3PHY_RESET_MUX			BIT(3)
+#define SW_USB4PHY_RESET			BIT(4)
+#define SW_USB4PHY_RESET_MUX			BIT(5)
 
 /* QPHY_V3_DP_COM_PHY_MODE_CTRL register bits */
 #define USB3_MODE				BIT(0) /* enables USB3 mode */
 #define DP_MODE					BIT(1) /* enables DP mode */
+#define USB4_MODE				BIT(2) /* mutually exclusive with the above */
+#define DP_TUNNELING_CLOCK_GEN_EN		BIT(3)
 
 /* QPHY_V3_DP_COM_TYPEC_CTRL register bits */
 #define SW_PORTSELECT_VAL			BIT(0)
@@ -82,6 +87,8 @@ enum qmpphy_mode {
 	QMPPHY_MODE_USB3DP = 0,
 	QMPPHY_MODE_DP_ONLY,
 	QMPPHY_MODE_USB3_ONLY,
+	/* USB4 QMPPHY mode refers to both USB4 and TBT3 */
+	QMPPHY_MODE_USB4,
 };
 
 /* set of registers with offsets different per-PHY */
@@ -94,6 +101,7 @@ enum qphy_reg_layout {
 	QPHY_PCS_LFPS_RXTERM_IRQ_CLEAR,
 	QPHY_PCS_POWER_DOWN_CONTROL,
 	QPHY_PCS_CLAMP_ENABLE,
+	QPHY_PCS_USB4_CLAMP_ENABLE,
 
 	QPHY_COM_RESETSM_CNTRL,
 	QPHY_COM_C_READY_STATUS,
@@ -2348,6 +2356,20 @@ static const struct qmp_phy_init_tbl hawi_usb3_pcs_usb_tbl[] = {
 	QMP_PHY_INIT_CFG(QPHY_V10_PCS_USB3_RCVR_DTCT_DLY_U3_H, 0x00),
 };
 
+static const struct qmp_phy_init_tbl nord_usb3_tx_tbl[] = {
+	QMP_PHY_INIT_CFG(QSERDES_V6_TX_RES_CODE_LANE_TX, 0x00),
+	QMP_PHY_INIT_CFG(QSERDES_V6_TX_RES_CODE_LANE_RX, 0x00),
+	QMP_PHY_INIT_CFG(QSERDES_V6_TX_RES_CODE_LANE_OFFSET_TX, 0x1f),
+	QMP_PHY_INIT_CFG(QSERDES_V6_TX_RES_CODE_LANE_OFFSET_RX, 0x09),
+	QMP_PHY_INIT_CFG(QSERDES_V6_TX_LANE_MODE_1, 0xf5),
+	QMP_PHY_INIT_CFG(QSERDES_V6_TX_LANE_MODE_3, 0x3f),
+	QMP_PHY_INIT_CFG(QSERDES_V6_TX_LANE_MODE_4, 0x3f),
+	QMP_PHY_INIT_CFG(QSERDES_V6_TX_LANE_MODE_5, 0x5f),
+	QMP_PHY_INIT_CFG(QSERDES_V6_TX_RCV_DETECT_LVL_2, 0x14),
+	QMP_PHY_INIT_CFG_LANE(QSERDES_V6_TX_PI_QEC_CTRL, 0x21, 1),
+	QMP_PHY_INIT_CFG_LANE(QSERDES_V6_TX_PI_QEC_CTRL, 0x05, 2),
+};
+
 /* list of regulators */
 static struct regulator_bulk_data qmp_phy_vreg_l[] = {
 	{ .supply = "vdda-phy", .init_load_uA = 21800, },
@@ -2489,6 +2511,8 @@ struct qmp_combo_offsets {
 	u16 dp_txb;
 	u16 dp_dp_phy;
 	u16 aon_toggle;
+	u16 usb4_serdes;
+	u16 usb4_pcs;
 };
 
 struct qmp_phy_cfg {
@@ -2536,6 +2560,18 @@ struct qmp_phy_cfg {
 	int (*calibrate_dp_phy)(struct qmp_combo *qmp);
 	void (*dp_aux_init)(struct qmp_combo *qmp);
 
+	/* USB4 specifics */
+	const struct qmp_phy_init_tbl *usb4_serdes_tbl;
+	int usb4_serdes_tbl_num;
+	const struct qmp_phy_init_tbl *usb4_serdes_tb3_ovrd_tbl;
+	int usb4_serdes_tb3_ovrd_num;
+	const struct qmp_phy_init_tbl *usb4_tx_tbl;
+	int usb4_tx_tbl_num;
+	const struct qmp_phy_init_tbl *usb4_rx_tbl;
+	int usb4_rx_tbl_num;
+	const struct qmp_phy_init_tbl *usb4_pcs_tbl;
+	int usb4_pcs_tbl_num;
+
 	/* resets to be requested */
 	const char * const *reset_list;
 	int num_resets;
@@ -2578,8 +2614,12 @@ struct qmp_combo {
 	void __iomem *dp_tx2;
 	void __iomem *dp_dp_phy;
 
+	void __iomem *usb4_serdes;
+	void __iomem *usb4_pcs;
+
 	struct clk *pipe_clk;
 	struct clk_bulk_data *clks;
+	struct clk *p2rr2p_pipe_clk;
 	int num_clks;
 	struct reset_control_bulk_data *resets;
 	struct regulator_bulk_data *vregs;
@@ -2598,6 +2638,10 @@ struct qmp_combo {
 	unsigned int dp_init_count;
 	bool dp_powered_on;
 
+	struct phy *usb4_phy;
+	enum tbt_phy_submode usb4_phy_submode;
+	unsigned int usb4_init_count;
+
 	struct clk_fixed_rate pipe_clk_fixed;
 	struct clk_hw dp_link_hw;
 	struct clk_hw dp_pixel_hw;
@@ -2622,30 +2666,6 @@ static void qmp_v8_dp_aux_init(struct qmp_combo *qmp);
 static int qmp_v8_configure_dp_clocks(struct qmp_combo *qmp);
 static int qmp_v8_configure_dp_phy(struct qmp_combo *qmp);
 
-static inline void qphy_setbits(void __iomem *base, u32 offset, u32 val)
-{
-	u32 reg;
-
-	reg = readl(base + offset);
-	reg |= val;
-	wri
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`clk_bulk_disable_unprepare`
- 外部函数：`clk_disable_unprepare`
- 外部函数：`clk_set_rate`
- 外部函数：`configure_dp_clocks`
- 外部函数：`configure_dp_tx`
- 外部函数：`dev_dbg`
- 外部函数：`dev_err`
- 外部函数：`dev_get_drvdata`
- 外部函数：`devm_clk_bulk_get_optional`
- 外部函数：`devm_kcalloc`
- 外部函数：`devm_of_phy_provider_register`
- 外部函数：`dp_aux_init`
- 外部函数：`int`
- 外部函数：`memcpy`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`phy_get_drvdata`
- 外部函数：`phy_set_drvdata`
- 外部函数：`qmp_combo_clk_init`
- 外部函数：`qmp_configure`
- 外部函数：`qmp_configure_lane`
- 外部函数：`readl`
- 外部函数：`readl_poll_timeout`
- 外部函数：`regulator_bulk_disable`
- 外部函数：`regulator_bulk_enable`
- 外部函数：`reset_control_bulk_assert`
- 外部函数：`typec_mux_get_drvdata`
- 外部函数：`typec_switch_get_drvdata`
- 外部函数：`typec_switch_unregister`
- 外部函数：`udelay`
- 外部函数：`usleep_range`
- 外部函数：`void`
- 外部函数：`writel`
- 大写宏：`ARRAY_SIZE`
- 大写宏：`BIT`
- 大写宏：`CLAMP_EN`
- 大写宏：`DOWN`
- 大写宏：`DP_PHY_PD_CTL_AUX_PWRDN`
- 大写宏：`DP_PHY_PD_CTL_DP_CLAMP_EN`
- 大写宏：`DP_PHY_PD_CTL_LANE_0_1_PWRDN`
- 大写宏：`DP_PHY_PD_CTL_LANE_2_3_PWRDN`
- 大写宏：`DP_PHY_PD_CTL_PLL_PWRDN`
- 大写宏：`DP_PHY_PD_CTL_PSR_PWRDN`
- 大写宏：`DP_PHY_PD_CTL_PWRDN`
- 大写宏：`EINVAL`
- 大写宏：`ENOMEM`
- 大写宏：`ERR_PTR`
- 大写宏：`ETIMEDOUT`
- 大写宏：`GFP_KERNEL`
- 大写宏：`PCS`
- 大写宏：`PCS_START`
- 大写宏：`PHY`
- 大写宏：`PHYSTATUS`
- 大写宏：`POWER`
- 大写宏：`QMPPHY_MODE_DP_ONLY`
- 大写宏：`QMPPHY_MODE_USB3DP`
- 大写宏：`QMPPHY_MODE_USB3_ONLY`
- 大写宏：`QMP_PHY_INIT_CFG`
- 大写宏：`QMP_USB43DP_DP_PHY`
- 大写宏：`QMP_USB43DP_USB3_PHY`
- 大写宏：`QPHY_AON_TOGGLE_ENABLE`
- 大写宏：`QPHY_COM_CMN_STATUS`
- 大写宏：`QPHY_COM_C_READY_STATUS`
- 大写宏：`QPHY_COM_RESETSM_CNTRL`
- 大写宏：`QPHY_DP_PHY_STATUS`
- 大写宏：`QPHY_DP_PHY_VCO_DIV`
- 大写宏：`QPHY_PCS_AUTONOMOUS_MODE_CTRL`
- 大写宏：`QPHY_PCS_CLAMP_ENABLE`
- 大写宏：`QPHY_PCS_LFPS_RXTERM_IRQ_CLEAR`
- 大写宏：`QPHY_PCS_POWER_DOWN_CONTROL`
- 大写宏：`QPHY_PCS_STATUS`
- 大写宏：`QPHY_START_CTRL`
- 大写宏：`QPHY_SW_RESET`
- 大写宏：`QPHY_TX_HIGHZ_DRVR_EN`
- 大写宏：`QPHY_TX_TRANSCEIVER_BIAS_EN`
- 大写宏：`QPHY_TX_TX_DRV_LVL`
- 大写宏：`QPHY_TX_TX_EMP_POST1_LVL`
- 大写宏：`QPHY_V10_PCS_USB3_RCVR_DTCT_DLY_U3_H`
- 大写宏：`QPHY_V3_DP_COM_PHY_MODE_CTRL`
- 大写宏：`QPHY_V3_DP_COM_RESET_OVRD_CTRL`
- 大写宏：`QPHY_V3_DP_COM_SWI_CTRL`
- 大写宏：`QPHY_V3_DP_COM_SW_RESET`
- 大写宏：`QPHY_V3_DP_COM_TYPEC_CTRL`
- 大写宏：`QSERDES_DP_PHY_AUX_CFG1`
- 大写宏：`QSERDES_DP_PHY_AUX_CFG2`
- 大写宏：`QSERDES_DP_PHY_CFG`
- 大写宏：`QSERDES_DP_PHY_CFG_1`
- 大写宏：`QSERDES_DP_PHY_MODE`
- 大写宏：`QSERDES_DP_PHY_PD_CTL`
- 大写宏：`QSERDES_V4_DP_PHY_TX0_TX1_LANE_CTL`
- 大写宏：`QSERDES_V4_DP_PHY_TX2_TX3_LANE_CTL`
- 大写宏：`QSERDES_V8_DP_PHY_AUXLESS_CFG1`
- 大写宏：`QSERDES_V8_DP_PHY_AUXLESS_SETUP_CYC`
- 大写宏：`QSERDES_V8_DP_PHY_AUXLESS_SILENCE_CYC`
- 大写宏：`QSERDES_V8_DP_PHY_LFPS_CYC`
- 大写宏：`QSERDES_V8_DP_PHY_LFPS_PERIOD`
- 大写宏：`QSERDES_V8_DP_PHY_LN0_DRV_LVL`
- 大写宏：`QSERDES_V8_DP_PHY_LN1_DRV_LVL`
- 大写宏：`QSERDES_V8_DP_PHY_TSYNC_OVRD`
- 大写宏：`QSERDES_V8_DP_PHY_TX0_TX1_LANE_CTL`
- 大写宏：`QSERDES_V8_DP_PHY_TX2_TX3_LANE_CTL`
- 大写宏：`SERDES_START`
- 大写宏：`SW_PWRDN`
- 大写宏：`SW_RESET`
- 大写宏：`THIS_MODULE`
- 大写宏：`TYPEC_ORIENTATION_NONE`
- 大写宏：`TYPEC_ORIENTATION_REVERSE`
- 大写宏：`USB3`
- 大写宏：`USB_TYPEC_DP_SID`
- 外部类型：`Coding`
- 外部类型：`Disable`
- 外部类型：`Enable`
- 外部类型：`Now`
- 外部类型：`Only`
- 外部类型：`Other`
- 外部类型：`PHYs`
- 外部类型：`Phy`
- 外部类型：`Pull`
- 外部类型：`Put`
- 外部类型：`Rx`
- 外部类型：`SerDes`
- 外部类型：`Sublayer`
- 外部类型：`Tx`
- 外部类型：`clk`
- 外部类型：`clk_bulk_data`
- 外部类型：`clk_fixed_rate`
- 外部类型：`clk_hw`
- 外部类型：`device`
- 外部类型：`mutex`
- 外部类型：`of_phandle_args`
- 外部类型：`phy`
- 外部类型：`phy_configure_opts`
- 外部类型：`phy_configure_opts_dp`
- 外部类型：`phy_mode`
- 外部类型：`phy_ops`
- 外部类型：`regulator_bulk_data`
- 外部类型：`reset_control_bulk_data`
- 外部类型：`typec_mux_dev`
- 外部类型：`typec_mux_state`
- 外部类型：`typec_orientation`
- 外部类型：`typec_switch_dev`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-4ecf2549ae` → 本草稿移入 `cases/defect/auto-linux-4ecf2549ae/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
