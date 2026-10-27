// AUTO-DRAFT from torvalds/linux PR #5b93dc1c0c61fd0fe575cb473e9e25f929fda509
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
struct ktd2692_led_config_data {
	/* maximum LED current in movie mode */
	u32 movie_max_microamp;
	/* maximum LED current in flash mode */
	u32 flash_max_microamp;
	/* maximum flash timeout */
	u32 flash_max_timeout;
	/* max LED brightness level */
	enum led_brightness max_brightness;
};
/* …（同文件无关代码省略）… */
};

struct ktd2692_context {
	/* Common ExpressWire properties (ctrl GPIO and timing) */
	struct expresswire_common_props props;

/* …（同文件无关代码省略）… */

static void regulator_disable_action(void *_data)
{
	struct device *dev = _data;
	struct ktd2692_context *led = dev_get_drvdata(dev);
	int ret;

	ret = regulator_disable(led->regulator);
	if (ret)
		dev_err(dev, "Failed to disable supply: %d\n", ret);
}

static int ktd2692_parse_dt(struct ktd2692_context *led, struct device *dev,
			    struct ktd2692_led_config_data *cfg)
{
	struct device_node *np = dev_of_node(dev);
	int ret;

	if (!np)
		return -ENXIO;

	led->props.ctrl_gpio = devm_gpiod_get(dev, "ctrl", GPIOD_ASIS);
	ret = PTR_ERR_OR_ZERO(led->props.ctrl_gpio);
	if (ret)
		return dev_err_probe(dev, ret, "cannot get ctrl-gpios\n");

	led->aux_gpio = devm_gpiod_get_optional(dev, "aux", GPIOD_ASIS);
	if (IS_ERR(led->aux_gpio))
		return dev_err_probe(dev, PTR_ERR(led->aux_gpio), "cannot get aux-gpios\n");

	led->regulator = devm_regulator_get(dev, "vin");
	if (IS_ERR(led->regulator))
		led->regulator = NULL;

	if (led->regulator) {
		ret = regulator_enable(led->regulator);
		if (ret) {
			dev_err(dev, "Failed to enable supply: %d\n", ret);
		} else {
			ret = devm_add_action_or_reset(dev,
						regulator_disable_action, dev);
			if (ret)
				return ret;
		}
	}

	struct device_node *child_node __free(device_node) =
		of_get_next_available_child(np, NULL);
	if (!child_node) {
		dev_err(dev, "No DT child node found for connected LED.\n");
		return -EINVAL;
	}

	led->fled_cdev.led_cdev.name =
		of_get_property(child_node, "label", NULL) ? : child_node->name;

	ret = of_property_read_u32(child_node, "led-max-microamp",
				   &cfg->movie_max_microamp);
	if (ret) {
		dev_err(dev, "failed to parse led-max-microamp\n");
		return ret;
	}

	ret = of_property_read_u32(child_node, "flash-max-microamp",
				   &cfg->flash_max_microamp);
	if (ret) {
		dev_err(dev, "failed to parse flash-max-microamp\n");
		return ret;
	}

	ret = of_property_read_u32(child_node, "flash-max-timeout-us",
				   &cfg->flash_max_timeout);
	if (ret) {
		dev_err(dev, "failed to parse flash-max-timeout-us\n");
		return ret;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
	if (!led)
		return -ENOMEM;

	fled_cdev = &led->fled_cdev;
	led_cdev = &fled_cdev->led_cdev;
	led->props.timing = ktd2692_timing;
