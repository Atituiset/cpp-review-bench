// AUTO-DRAFT from torvalds/linux PR #d857e1f3faf998f2614dcdb52a90a27bc83bd71d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	.cache_type	= REGCACHE_MAPLE,
};

static struct mfd_cell khadas_mcu_fan_cells[] = {
	/* VIM1/2 Rev13+ and VIM3 only */
	{ .name = "khadas-mcu-fan-ctrl", },
};

static struct mfd_cell khadas_mcu_cells[] = {
	{ .name = "khadas-mcu-user-mem", },
};

static int khadas_mcu_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct khadas_mcu *ddata;
	int ret;

	ddata = devm_kzalloc(dev, sizeof(*ddata), GFP_KERNEL);
	if (!ddata)
		return -ENOMEM;

	i2c_set_clientdata(client, ddata);

	ddata->dev = dev;

	ddata->regmap = devm_regmap_init_i2c(client, &khadas_mcu_regmap_config);
	if (IS_ERR(ddata->regmap)) {
		ret = PTR_ERR(ddata->regmap);
		dev_err(dev, "Failed to allocate register map: %d\n", ret);
		return ret;
	}

	ret = devm_mfd_add_devices(dev, PLATFORM_DEVID_NONE,
				   khadas_mcu_cells,
				   ARRAY_SIZE(khadas_mcu_cells),
				   NULL, 0, NULL);
	if (ret)
		return ret;

	if (of_property_present(dev->of_node, "#cooling-cells"))
		return devm_mfd_add_devices(dev, PLATFORM_DEVID_NONE,
					    khadas_mcu_fan_cells,
					    ARRAY_SIZE(khadas_mcu_fan_cells),
					    NULL, 0, NULL);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id khadas_mcu_of_match[] = {
	{ .compatible = "khadas,mcu", },
	{},
};
MODULE_DEVICE_TABLE(of, khadas_mcu_of_match);
