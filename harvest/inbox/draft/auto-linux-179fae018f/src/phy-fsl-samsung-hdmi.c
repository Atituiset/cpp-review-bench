// AUTO-DRAFT from torvalds/linux PR #e191b05f321b064a973399b7cdc003c0de89e5b9
#define PHY_REG(reg)		(reg * 4)
/* …（同文件无关代码省略）… */
#define REG12_CK_DIV_MASK	GENMASK(5, 4)
/* …（同文件无关代码省略）… */
#define REG13_TG_CODE_LOW_MASK	GENMASK(7, 0)
/* …（同文件无关代码省略）… */
#define REG14_TOL_MASK		GENMASK(7, 4)
#define REG14_RP_CODE_MASK	GENMASK(3, 1)
#define REG14_TG_CODE_HIGH_MASK	GENMASK(0, 0)
/* …（同文件无关代码省略）… */
#define REG21_SEL_TX_CK_INV	BIT(7)
#define REG21_PMS_S_MASK	GENMASK(3, 0)
/* …（同文件无关代码省略）… */
#define REG33_MODE_SET_DONE	BIT(7)
#define REG33_FIX_DA		BIT(1)
/* …（同文件无关代码省略）… */
#define REG34_PLL_LOCK		BIT(6)
/* …（同文件无关代码省略）… */
#define MHZ	(1000UL * 1000UL)
#endif
  // <<< BUG ANCHOR
#define PHY_PLL_DIV_REGS_NUM 7

struct phy_config {
/* …（同文件无关代码省略）… */
struct fsl_samsung_hdmi_phy {
	struct device *dev;
	void __iomem *regs;
	struct clk *apbclk;
	struct clk *refclk;

	/* clk provider */
	struct clk_hw hw;
	const struct phy_config *cur_cfg;
};
/* …（同文件无关代码省略）… */
static inline struct fsl_samsung_hdmi_phy *
to_fsl_samsung_hdmi_phy(struct clk_hw *hw)
{
	return container_of(hw, struct fsl_samsung_hdmi_phy, hw);
}
/* …（同文件无关代码省略）… */
static int
fsl_samsung_hdmi_phy_configure_pll_lock_det(struct fsl_samsung_hdmi_phy *phy,
					    const struct phy_config *cfg)
{
	u32 pclk = cfg->pixclk;
	u32 fld_tg_code;
	u32 int_pllclk;
	u8 div;

	/* Find int_pllclk speed */
	for (div = 0; div < 4; div++) {
		int_pllclk = pclk / (1 << div);
		if (int_pllclk < (50 * MHZ))
			break;
	}

	if (unlikely(div == 4))
		return -EINVAL;

	writeb(FIELD_PREP(REG12_CK_DIV_MASK, div), phy->regs + PHY_REG(12));

	/*
	 * Calculation for the frequency lock detector target code (fld_tg_code)
	 * is based on reference manual register description of PHY_REG13
	 * (13.10.3.1.14.2):
	 *   1st) Calculate int_pllclk which is determinded by FLD_CK_DIV
	 *   2nd) Increase resolution to avoid rounding issues
	 *   3th) Do the div (256 / Freq. of int_pllclk) * 24
	 *   4th) Reduce the resolution and always round up since the NXP
	 *        settings rounding up always too. TODO: Check if that is
	 *        correct.
	 */

	fld_tg_code =  DIV_ROUND_UP(24 * MHZ * 256, int_pllclk);

	/* FLD_TOL and FLD_RP_CODE taken from downstream driver */
	writeb(FIELD_PREP(REG13_TG_CODE_LOW_MASK, fld_tg_code),
	       phy->regs + PHY_REG(13));
	writeb(FIELD_PREP(REG14_TOL_MASK, 2) |
	       FIELD_PREP(REG14_RP_CODE_MASK, 2) |
	       FIELD_PREP(REG14_TG_CODE_HIGH_MASK, fld_tg_code >> 8),
	       phy->regs + PHY_REG(14));

	return 0;
}
/* …（同文件无关代码省略）… */
static int fsl_samsung_hdmi_phy_configure(struct fsl_samsung_hdmi_phy *phy,
					  const struct phy_config *cfg)
{
	int i, ret;
	u8 val;

	phy->cur_cfg = cfg;

	/* HDMI PHY init */
	writeb(REG33_FIX_DA, phy->regs + PHY_REG(33));

	/* common PHY registers */
	for (i = 0; i < ARRAY_SIZE(common_phy_cfg); i++)
		writeb(common_phy_cfg[i].val, phy->regs + common_phy_cfg[i].reg);

	/* set individual PLL registers PHY_REG1 ... PHY_REG7 */
	for (i = 0; i < PHY_PLL_DIV_REGS_NUM; i++)
		writeb(cfg->pll_div_regs[i], phy->regs + PHY_REG(1) + i * 4);

	/* High nibble of PHY_REG3 and low nibble of PHY_REG21 both contain 'S' */
	writeb(REG21_SEL_TX_CK_INV | FIELD_PREP(REG21_PMS_S_MASK,
	       cfg->pll_div_regs[2] >> 4), phy->regs + PHY_REG(21));

	ret = fsl_samsung_hdmi_phy_configure_pll_lock_det(phy, cfg);
	if (ret) {
		dev_err(phy->dev, "pixclock too large\n");
		return ret;
	}

	writeb(REG33_FIX_DA | REG33_MODE_SET_DONE, phy->regs + PHY_REG(33));

	ret = readb_poll_timeout(phy->regs + PHY_REG(34), val,
				 val & REG34_PLL_LOCK, 50, 20000);
	if (ret)
		dev_err(phy->dev, "PLL failed to lock\n");

	return ret;
}

static unsigned long phy_clk_recalc_rate(struct clk_hw *hw,
					 unsigned long parent_rate)
{
	struct fsl_samsung_hdmi_phy *phy = to_fsl_samsung_hdmi_phy(hw);

	if (!phy->cur_cfg)
		return 74250000;

	return phy->cur_cfg->pixclk;
}

/* Helper function to lookup the available fractional-divider rate */
static const struct phy_config *fsl_samsung_hdmi_phy_lookup_rate(unsigned long rate)
{
/* …（同文件无关代码省略）… */
		&phy_pll_cfg[i] : &phy_pll_cfg[i+1]);
}

static void fsl_samsung_hdmi_calculate_phy(struct phy_config *cal_phy, unsigned long rate,
				    u8 p, u16 m, u8 s)
{
/* …（同文件无关代码省略）… */
{
	struct device *dev = phy->dev;
	struct device_node *np = dev->of_node;
	struct clk_init_data init;
	const char *parent_name;
	struct clk *phyclk;
	int ret;
/* …（同文件无关代码省略）… */

static int fsl_samsung_hdmi_phy_probe(struct platform_device *pdev)
{
	struct fsl_samsung_hdmi_phy *phy;
	int ret;

/* …（同文件无关代码省略）… */
		return dev_err_probe(phy->dev, PTR_ERR(phy->refclk),
				     "failed to get ref clk\n");

	pm_runtime_get_noresume(phy->dev);
	pm_runtime_set_active(phy->dev);
	pm_runtime_enable(phy->dev);
/* …（同文件无关代码省略）… */
static int __maybe_unused fsl_samsung_hdmi_phy_resume(struct device *dev)
{
	struct fsl_samsung_hdmi_phy *phy = dev_get_drvdata(dev);
	int ret = 0;

	ret = clk_prepare_enable(phy->apbclk);
	if (ret) {
		dev_err(phy->dev, "failed to enable apbclk\n");
		return ret;
	}

	if (phy->cur_cfg)
		ret = fsl_samsung_hdmi_phy_configure(phy, phy->cur_cfg);

	return ret;

}

static DEFINE_RUNTIME_DEV_PM_OPS(fsl_samsung_hdmi_phy_pm_ops,
