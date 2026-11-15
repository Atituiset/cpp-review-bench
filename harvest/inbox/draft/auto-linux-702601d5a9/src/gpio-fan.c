// AUTO-DRAFT from torvalds/linux PR #54975c6523dc77862c25deeb5fa395990b8d3fc4
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static void fan_alarm_notify(struct work_struct *ws)
{
	struct gpio_fan_data *fan_data =
		container_of(ws, struct gpio_fan_data, alarm_work);

	sysfs_notify(&fan_data->hwmon_dev->kobj, NULL, "fan1_alarm");
	kobject_uevent(&fan_data->hwmon_dev->kobj, KOBJ_CHANGE);
}
/* …（同文件无关代码省略）… */
static irqreturn_t fan_alarm_irq_handler(int irq, void *dev_id)
{
	struct gpio_fan_data *fan_data = dev_id;

	schedule_work(&fan_data->alarm_work);

	return IRQ_HANDLED;
}
/* …（同文件无关代码省略）… */
static int fan_alarm_init(struct gpio_fan_data *fan_data)
{
	int alarm_irq;
	int err;
	struct device *dev = fan_data->dev;

	/*
	 * If the alarm GPIO don't support interrupts, just leave
	 * without initializing the fail notification support.
	 */
	alarm_irq = gpiod_to_irq(fan_data->alarm_gpio);
	if (alarm_irq <= 0)
		return 0;

	err = devm_work_autocancel(dev, &fan_data->alarm_work,
				   fan_alarm_notify);
	if (err)
		return err;

	irq_set_irq_type(alarm_irq, IRQ_TYPE_EDGE_BOTH);
	return devm_request_irq(dev, alarm_irq, fan_alarm_irq_handler,
				0, "GPIO fan alarm", fan_data);
}
/* …（同文件无关代码省略）… */
static void __set_fan_ctrl(struct gpio_fan_data *fan_data, int ctrl_val)
{
	int i;

	for (i = 0; i < fan_data->num_gpios; i++)
		gpiod_set_value_cansleep(fan_data->gpios[i],
					 (ctrl_val >> i) & 1);
}
/* …（同文件无关代码省略）… */
static int __get_fan_ctrl(struct gpio_fan_data *fan_data)
{
	int i;
	int ctrl_val = 0;

	for (i = 0; i < fan_data->num_gpios; i++) {
		int value;

		value = gpiod_get_value_cansleep(fan_data->gpios[i]);
		ctrl_val |= (value << i);
	}
	return ctrl_val;
}
/* …（同文件无关代码省略）… */
static int set_fan_speed(struct gpio_fan_data *fan_data, int speed_index)
{
	if (fan_data->speed_index == speed_index)
		return 0;

	if (fan_data->speed_index == 0 && speed_index > 0) {
		int ret;

		ret = pm_runtime_resume_and_get(fan_data->dev);
		if (ret < 0)
			return ret;
	}

	__set_fan_ctrl(fan_data, fan_data->speed[speed_index].ctrl_val);

	/* The GPIOs have changed, and a PM put drops the reference on error. */
	fan_data->speed_index = speed_index;

	if (speed_index == 0) {
		int ret;

		ret = pm_runtime_put_sync(fan_data->dev);
		if (ret < 0 && ret != -ENOSYS)
			return ret;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static int get_fan_speed_index(struct gpio_fan_data *fan_data)
{
	int ctrl_val = __get_fan_ctrl(fan_data);
	int i;

	for (i = 0; i < fan_data->num_speed; i++)
		if (fan_data->speed[i].ctrl_val == ctrl_val)
			return i;

	dev_warn(fan_data->dev,
		 "missing speed array entry for GPIO value 0x%x\n", ctrl_val);

	return -ENODEV;
}
/* …（同文件无关代码省略）… */
static int fan_ctrl_init(struct gpio_fan_data *fan_data)
{
	int num_gpios = fan_data->num_gpios;
	struct gpio_desc **gpios = fan_data->gpios;
	int i, err;

	for (i = 0; i < num_gpios; i++) {
		/*
		 * The GPIO descriptors were retrieved with GPIOD_ASIS so here
		 * we set the GPIO into output mode, carefully preserving the
		 * current value by setting it to whatever it is already set
		 * (no surprise changes in default fan speed).
		 */
		err = gpiod_direction_output(gpios[i],
					gpiod_get_value_cansleep(gpios[i]));
		if (err)
			return err;
	}

	fan_data->pwm_enable = true; /* Enable manual fan speed control. */
	fan_data->speed_index = get_fan_speed_index(fan_data);
	if (fan_data->speed_index < 0)
		return fan_data->speed_index;

	return 0;
}
/* …（同文件无关代码省略）… */
static int gpio_fan_get_of_data(struct gpio_fan_data *fan_data)
{
	struct gpio_fan_speed *speed;
	struct device *dev = fan_data->dev;
	struct device_node *np = dev->of_node;
	struct gpio_desc **gpios;
	unsigned i;
	u32 u;
	struct property *prop;
	const __be32 *p;

	/* Alarm GPIO if one exists */
	fan_data->alarm_gpio = devm_gpiod_get_optional(dev, "alarm", GPIOD_IN);
	if (IS_ERR(fan_data->alarm_gpio))
		return PTR_ERR(fan_data->alarm_gpio);

	/* Fill GPIO pin array */
	fan_data->num_gpios = gpiod_count(dev, NULL);
	if (fan_data->num_gpios <= 0) {
		if (fan_data->alarm_gpio)
			return 0;
		dev_err(dev, "DT properties empty / missing");
		return -ENODEV;
	}
	gpios = devm_kzalloc_objs(dev, struct gpio_desc *, fan_data->num_gpios);
	if (!gpios)
		return -ENOMEM;
	for (i = 0; i < fan_data->num_gpios; i++) {
		gpios[i] = devm_gpiod_get_index(dev, NULL, i, GPIOD_ASIS);
		if (IS_ERR(gpios[i]))
			return PTR_ERR(gpios[i]);
	}
	fan_data->gpios = gpios;

	/* Get number of RPM/ctrl_val pairs in speed map */
	prop = of_find_property(np, "gpio-fan,speed-map", &i);
	if (!prop) {
		dev_err(dev, "gpio-fan,speed-map DT property missing");
		return -ENODEV;
	}
	i = i / sizeof(u32);
	if (i == 0 || i & 1) {
		dev_err(dev, "gpio-fan,speed-map contains zero/odd number of entries");
		return -ENODEV;
	}
	fan_data->num_speed = i / 2;

	/*
	 * Populate speed map
	 * Speed map is in the form <RPM ctrl_val RPM ctrl_val ...>
	 * this needs splitting into pairs to create gpio_fan_speed structs
	 */
	speed = devm_kzalloc_objs(dev, struct gpio_fan_speed,
				  fan_data->num_speed);
	if (!speed)
		return -ENOMEM;
	p = NULL;
	for (i = 0; i < fan_data->num_speed; i++) {
		p = of_prop_next_u32(prop, p, &u);
		if (!p)
			return -ENODEV;
		speed[i].rpm = u;
		p = of_prop_next_u32(prop, p, &u);
		if (!p)
			return -ENODEV;
		speed[i].ctrl_val = u;
	}
	fan_data->speed = speed;

	return 0;
}
/* …（同文件无关代码省略）… */
static void gpio_fan_stop(void *data)
{
	struct gpio_fan_data *fan_data = data;

	mutex_lock(&fan_data->lock);
	set_fan_speed(data, 0);
	mutex_unlock(&fan_data->lock);

	pm_runtime_disable(fan_data->dev);
}

static int gpio_fan_probe(struct platform_device *pdev)
{
	int err;
	struct gpio_fan_data *fan_data;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;

	fan_data = devm_kzalloc_obj(dev, struct gpio_fan_data);
	if (!fan_data)
		return -ENOMEM;

	fan_data->dev = dev;
	err = gpio_fan_get_of_data(fan_data);
	if (err)
		return err;

	platform_set_drvdata(pdev, fan_data);
	mutex_init(&fan_data->lock);

	fan_data->supply = devm_regulator_get(dev, "fan");
	if (IS_ERR(fan_data->supply))
		return dev_err_probe(dev, PTR_ERR(fan_data->supply),
				     "Failed to get fan-supply");

	/* Configure control GPIOs if available. */
	if (fan_data->gpios && fan_data->num_gpios > 0) {
		if (!fan_data->speed || fan_data->num_speed <= 1)
			return -EINVAL;
		err = fan_ctrl_init(fan_data);
		if (err)
			return err;
		err = devm_add_action_or_reset(dev, gpio_fan_stop, fan_data);
		if (err)
			return err;
	}

	/* Make this driver part of hwmon class. */
	fan_data->hwmon_dev =
		devm_hwmon_device_register_with_groups(dev,
						       "gpio_fan", fan_data,
						       gpio_fan_groups);
	if (IS_ERR(fan_data->hwmon_dev))
		return PTR_ERR(fan_data->hwmon_dev);

	/* Configure alarm GPIO if available. */
	if (fan_data->alarm_gpio) {
		err = fan_alarm_init(fan_data);
		if (err)
			return err;
	}

	pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
	/* If current GPIO state is active, mark RPM as active as well */
	if (fan_data->speed_index > 0) {
		int ret;

		ret = pm_runtime_resume_and_get(&pdev->dev);
		if (ret)
			return ret;
	}

	/* Optional cooling device register for Device tree platforms */
	fan_data->cdev = devm_thermal_of_child_cooling_device_register(dev, np,
								       "gpio-fan",
								       fan_data,
								       &gpio_fan_cool_ops);

	dev_info(dev, "GPIO fan initialized\n");

	return 0;
}
