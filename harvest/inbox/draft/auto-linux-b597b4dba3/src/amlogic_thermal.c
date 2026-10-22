// AUTO-DRAFT from torvalds/linux PR #4403278e4ea2f654f8130a5c03eb8b4516a0f201
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
#define TSENSOR_CFG_REG1			0x4
/* …（同文件无关代码省略）… */
	#define TSENSOR_CFG_REG1_VCM_EN		BIT(10)
	#define TSENSOR_CFG_REG1_VBG_EN		BIT(9)
/* …（同文件无关代码省略）… */
	#define TSENSOR_CFG_REG1_FILTER_EN	BIT(5)
	#define TSENSOR_CFG_REG1_DEM_EN		BIT(3)
	#define TSENSOR_CFG_REG1_CH_SEL		GENMASK(1, 0)
	#define TSENSOR_CFG_REG1_ENABLE		\
		(TSENSOR_CFG_REG1_FILTER_EN |	\
		 TSENSOR_CFG_REG1_VCM_EN |	\
		 TSENSOR_CFG_REG1_VBG_EN |	\
		 TSENSOR_CFG_REG1_DEM_EN |	\
		 TSENSOR_CFG_REG1_CH_SEL)
/* …（同文件无关代码省略）… */
#define TSENSOR_STAT0			0x40
/* …（同文件无关代码省略）… */
#define TSENSOR_READ_TEMP_MASK		GENMASK(15, 0)
/* …（同文件无关代码省略）… */
#define TSENSOR_TRIM_SIGN_MASK		BIT(15)
#define TSENSOR_TRIM_TEMP_MASK		GENMASK(14, 0)
/* …（同文件无关代码省略）… */
struct amlogic_thermal_soc_calib_data {
	int A;
	int B;
	int m;
	int n;
};
/* …（同文件无关代码省略）… */
struct amlogic_thermal_data {
	int u_efuse_off;
	const struct amlogic_thermal_soc_calib_data *calibration_parameters;
	const struct regmap_config *regmap_config;
	bool use_sm;
};
/* …（同文件无关代码省略）… */
struct amlogic_thermal {
	struct platform_device *pdev;
	const struct amlogic_thermal_data *data;
	struct regmap *regmap;
	struct regmap *sec_ao_map;
	struct clk *clk;
	struct thermal_zone_device *tzd;
	u32 trim_info;
	struct meson_sm_firmware *sm_fw;
	u32 tsensor_id;
};
/* …（同文件无关代码省略）… */
static int amlogic_thermal_code_to_millicelsius(struct amlogic_thermal *pdata,
						int temp_code)
{
	const struct amlogic_thermal_soc_calib_data *param =
					pdata->data->calibration_parameters;
	int temp;
	s64 factor, uptat, uefuse;

	uefuse = pdata->trim_info & TSENSOR_TRIM_SIGN_MASK ?
			     ~(pdata->trim_info & TSENSOR_TRIM_TEMP_MASK) + 1 :
			     (pdata->trim_info & TSENSOR_TRIM_TEMP_MASK);

	factor = param->n * temp_code;
	factor = div_s64(factor, 100);

	uptat = temp_code * param->m;
	uptat = div_s64(uptat, 100);
	uptat = uptat * BIT(16);
	uptat = div_s64(uptat, BIT(16) + factor);

	temp = (uptat + uefuse) * param->A;
	temp = div_s64(temp, BIT(16));
	temp = (temp - param->B) * 100;

	return temp;
}
/* …（同文件无关代码省略）… */
{
	int ret;

	ret = clk_prepare_enable(data->clk);
	if (ret)
		return ret;

/* …（同文件无关代码省略）… */
{
	regmap_update_bits(data->regmap, TSENSOR_CFG_REG1,
			   TSENSOR_CFG_REG1_ENABLE, 0);
	clk_disable_unprepare(data->clk);
}

static int amlogic_thermal_get_temp(struct thermal_zone_device *tz, int *temp)
{
	unsigned int tval;
	struct amlogic_thermal *pdata = thermal_zone_device_priv(tz);

	if (!pdata)
		return -EINVAL;

	regmap_read(pdata->regmap, TSENSOR_STAT0, &tval);
	*temp =
	   amlogic_thermal_code_to_millicelsius(pdata,
						tval & TSENSOR_READ_TEMP_MASK);

	return 0;
}
/* …（同文件无关代码省略）… */
static int amlogic_thermal_probe_sm(struct platform_device *pdev,
				    struct amlogic_thermal *pdata)
{
	struct device *dev = &pdev->dev;
	struct of_phandle_args ph_args;
	int ret;

	ret = of_parse_phandle_with_fixed_args(pdev->dev.of_node,
					       "amlogic,secure-monitor",
					       1, 0, &ph_args);
	if (ret)
		return ret;

	if (!ph_args.np) {
		dev_err(dev, "Failed to parse secure monitor phandle\n");
		return -ENODEV;
	}

	pdata->sm_fw = meson_sm_get(ph_args.np);
	of_node_put(ph_args.np);
	if (!pdata->sm_fw) {
		dev_err(dev, "Failed to get secure monitor firmware\n");
		return -EPROBE_DEFER;
	}

	pdata->tsensor_id = ph_args.args[0];

	return meson_sm_get_thermal_calib(pdata->sm_fw,
					  &pdata->trim_info,
					  pdata->tsensor_id);
}
/* …（同文件无关代码省略）… */
	if (IS_ERR(pdata->regmap))
		return PTR_ERR(pdata->regmap);

	pdata->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(pdata->clk))
		return dev_err_probe(dev, PTR_ERR(pdata->clk), "failed to get clock\n");

	if (pdata->data->use_sm)
		ret = amlogic_thermal_probe_sm(pdev, pdata);
