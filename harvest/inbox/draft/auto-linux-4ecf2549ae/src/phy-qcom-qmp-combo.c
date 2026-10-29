// AUTO-DRAFT from torvalds/linux PR #e191b05f321b064a973399b7cdc003c0de89e5b9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#include <linux/usb/typec.h>
#include <linux/usb/typec_dp.h>
#include <linux/usb/typec_mux.h>

#include <drm/bridge/aux-bridge.h>

/* …（同文件无关代码省略）… */
#define SW_DPPHY_RESET				BIT(0)
/* …（同文件无关代码省略）… */
#define SW_DPPHY_RESET_MUX			BIT(1)
/* …（同文件无关代码省略）… */
#define SW_USB3PHY_RESET			BIT(2)
/* mux to select USB3 PHY reset control, 0:HW control, 1: software reset */
#define SW_USB3PHY_RESET_MUX			BIT(3)

/* QPHY_V3_DP_COM_PHY_MODE_CTRL register bits */
#define USB3_MODE				BIT(0) /* enables USB3 mode */
#define DP_MODE					BIT(1) /* enables DP mode */

/* QPHY_V3_DP_COM_TYPEC_CTRL register bits */
#define SW_PORTSELECT_VAL			BIT(0)
/* …（同文件无关代码省略）… */
#define INVERT_CC_POLARITY			BIT(2)
/* …（同文件无关代码省略）… */
#define PHY_INIT_COMPLETE_TIMEOUT		10000
/* …（同文件无关代码省略）… */
enum qmpphy_mode {
	QMPPHY_MODE_USB3DP = 0,
	QMPPHY_MODE_DP_ONLY,
	QMPPHY_MODE_USB3_ONLY,
};

/* set of registers with offsets different per-PHY */
/* …（同文件无关代码省略）… */
	QPHY_PCS_LFPS_RXTERM_IRQ_CLEAR,
	QPHY_PCS_POWER_DOWN_CONTROL,
	QPHY_PCS_CLAMP_ENABLE,

	QPHY_COM_RESETSM_CNTRL,
	QPHY_COM_C_READY_STATUS,
/* …（同文件无关代码省略）… */
	QMP_PHY_INIT_CFG(QPHY_V10_PCS_USB3_RCVR_DTCT_DLY_U3_H, 0x00),
};

/* list of regulators */
static struct regulator_bulk_data qmp_phy_vreg_l[] = {
	{ .supply = "vdda-phy", .init_load_uA = 21800, },
/* …（同文件无关代码省略）… */
struct qmp_combo_offsets {
	u16 com;
	u16 txa;
	u16 rxa;
	u16 txb;
	u16 rxb;
	u16 usb3_serdes;
	u16 usb3_pcs_misc;
	u16 usb3_pcs;
	u16 usb3_pcs_aon;
	u16 usb3_pcs_usb;
	u16 dp_serdes;
	u16 dp_txa;
	u16 dp_txb;
	u16 dp_dp_phy;
	u16 aon_toggle;
};

struct qmp_phy_cfg {
/* …（同文件无关代码省略）… */
	int (*calibrate_dp_phy)(struct qmp_combo *qmp);
	void (*dp_aux_init)(struct qmp_combo *qmp);

	/* resets to be requested */
	const char * const *reset_list;
	int num_resets;
/* …（同文件无关代码省略）… */
struct qmp_combo {
	struct device *dev;

	const struct qmp_phy_cfg *cfg;

	void __iomem *com;
	void __iomem *aon_toggle;

	void __iomem *serdes;
	void __iomem *tx;
	void __iomem *rx;
	void __iomem *pcs;
	void __iomem *tx2;
	void __iomem *rx2;
	void __iomem *pcs_misc;
	void __iomem *pcs_aon;
	void __iomem *pcs_usb;

	void __iomem *dp_serdes;
	void __iomem *dp_tx;
	void __iomem *dp_tx2;
	void __iomem *dp_dp_phy;

	struct clk *pipe_clk;
	struct clk_bulk_data *clks;
	int num_clks;
	struct reset_control_bulk_data *resets;
	struct regulator_bulk_data *vregs;

	struct mutex phy_mutex;
	int init_count;
	enum qmpphy_mode qmpphy_mode;

	struct phy *usb_phy;
	enum phy_mode phy_mode;
	unsigned int usb_init_count;

	struct phy *dp_phy;
	unsigned int dp_aux_cfg;
	struct phy_configure_opts_dp dp_opts;
	unsigned int dp_init_count;
	bool dp_powered_on;

	struct clk_fixed_rate pipe_clk_fixed;
	struct clk_hw dp_link_hw;
	struct clk_hw dp_pixel_hw;

	struct typec_switch_dev *sw;
	enum typec_orientation orientation;

	struct typec_mux_dev *mux;
};
/* …（同文件无关代码省略）… */
static int qmp_v8_configure_dp_clocks(struct qmp_combo *qmp);
static int qmp_v8_configure_dp_phy(struct qmp_combo *qmp);

static inline void qphy_setbits(void __iomem *base, u32 offset, u32 val)
{
	u32 reg;

	reg = readl(base + offset);
	reg |= val;
	writel(reg, base + offset);

	/* ensure that above write is through */
	readl(base + offset);
}

static inline void qphy_clrbits(void __iomem *base, u32 offset, u32 val)
{
	u32 reg;

	reg = readl(base + offset);
	reg &= ~val;
	writel(reg, base + offset);

	/* ensure that above write is through */
	readl(base + offset);
}

/* list of clocks required by phy */
static const char * const qmp_combo_phy_clk_l[] = {
	"aux", "cfg_ahb", "ref", "com_aux",
/* …（同文件无关代码省略）… */
	.dp_dp_phy	= 0x2200,
};

static const struct qmp_combo_offsets qmp_combo_offsets_v8 = {
	.com		= 0x0000,
	.txa		= 0x1400,
/* …（同文件无关代码省略）… */
	.num_vregs		= ARRAY_SIZE(qmp_phy_vreg_refgen),
};

static int qmp_combo_dp_serdes_init(struct qmp_combo *qmp)
{
	const struct qmp_phy_cfg *cfg = qmp->cfg;
/* …（同文件无关代码省略）… */
static bool qmp_combo_configure_dp_mode(struct qmp_combo *qmp)
{
	bool reverse = (qmp->orientation == TYPEC_ORIENTATION_REVERSE);
	const struct phy_configure_opts_dp *dp_opts = &qmp->dp_opts;
	u32 val;

	val = DP_PHY_PD_CTL_PWRDN | DP_PHY_PD_CTL_AUX_PWRDN |
	      DP_PHY_PD_CTL_PLL_PWRDN | DP_PHY_PD_CTL_DP_CLAMP_EN;

	if (dp_opts->lanes == 4 || reverse)
		val |= DP_PHY_PD_CTL_LANE_0_1_PWRDN;
	if (dp_opts->lanes == 4 || !reverse)
		val |= DP_PHY_PD_CTL_LANE_2_3_PWRDN;

	writel(val, qmp->dp_dp_phy + QSERDES_DP_PHY_PD_CTL);

	if (reverse)
		writel(0x4c, qmp->dp_dp_phy + QSERDES_DP_PHY_MODE);
	else
		writel(0x5c, qmp->dp_dp_phy + QSERDES_DP_PHY_MODE);

	return reverse;
}
/* …（同文件无关代码省略）… */
static int qmp_v8_configure_dp_clocks(struct qmp_combo *qmp)
{
	const struct phy_configure_opts_dp *dp_opts = &qmp->dp_opts;
	u32 phy_vco_div;
	unsigned long pixel_freq;
	const struct qmp_phy_cfg *cfg = qmp->cfg;

	switch (dp_opts->link_rate) {
	case 1620:
		phy_vco_div = 0x4;
		pixel_freq = 1620000000UL / 2;
		break;
	case 2700:
		phy_vco_div = 0x2;
		pixel_freq = 2700000000UL / 2;
		break;
	case 5400:
		phy_vco_div = 0x4;
		pixel_freq = 5400000000UL / 4;
		break;
	case 8100:
		phy_vco_div = 0x3;
		pixel_freq = 8100000000UL / 6;
		break;
	default:
		/* Other link rates aren't supported */
		return -EINVAL;
	}
	writel(phy_vco_div, qmp->dp_dp_phy + cfg->regs[QPHY_DP_PHY_VCO_DIV]);

	/* disable core reset tsync */
	writel(0x09, qmp->dp_dp_phy + QSERDES_DP_PHY_CFG);

	writel(0x04, qmp->dp_dp_phy + QSERDES_V8_DP_PHY_AUXLESS_SETUP_CYC);
	writel(0x08, qmp->dp_dp_phy + QSERDES_V8_DP_PHY_AUXLESS_SILENCE_CYC);
	writel(0x08, qmp->dp_dp_phy + QSERDES_V8_DP_PHY_LFPS_CYC);
	writel(0x11, qmp->dp_dp_phy + QSERDES_V8_DP_PHY_LFPS_PERIOD);

	writel(0x3e, qmp->dp_dp_phy + QSERDES_V8_DP_PHY_TSYNC_OVRD);
	writel(0x05, qmp->dp_dp_phy + QSERDES_V8_DP_PHY_TX2_TX3_LANE_CTL);
	writel(0x05, qmp->dp_dp_phy + QSERDES_V8_DP_PHY_TX0_TX1_LANE_CTL);
	writel(0x01, qmp->dp_dp_phy + QSERDES_V8_DP_PHY_AUXLESS_CFG1);
	writel(0x11, qmp->dp_dp_phy + QSERDES_V8_DP_PHY_LFPS_PERIOD);
	writel(0x1f, qmp->dp_dp_phy + QSERDES_V8_DP_PHY_LN0_DRV_LVL);
	writel(0x1f, qmp->dp_dp_phy + QSERDES_V8_DP_PHY_LN1_DRV_LVL);

	clk_set_rate(qmp->dp_link_hw.clk, dp_opts->link_rate * 100000);
	clk_set_rate(qmp->dp_pixel_hw.clk, pixel_freq);

	return 0;
}
/* …（同文件无关代码省略）… */
static int qmp_v456_configure_dp_phy(struct qmp_combo *qmp)
{
	const struct qmp_phy_cfg *cfg = qmp->cfg;
	u32 status;
	int ret;

	writel(0x0f, qmp->dp_dp_phy + QSERDES_DP_PHY_CFG_1);

	qmp_combo_configure_dp_mode(qmp);

	writel(0x13, qmp->dp_dp_phy + QSERDES_DP_PHY_AUX_CFG1);
	writel(0xa4, qmp->dp_dp_phy + QSERDES_DP_PHY_AUX_CFG2);

	writel(0x05, qmp->dp_dp_phy + QSERDES_V4_DP_PHY_TX0_TX1_LANE_CTL);
	writel(0x05, qmp->dp_dp_phy + QSERDES_V4_DP_PHY_TX2_TX3_LANE_CTL);

	ret = qmp->cfg->configure_dp_clocks(qmp);
	if (ret)
		return ret;

	writel(0x01, qmp->dp_dp_phy + QSERDES_DP_PHY_CFG);
	writel(0x05, qmp->dp_dp_phy + QSERDES_DP_PHY_CFG);
	writel(0x01, qmp->dp_dp_phy + QSERDES_DP_PHY_CFG);
	writel(0x09, qmp->dp_dp_phy + QSERDES_DP_PHY_CFG);

	writel(0x20, qmp->dp_serdes + cfg->regs[QPHY_COM_RESETSM_CNTRL]);

	if (readl_poll_timeout(qmp->dp_serdes + cfg->regs[QPHY_COM_C_READY_STATUS],
			status,
			((status & BIT(0)) > 0),
			500,
			10000))
		return -ETIMEDOUT;

	if (readl_poll_timeout(qmp->dp_serdes + cfg->regs[QPHY_COM_CMN_STATUS],
			status,
			((status & BIT(0)) > 0),
			500,
			10000))
		return -ETIMEDOUT;

	if (readl_poll_timeout(qmp->dp_serdes + cfg->regs[QPHY_COM_CMN_STATUS],
			status,
			((status & BIT(1)) > 0),
			500,
			10000))
		return -ETIMEDOUT;

	writel(0x19, qmp->dp_dp_phy + QSERDES_DP_PHY_CFG);

	if (readl_poll_timeout(qmp->dp_dp_phy + cfg->regs[QPHY_DP_PHY_STATUS],
			status,
			((status & BIT(0)) > 0),
			500,
			10000))
		return -ETIMEDOUT;

	if (readl_poll_timeout(qmp->dp_dp_phy + cfg->regs[QPHY_DP_PHY_STATUS],
			status,
			((status & BIT(1)) > 0),
			500,
			10000))
		return -ETIMEDOUT;

	return 0;
}
/* …（同文件无关代码省略）… */
static int qmp_v8_configure_dp_phy(struct qmp_combo *qmp)
{
	const struct qmp_phy_cfg *cfg = qmp->cfg;
	bool reverse = (qmp->orientation == TYPEC_ORIENTATION_REVERSE);
	const struct phy_configure_opts_dp *dp_opts = &qmp->dp_opts;
	u32 bias0_en, drvr0_en, bias1_en, drvr1_en;
	u32 status;
	int ret;

	ret = qmp_v456_configure_dp_phy(qmp);
	if (ret < 0)
		return ret;

	if (dp_opts->lanes == 1) {
		bias0_en = reverse ? 0x3e : 0x15;
		bias1_en = reverse ? 0x15 : 0x3e;
		drvr0_en = reverse ? 0x13 : 0x10;
		drvr1_en = reverse ? 0x10 : 0x13;
	} else if (dp_opts->lanes == 2) {
		bias0_en = reverse ? 0x3f : 0x15;
		bias1_en = reverse ? 0x15 : 0x3f;
		drvr0_en = 0x10;
		drvr1_en = 0x10;
	} else {
		bias0_en = 0x3f;
		bias1_en = 0x3f;
		drvr0_en = 0x34;
		drvr1_en = 0x34;
	}

	writel(drvr0_en, qmp->dp_tx + cfg->regs[QPHY_TX_HIGHZ_DRVR_EN]);
	writel(bias0_en, qmp->dp_tx + cfg->regs[QPHY_TX_TRANSCEIVER_BIAS_EN]);
	writel(drvr1_en, qmp->dp_tx2 + cfg->regs[QPHY_TX_HIGHZ_DRVR_EN]);
	writel(bias1_en, qmp->dp_tx2 + cfg->regs[QPHY_TX_TRANSCEIVER_BIAS_EN]);

	writel(0x08, qmp->dp_dp_phy + QSERDES_DP_PHY_CFG);
	udelay(100);
	writel(0x09, qmp->dp_dp_phy + QSERDES_DP_PHY_CFG);
	udelay(500);

	if (readl_poll_timeout(qmp->dp_dp_phy + cfg->regs[QPHY_DP_PHY_STATUS],
			status,
			((status & BIT(1)) > 0),
			500,
			10000))
		return -ETIMEDOUT;

	writel(0x00, qmp->dp_tx + cfg->regs[QPHY_TX_TX_DRV_LVL]);
	writel(0x00, qmp->dp_tx2 + cfg->regs[QPHY_TX_TX_DRV_LVL]);

	writel(0x2b, qmp->dp_tx + cfg->regs[QPHY_TX_TX_EMP_POST1_LVL]);
	writel(0x2b, qmp->dp_tx2 + cfg->regs[QPHY_TX_TX_EMP_POST1_LVL]);

	return 0;
}
/* …（同文件无关代码省略）… */
static int qmp_combo_dp_configure(struct phy *phy, union phy_configure_opts *opts)
{
	const struct phy_configure_opts_dp *dp_opts = &opts->dp;
	struct qmp_combo *qmp = phy_get_drvdata(phy);
	const struct qmp_phy_cfg *cfg = qmp->cfg;

	mutex_lock(&qmp->phy_mutex);

	memcpy(&qmp->dp_opts, dp_opts, sizeof(*dp_opts));
	if (qmp->dp_opts.set_voltages) {
		cfg->configure_dp_tx(qmp);
		qmp->dp_opts.set_voltages = 0;
	}

	mutex_unlock(&qmp->phy_mutex);

	return 0;
}
/* …（同文件无关代码省略）… */
static int qmp_combo_com_init(struct qmp_combo *qmp, bool force)
{
	const struct qmp_phy_cfg *cfg = qmp->cfg;
	void __iomem *com = qmp->com;
	void __iomem *aon_toggle;
	int ret;
/* …（同文件无关代码省略）… */
	else
		aon_toggle = qmp->pcs_aon;

	ret = regulator_bulk_enable(cfg->num_vregs, qmp->vregs);
	if (ret) {
		dev_err(qmp->dev, "failed to enable regulators, err=%d\n", ret);
/* …（同文件无关代码省略）… */

	/* override hardware control for reset of qmp phy */
	qphy_setbits(com, QPHY_V3_DP_COM_RESET_OVRD_CTRL,
			SW_DPPHY_RESET_MUX | SW_DPPHY_RESET |
			SW_USB3PHY_RESET_MUX | SW_USB3PHY_RESET);

	/* override hardware control for reset of qmp phy */
	if (aon_toggle && cfg->regs[QPHY_AON_TOGGLE_ENABLE]) {
/* …（同文件无关代码省略）… */
	if (cfg->invert_cc_polarity)
		val |= INVERT_CC_POLARITY;

	writel(val, com + QPHY_V3_DP_COM_TYPEC_CTRL);

	switch (qmp->qmpphy_mode) {
/* …（同文件无关代码省略）… */
		qphy_clrbits(com, QPHY_V3_DP_COM_RESET_OVRD_CTRL,
				SW_USB3PHY_RESET_MUX | SW_USB3PHY_RESET);
		break;
	}

	qphy_clrbits(com, QPHY_V3_DP_COM_SWI_CTRL, 0x03);
	qphy_clrbits(com, QPHY_V3_DP_COM_SW_RESET, SW_RESET);

	qphy_setbits(qmp->pcs, cfg->regs[QPHY_PCS_POWER_DOWN_CONTROL],
			SW_PWRDN);

	return 0;
/* …（同文件无关代码省略）… */
static int qmp_combo_com_exit(struct qmp_combo *qmp, bool force)
{
	const struct qmp_phy_cfg *cfg = qmp->cfg;

	if (!force && --qmp->init_count)
		return 0;

	reset_control_bulk_assert(cfg->num_resets, qmp->resets);

	clk_disable_unprepare(qmp->pipe_clk);
	clk_bulk_disable_unprepare(qmp->num_clks, qmp->clks);

	regulator_bulk_disable(cfg->num_vregs, qmp->vregs);

	return 0;
}
/* …（同文件无关代码省略）… */
static int qmp_combo_dp_init(struct phy *phy)
{
	struct qmp_combo *qmp = phy_get_drvdata(phy);
	const struct qmp_phy_cfg *cfg = qmp->cfg;
	int ret;

	mutex_lock(&qmp->phy_mutex);

	ret = qmp_combo_com_init(qmp, false);
	if (ret)
		goto out_unlock;

	cfg->dp_aux_init(qmp);

	qmp->dp_init_count++;

out_unlock:
	mutex_unlock(&qmp->phy_mutex);
	return ret;
}
/* …（同文件无关代码省略）… */
static int qmp_combo_usb_power_on(struct phy *phy)
{
	struct qmp_combo *qmp = phy_get_drvdata(phy);
	const struct qmp_phy_cfg *cfg = qmp->cfg;
	void __iomem *serdes = qmp->serdes;
	void __iomem *tx = qmp->tx;
	void __iomem *rx = qmp->rx;
	void __iomem *tx2 = qmp->tx2;
	void __iomem *rx2 = qmp->rx2;
	void __iomem *pcs = qmp->pcs;
	void __iomem *pcs_usb = qmp->pcs_usb;
	void __iomem *status;
	unsigned int val;
	int ret;

	qmp_configure(qmp->dev, serdes, cfg->serdes_tbl, cfg->serdes_tbl_num);

	/* Tx, Rx, and PCS configurations */
	qmp_configure_lane(qmp->dev, tx, cfg->tx_tbl, cfg->tx_tbl_num, 1);
	qmp_configure_lane(qmp->dev, tx2, cfg->tx_tbl, cfg->tx_tbl_num, 2);

	qmp_configure_lane(qmp->dev, rx, cfg->rx_tbl, cfg->rx_tbl_num, 1);
	qmp_configure_lane(qmp->dev, rx2, cfg->rx_tbl, cfg->rx_tbl_num, 2);

	qmp_configure(qmp->dev, pcs, cfg->pcs_tbl, cfg->pcs_tbl_num);
	qmp_configure(qmp->dev, qmp->pcs_misc, cfg->pcs_misc_tbl, cfg->pcs_misc_tbl_num);


	if (pcs_usb)
		qmp_configure(qmp->dev, pcs_usb, cfg->pcs_usb_tbl,
			      cfg->pcs_usb_tbl_num);

	if (cfg->has_pwrdn_delay)
		usleep_range(10, 20);

	/* Pull PHY out of reset state */
	qphy_clrbits(pcs, cfg->regs[QPHY_SW_RESET], SW_RESET);

	/* start SerDes and Phy-Coding-Sublayer */
	qphy_setbits(pcs, cfg->regs[QPHY_START_CTRL], SERDES_START | PCS_START);

	status = pcs + cfg->regs[QPHY_PCS_STATUS];
	ret = readl_poll_timeout(status, val, !(val & PHYSTATUS), 200,
			PHY_INIT_COMPLETE_TIMEOUT);
	if (ret) {
		dev_err(qmp->dev, "phy initialization timed-out\n");
		goto err_disable_pipe_clk;
	}

	return 0;

err_disable_pipe_clk:
	clk_disable_unprepare(qmp->pipe_clk);

	return ret;
}
/* …（同文件无关代码省略）… */
static int qmp_combo_usb_power_off(struct phy *phy)
{
	struct qmp_combo *qmp = phy_get_drvdata(phy);
	const struct qmp_phy_cfg *cfg = qmp->cfg;

	/* PHY reset */
	qphy_setbits(qmp->pcs, cfg->regs[QPHY_SW_RESET], SW_RESET);

	/* stop SerDes and Phy-Coding-Sublayer */
	qphy_clrbits(qmp->pcs, cfg->regs[QPHY_START_CTRL],
			SERDES_START | PCS_START);

	/* Put PHY into POWER DOWN state: active low */
	qphy_clrbits(qmp->pcs, cfg->regs[QPHY_PCS_POWER_DOWN_CONTROL],
			SW_PWRDN);

	return 0;
}
/* …（同文件无关代码省略）… */
static int qmp_combo_usb_init(struct phy *phy)
{
	struct qmp_combo *qmp = phy_get_drvdata(phy);
	int ret;

	mutex_lock(&qmp->phy_mutex);
	ret = qmp_combo_com_init(qmp, false);
	if (ret)
		goto out_unlock;

	ret = qmp_combo_usb_power_on(phy);
	if (ret) {
		qmp_combo_com_exit(qmp, false);
		goto out_unlock;
	}

	qmp->usb_init_count++;

out_unlock:
	mutex_unlock(&qmp->phy_mutex);
	return ret;
}

static int qmp_combo_usb_exit(struct phy *phy)
{
	struct qmp_combo *qmp = phy_get_drvdata(phy);
	int ret;

	mutex_lock(&qmp->phy_mutex);
	ret = qmp_combo_usb_power_off(phy);
	if (ret)
		goto out_unlock;

	ret = qmp_combo_com_exit(qmp, false);
	if (ret)
		goto out_unlock;

	qmp->usb_init_count--;

out_unlock:
	mutex_unlock(&qmp->phy_mutex);
	return ret;
}

static int qmp_combo_usb_set_mode(struct phy *phy, enum phy_mode mode, int submode)
{
	struct qmp_combo *qmp = phy_get_drvdata(phy);

	qmp->phy_mode = mode;

	return 0;
}

static const struct phy_ops qmp_combo_usb_phy_ops = {
	.init		= qmp_combo_usb_init,
	.exit		= qmp_combo_usb_exit,
	.set_mode	= qmp_combo_usb_set_mode,
	.owner		= THIS_MODULE,
};

static const struct phy_ops qmp_combo_dp_phy_ops = {
	.init		= qmp_combo_dp_init,
	.configure	= qmp_combo_dp_configure,
/* …（同文件无关代码省略）… */
	 * Enable i/o clamp_n for autonomous mode
	 * V6 and later versions use pcs aon clamp register
	 */
	if (pcs_aon)
		qphy_clrbits(pcs_aon, cfg->regs[QPHY_PCS_CLAMP_ENABLE], CLAMP_EN);
	else if (pcs_misc)
		qphy_clrbits(pcs_misc, cfg->regs[QPHY_PCS_CLAMP_ENABLE], CLAMP_EN);
}

/* …（同文件无关代码省略）… */
	void __iomem *pcs_aon = qmp->pcs_aon;

	/* Disable i/o clamp_n on resume for normal mode */
	if (pcs_aon)
		qphy_setbits(pcs_aon, cfg->regs[QPHY_PCS_CLAMP_ENABLE], CLAMP_EN);
	else if (pcs_misc)
		qphy_setbits(pcs_misc, cfg->regs[QPHY_PCS_CLAMP_ENABLE], CLAMP_EN);

	qphy_clrbits(pcs_usb, cfg->regs[QPHY_PCS_AUTONOMOUS_MODE_CTRL],
/* …（同文件无关代码省略）… */
static int qmp_combo_clk_init(struct qmp_combo *qmp)
{
	struct device *dev = qmp->dev;
	int num = ARRAY_SIZE(qmp_combo_phy_clk_l);
	int i;

	qmp->clks = devm_kcalloc(dev, num, sizeof(*qmp->clks), GFP_KERNEL);
	if (!qmp->clks)
		return -ENOMEM;

	for (i = 0; i < num; i++)
		qmp->clks[i].id = qmp_combo_phy_clk_l[i];

	qmp->num_clks = num;

	return devm_clk_bulk_get_optional(dev, num, qmp->clks);
}
/* …（同文件无关代码省略）… */
	struct qmp_combo *qmp = typec_switch_get_drvdata(sw);
	const struct qmp_phy_cfg *cfg = qmp->cfg;

	if (orientation == qmp->orientation || orientation == TYPEC_ORIENTATION_NONE)
		return 0;

	mutex_lock(&qmp->phy_mutex);
	qmp->orientation = orientation;

	if (qmp->init_count) {
		if (qmp->usb_init_count)
			qmp_combo_usb_power_off(qmp->usb_phy);
		qmp_combo_com_exit(qmp, true);

		qmp_combo_com_init(qmp, true);
		if (qmp->usb_init_count)
			qmp_combo_usb_power_on(qmp->usb_phy);
		if (qmp->dp_init_count)
			cfg->dp_aux_init(qmp);
	}
	mutex_unlock(&qmp->phy_mutex);

	return 0;
}

static int qmp_combo_typec_mux_set(struct typec_mux_dev *mux, struct typec_mux_state *state)
{
	struct qmp_combo *qmp = typec_mux_get_drvdata(mux);
	const struct qmp_phy_cfg *cfg = qmp->cfg;
	enum qmpphy_mode new_mode;
	unsigned int svid;

/* …（同文件无关代码省略）… */
	else
		svid = 0;

	if (svid == USB_TYPEC_DP_SID) {
		switch (state->mode) {
		/* DP Only */
/* …（同文件无关代码省略）… */
		return 0;
	}

	dev_dbg(qmp->dev, "typec_mux_set: switching from qmpphy mode %d to %d\n",
		qmp->qmpphy_mode, new_mode);

	qmp->qmpphy_mode = new_mode;

	if (qmp->init_count) {
		if (qmp->usb_init_count)
			qmp_combo_usb_power_off(qmp->usb_phy);

		if (qmp->dp_init_count)
			writel(DP_PHY_PD_CTL_PSR_PWRDN, qmp->dp_dp_phy + QSERDES_DP_PHY_PD_CTL);

		qmp_combo_com_exit(qmp, true);

		/* Now everything's powered down, power up the right PHYs */
		qmp_combo_com_init(qmp, true);

		if (new_mode == QMPPHY_MODE_DP_ONLY) {
			if (qmp->usb_init_count)
				qmp->usb_init_count--;
		}

		if (new_mode == QMPPHY_MODE_USB3DP || new_mode == QMPPHY_MODE_USB3_ONLY) {
			qmp_combo_usb_power_on(qmp->usb_phy);
			if (!qmp->usb_init_count)
				qmp->usb_init_count++;
		}

		if (new_mode == QMPPHY_MODE_DP_ONLY || new_mode == QMPPHY_MODE_USB3DP) {
			if (qmp->dp_init_count)
				cfg->dp_aux_init(qmp);
		}
	}

	return 0;
}

static void qmp_combo_typec_switch_unregister(void *data)
{
	struct qmp_combo *qmp = data;

	typec_switch_unregister(qmp->sw);
}
/* …（同文件无关代码省略）… */
	if (offs->aon_toggle)
		qmp->aon_toggle = base + offs->aon_toggle;

	ret = qmp_combo_clk_init(qmp);
	if (ret)
		return ret;
/* …（同文件无关代码省略）… */
				"failed to get usb3_pipe clock\n");
	}

	return 0;
}

static struct phy *qmp_combo_phy_xlate(struct device *dev, const struct of_phandle_args *args)
{
	struct qmp_combo *qmp = dev_get_drvdata(dev);

	if (args->args_count == 0)
		return ERR_PTR(-EINVAL);

	switch (args->args[0]) {
	case QMP_USB43DP_USB3_PHY:
		return qmp->usb_phy;
	case QMP_USB43DP_DP_PHY:
		return qmp->dp_phy;
	}

	return ERR_PTR(-EINVAL);
}
/* …（同文件无关代码省略）… */

	phy_set_drvdata(qmp->dp_phy, qmp);

	if (usb_np == dev->of_node)
		phy_provider = devm_of_phy_provider_register(dev, qmp_combo_phy_xlate);
	else
/* …（同文件无关代码省略）… */
		.compatible = "qcom,hawi-qmp-usb3-dp-phy",
		.data = &hawi_usb3dpphy_cfg,
	},
	{
		.compatible = "qcom,sar2130p-qmp-usb3-dp-phy",
		.data = &sar2130p_usb3dpphy_cfg,
