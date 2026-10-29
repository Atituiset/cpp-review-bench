// AUTO-DRAFT from torvalds/linux PR #e191b05f321b064a973399b7cdc003c0de89e5b9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <time.h>
  // <<< BUG ANCHOR
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
/* …（同文件无关代码省略）… */
#include <linux/slab.h>

#include "phy-qcom-qmp.h"

#define PHY_INIT_COMPLETE_TIMEOUT_US		10000

/* …（同文件无关代码省略）… */
	QMP_PCIE_GLYMUR_MODE_X4X4,
};

enum qphy_reg_layout {
	QPHY_PCS_STATUS,
	QPHY_LAYOUT_SIZE
};

static const unsigned int pciephy_v8_50_regs_layout[QPHY_LAYOUT_SIZE] = {
	[QPHY_PCS_STATUS]		= QPHY_V8_50_PCS_STATUS1,
};

struct qmp_pcie_offsets {
	u16 pcs;
};

struct qmp_phy_cfg {
	const struct qmp_pcie_offsets *offsets;
	const char * const *reg_names;
	int num_regs;
	const char * const *pd_names;
	int num_pds;
	const char * const *nocsr_reset_list;
	int num_nocsr_resets;
	const char * const *vreg_list;
/* …（同文件无关代码省略）… */
struct qmp_pcie {
	struct device *dev;
	const struct qmp_phy_cfg *cfg;
	void __iomem **base;
	struct clk_bulk_data *clks;
	struct clk_bulk_data *pipe_clks;
	struct reset_control_bulk_data *nocsr_resets;
	struct regulator_bulk_data *vregs;
	struct device **pd_devs;
/* …（同文件无关代码省略）… */
	u32 num_modes;
};

static const char * const glymur_pciephy_clk_l[] = {
	"aux", "cfg_ahb", "ref", "rchng", "phy_b_aux",
};
/* …（同文件无关代码省略）… */
	"vdda-phy", "vdda-pll", "vdda-refgen0p9", "vdda-refgen1p2",
};

static const char * const glymur_pciephy_a_reg_l[] = {
	"port_a",
};

static const char * const glymur_pciephy_b_reg_l[] = {
	"port_b",
};

static const char * const glymur_pciephy_reg_l[] = {
	"port_a", "port_b",
};

static const char * const glymur_pciephy_a_pd_l[] = {
	"port_a",
};

static const char * const glymur_pciephy_b_pd_l[] = {
	"port_b",
};

static const char * const glymur_pciephy_pd_l[] = {
	"port_a", "port_b",
};

static const char * const glymur_pciephy_a_nocsr_reset_l[] = {
	"port_a_nocsr",
};

static const char * const glymur_pciephy_nocsr_reset_l[] = {
	"port_a_nocsr", "port_b_nocsr",
};

static const char * const glymur_pciephy_b_nocsr_reset_l[] = {
	"port_b_nocsr",
};

/* …（同文件无关代码省略）… */

static const struct qmp_phy_cfg glymur_qmp_gen5x4_pciephy_a_cfg = {
	.offsets		= &glymur_pcie_offsets_v8_50,
	.reg_names		= glymur_pciephy_a_reg_l,
	.num_regs		= ARRAY_SIZE(glymur_pciephy_a_reg_l),
	.pd_names		= glymur_pciephy_a_pd_l,
	.num_pds		= ARRAY_SIZE(glymur_pciephy_a_pd_l),
	.nocsr_reset_list	= glymur_pciephy_a_nocsr_reset_l,
	.num_nocsr_resets	= ARRAY_SIZE(glymur_pciephy_a_nocsr_reset_l),
	.vreg_list		= glymur_vreg_l,
	.num_vregs		= ARRAY_SIZE(glymur_vreg_l),
	.regs			= pciephy_v8_50_regs_layout,
/* …（同文件无关代码省略）… */

static const struct qmp_phy_cfg glymur_qmp_gen5x4_pciephy_b_cfg = {
	.offsets		= &glymur_pcie_offsets_v8_50,
	.reg_names		= glymur_pciephy_b_reg_l,
	.num_regs		= ARRAY_SIZE(glymur_pciephy_b_reg_l),
	.pd_names		= glymur_pciephy_b_pd_l,
	.num_pds		= ARRAY_SIZE(glymur_pciephy_b_pd_l),
	.nocsr_reset_list	= glymur_pciephy_b_nocsr_reset_l,
	.num_nocsr_resets	= ARRAY_SIZE(glymur_pciephy_b_nocsr_reset_l),
	.vreg_list		= glymur_vreg_l,
	.num_vregs		= ARRAY_SIZE(glymur_vreg_l),
	.regs			= pciephy_v8_50_regs_layout,
/* …（同文件无关代码省略）… */

static const struct qmp_phy_cfg glymur_qmp_gen5x8_pciephy_cfg = {
	.offsets		= &glymur_pcie_offsets_v8_50,
	.reg_names		= glymur_pciephy_reg_l,
	.num_regs		= ARRAY_SIZE(glymur_pciephy_reg_l),
	.pd_names		= glymur_pciephy_pd_l,
	.num_pds		= ARRAY_SIZE(glymur_pciephy_pd_l),
	.nocsr_reset_list	= glymur_pciephy_nocsr_reset_l,
	.num_nocsr_resets	= ARRAY_SIZE(glymur_pciephy_nocsr_reset_l),
	.vreg_list		= glymur_vreg_l,
/* …（同文件无关代码省略）… */
	.num_modes	= ARRAY_SIZE(glymur_qmp_gen5x8_mode_cfgs),
};

static int qmp_pcie_pd_power_on(struct qmp_pcie *qmp)
{
	const struct qmp_phy_cfg *cfg = qmp->cfg;
/* …（同文件无关代码省略）… */
{
	struct qmp_pcie *qmp = phy_get_drvdata(phy);
	const struct qmp_phy_cfg *cfg = qmp->cfg;
	int ret;

	ret = qmp_pcie_pd_power_on(qmp);
	if (ret)
		return ret;

	ret = regulator_bulk_enable(cfg->num_vregs, qmp->vregs);
	if (ret) {
		dev_err(qmp->dev, "failed to enable regulators: %d\n", ret);
		goto err_pd_power_off;
	}

	ret = reset_control_bulk_assert(qmp->cfg->num_nocsr_resets, qmp->nocsr_resets);
	if (ret) {
		dev_err(qmp->dev, "no-csr reset assert failed: %d\n", ret);
		goto err_disable_regulators;
	}

	usleep_range(200, 300);

	ret = clk_bulk_prepare_enable(qmp->cfg->num_clks, qmp->clks);
	if (ret)
		goto err_disable_regulators;

	return 0;

err_disable_regulators:
	regulator_bulk_disable(cfg->num_vregs, qmp->vregs);
err_pd_power_off:
/* …（同文件无关代码省略）… */
	struct qmp_pcie *qmp = phy_get_drvdata(phy);
	const struct qmp_phy_cfg *cfg = qmp->cfg;

	reset_control_bulk_assert(qmp->cfg->num_nocsr_resets, qmp->nocsr_resets);

	clk_bulk_disable_unprepare(qmp->cfg->num_clks, qmp->clks);
	regulator_bulk_disable(cfg->num_vregs, qmp->vregs);
/* …（同文件无关代码省略）… */
		goto err_disable_pipe_clk;
	}

	for (i = 0; i < cfg->num_regs; i++) {
		status = qmp->base[i] + offs->pcs + cfg->regs[QPHY_PCS_STATUS];
		ret = readl_poll_timeout(status, val, !(val & cfg->phy_status), 200,
/* …（同文件无关代码省略）… */
static void qmp_pcie_pd_detach(void *data)
{
	struct qmp_pcie *qmp = data;
	const struct qmp_phy_cfg *cfg = qmp->cfg;
	int i;

	for (i = 0; i < cfg->num_pds; i++) {
		if (!IS_ERR_OR_NULL(qmp->pd_devs[i]))
			dev_pm_domain_detach(qmp->pd_devs[i], true);
	}
}

static int qmp_pcie_pd_init(struct qmp_pcie *qmp)
{
	const struct qmp_phy_cfg *cfg = qmp->cfg;
	struct device *dev = qmp->dev;
	int i, ret;

	if (!cfg->num_pds)
		return 0;

	qmp->pd_devs = devm_kcalloc(dev, cfg->num_pds, sizeof(*qmp->pd_devs),
				    GFP_KERNEL);
	if (!qmp->pd_devs)
		return -ENOMEM;

	for (i = 0; i < cfg->num_pds; i++) {
		qmp->pd_devs[i] = dev_pm_domain_attach_by_name(dev,
							       cfg->pd_names[i]);
		if (IS_ERR_OR_NULL(qmp->pd_devs[i])) {
			ret = PTR_ERR(qmp->pd_devs[i]) ? : -ENODATA;
			goto err_detach;
		}
	}

	return devm_add_action_or_reset(dev, qmp_pcie_pd_detach, qmp);

err_detach:
	while (--i >= 0)
		dev_pm_domain_detach(qmp->pd_devs[i], false);

	return ret;
}

static int qmp_pcie_vreg_init(struct qmp_pcie *qmp)
{
	const struct qmp_phy_cfg *cfg = qmp->cfg;
	struct device *dev = qmp->dev;
	int i;

	qmp->vregs = devm_kcalloc(dev, cfg->num_vregs, sizeof(*qmp->vregs),
				  GFP_KERNEL);
	if (!qmp->vregs)
		return -ENOMEM;

	for (i = 0; i < cfg->num_vregs; i++)
		qmp->vregs[i].supply = cfg->vreg_list[i];

	return devm_regulator_bulk_get(dev, cfg->num_vregs, qmp->vregs);
}
/* …（同文件无关代码省略）… */
static int qmp_pcie_reset_init(struct qmp_pcie *qmp)
{
	const struct qmp_phy_cfg *cfg = qmp->cfg;
	struct device *dev = qmp->dev;
	int i, ret;

	qmp->nocsr_resets = devm_kcalloc(dev, cfg->num_nocsr_resets,
					 sizeof(*qmp->nocsr_resets),
					 GFP_KERNEL);
	if (!qmp->nocsr_resets)
		return -ENOMEM;

	for (i = 0; i < cfg->num_nocsr_resets; i++)
		qmp->nocsr_resets[i].id = cfg->nocsr_reset_list[i];

	ret = devm_reset_control_bulk_get_exclusive(dev,
						    cfg->num_nocsr_resets,
						    qmp->nocsr_resets);
	if (ret)
		return dev_err_probe(dev, ret, "failed to get no-csr resets\n");

	return 0;
}
/* …（同文件无关代码省略）… */
static int qmp_pcie_clk_init(struct qmp_pcie *qmp)
{
	const struct qmp_phy_cfg *cfg = qmp->cfg;
	struct device *dev = qmp->dev;
	int i, ret;

	qmp->clks = devm_kcalloc(dev, cfg->num_clks, sizeof(*qmp->clks),
				 GFP_KERNEL);
	if (!qmp->clks)
		return -ENOMEM;

	for (i = 0; i < cfg->num_clks; i++)
		qmp->clks[i].id = cfg->clk_list[i];

	ret = devm_clk_bulk_get_optional(dev, cfg->num_clks, qmp->clks);
	if (ret)
		return ret;

	qmp->pipe_clks = devm_kcalloc(dev, cfg->num_pipe_clks,
				      sizeof(*qmp->pipe_clks), GFP_KERNEL);
	if (!qmp->pipe_clks)
		return -ENOMEM;

	for (i = 0; i < cfg->num_pipe_clks; i++)
		qmp->pipe_clks[i].id = cfg->pipe_clk_list[i];

	return devm_clk_bulk_get_optional(dev, cfg->num_pipe_clks,
					  qmp->pipe_clks);
}
/* …（同文件无关代码省略）… */
	int i, ret;

	num_pipe_outputs = of_property_count_strings(np, "clock-output-names");

	qmp_data->num_pipe_outputs = num_pipe_outputs;
	qmp_data->pipe_out_clks = devm_kcalloc(dev, num_pipe_outputs,
/* …（同文件无关代码省略）… */
static int qmp_pcie_get_mmio(struct qmp_pcie *qmp)
{
	struct platform_device *pdev = to_platform_device(qmp->dev);
	const struct qmp_phy_cfg *cfg = qmp->cfg;
	struct device *dev = qmp->dev;
	void __iomem *base;
	int i;

	qmp->base = devm_kcalloc(dev, cfg->num_regs, sizeof(*qmp->base),
				 GFP_KERNEL);
	if (!qmp->base)
		return -ENOMEM;

	for (i = 0; i < cfg->num_regs; i++) {
		base = devm_platform_ioremap_resource_byname(pdev, cfg->reg_names[i]);
		if (IS_ERR(base))
			return PTR_ERR(base);

		qmp->base[i] = base;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static int qmp_pcie_read_link_mode(struct device *dev, unsigned int *link_mode)
{
	struct regmap *map;
	unsigned int args[1];

	map = syscon_regmap_lookup_by_phandle_args(dev->of_node, "qcom,link-mode",
						   ARRAY_SIZE(args), args);
	if (IS_ERR(map))
		return PTR_ERR(map);

	return regmap_read(map, args[0], link_mode);
}
/* …（同文件无关代码省略）… */
static int qmp_pcie_probe_phy(struct qmp_pcie *qmp, struct device_node *np,
			      struct phy **out_phy)
{
	int ret;

	ret = qmp_pcie_get_mmio(qmp);
	if (ret)
		return ret;

	ret = qmp_pcie_clk_init(qmp);
	if (ret)
		return ret;

	ret = qmp_pcie_reset_init(qmp);
	if (ret)
		return ret;

	ret = qmp_pcie_vreg_init(qmp);
	if (ret)
		return ret;

	ret = qmp_pcie_pd_init(qmp);
	if (ret)
		return ret;

	*out_phy = devm_phy_create(qmp->dev, np, &qmp_pcie_phy_ops);
	if (IS_ERR(*out_phy))
		return PTR_ERR(*out_phy);

	phy_set_drvdata(*out_phy, qmp);

	return 0;
}
/* …（同文件无关代码省略）… */
	int ret;

	qmp_data = devm_kzalloc(dev, sizeof(*qmp_data), GFP_KERNEL);

	match_data = of_device_get_match_data(dev);
	if (!match_data)
		return -EINVAL;

	if (!qmp_data)
		return -ENOMEM;

	ret = qmp_pcie_read_link_mode(dev, &link_mode);
	if (ret)
		return dev_err_probe(dev, ret, "failed to read qcom,link-mode\n");
/* …（同文件无关代码省略）… */
		.compatible = "qcom,glymur-qmp-gen5x8-pcie-phy",
		.data = &glymur_qmp_gen5x8_match_data,
	},
	{ }
};
MODULE_DEVICE_TABLE(of, qmp_pcie_multiphy_of_match_table);
/* …（同文件无关代码省略）… */
module_platform_driver(qmp_pcie_multiphy_driver);

MODULE_AUTHOR("Qiang Yu <qiang.yu@oss.qualcomm.com>");
MODULE_DESCRIPTION("Qualcomm QMP PCIe Multi-PHY driver for Glymur");
MODULE_LICENSE("GPL");
