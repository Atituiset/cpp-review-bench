// AUTO-DRAFT from torvalds/linux PR #b152d7176339f11f05457799be609268e1a61cf2
#include <linux/scmi_protocol.h>
#include <linux/slab.h>
#include <linux/types.h>
  // <<< BUG ANCHOR
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinconf.h>
/* …（同文件无关代码省略）… */
static int pinctrl_scmi_map_pinconf_type(enum pin_config_param param,
					 enum scmi_pinctrl_conf_type *type)
{
	u32 arg = param;

	switch (arg) {
	case PIN_CONFIG_BIAS_BUS_HOLD:
		*type = SCMI_PIN_BIAS_BUS_HOLD;
		break;
	case PIN_CONFIG_BIAS_DISABLE:
		*type = SCMI_PIN_BIAS_DISABLE;
		break;
	case PIN_CONFIG_BIAS_HIGH_IMPEDANCE:
		*type = SCMI_PIN_BIAS_HIGH_IMPEDANCE;
		break;
	case PIN_CONFIG_BIAS_PULL_DOWN:
		*type = SCMI_PIN_BIAS_PULL_DOWN;
		break;
	case PIN_CONFIG_BIAS_PULL_PIN_DEFAULT:
		*type = SCMI_PIN_BIAS_PULL_DEFAULT;
		break;
	case PIN_CONFIG_BIAS_PULL_UP:
		*type = SCMI_PIN_BIAS_PULL_UP;
		break;
	case PIN_CONFIG_DRIVE_OPEN_DRAIN:
		*type = SCMI_PIN_DRIVE_OPEN_DRAIN;
		break;
	case PIN_CONFIG_DRIVE_OPEN_SOURCE:
		*type = SCMI_PIN_DRIVE_OPEN_SOURCE;
		break;
	case PIN_CONFIG_DRIVE_PUSH_PULL:
		*type = SCMI_PIN_DRIVE_PUSH_PULL;
		break;
	case PIN_CONFIG_DRIVE_STRENGTH:
		*type = SCMI_PIN_DRIVE_STRENGTH;
		break;
	case PIN_CONFIG_DRIVE_STRENGTH_UA:
		*type = SCMI_PIN_DRIVE_STRENGTH;
		break;
	case PIN_CONFIG_INPUT_DEBOUNCE:
		*type = SCMI_PIN_INPUT_DEBOUNCE;
		break;
	case PIN_CONFIG_INPUT_ENABLE:
		*type = SCMI_PIN_INPUT_MODE;
		break;
	case PIN_CONFIG_INPUT_SCHMITT:
		*type = SCMI_PIN_INPUT_SCHMITT;
		break;
	case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
		*type = SCMI_PIN_INPUT_MODE;
		break;
	case PIN_CONFIG_MODE_LOW_POWER:
		*type = SCMI_PIN_LOW_POWER_MODE;
		break;
	case PIN_CONFIG_OUTPUT_ENABLE:
		*type = SCMI_PIN_OUTPUT_MODE;
		break;
	case PIN_CONFIG_POWER_SOURCE:
		*type = SCMI_PIN_POWER_SOURCE;
		break;
	case PIN_CONFIG_SLEW_RATE:
		*type = SCMI_PIN_SLEW_RATE;
		break;
	case SCMI_PIN_OEM_START ... SCMI_PIN_OEM_END:
		*type = arg;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int pinctrl_scmi_map_pinconf_type_get(enum pin_config_param param,
					     enum scmi_pinctrl_conf_type *type)
{
/* …（同文件无关代码省略）… */
	return pinctrl_scmi_map_pinconf_type(param, type);
}

static int pinctrl_scmi_map_pinconf_type_set(enum pin_config_param param,
					     enum scmi_pinctrl_conf_type *type)
{
	if (param == PIN_CONFIG_LEVEL) {
		*type = SCMI_PIN_OUTPUT_VALUE;
		return 0;
	}

	return pinctrl_scmi_map_pinconf_type(param, type);
}

/* …（同文件无关代码省略）… */
	if (ret)
		return ret;

	*config = pinconf_to_config_packed(config_type, config_value);

	return 0;
/* …（同文件无关代码省略）… */
		param = pinconf_to_config_param(configs[i]);
		if (param == PIN_CONFIG_PERSIST_STATE)
			continue;
		ret = pinctrl_scmi_map_pinconf_type_set(param, &p_config_type[cnt]);
		if (ret) {
			dev_err(pmx->dev, "Error map pinconf_type %d\n", ret);
			goto free_config;
		}
		p_config_value[cnt] = pinconf_to_config_argument(configs[i]);
		cnt++;
	}

/* …（同文件无关代码省略）… */

	for (i = 0; i < num_configs; i++) {
		param = pinconf_to_config_param(configs[i]);
		ret = pinctrl_scmi_map_pinconf_type_set(param, &p_config_type[i]);
		if (ret) {
			dev_err(pmx->dev, "Error map pinconf_type %d\n", ret);
			goto free_config;
		}

		p_config_value[i] = pinconf_to_config_argument(configs[i]);
	}

	ret = pinctrl_ops->settings_conf(pmx->ph, group, GROUP_TYPE,
/* …（同文件无关代码省略）… */
	if (ret)
		return ret;

	*config = pinconf_to_config_packed(config_type, config_value);

	return 0;
