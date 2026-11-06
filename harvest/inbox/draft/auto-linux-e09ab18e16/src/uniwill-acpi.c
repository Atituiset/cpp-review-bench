// AUTO-DRAFT from torvalds/linux PR #2d2200551ccc3b9e444aecb59c4b53287e169266
static int uniwill_lightbar_trigger_activate(struct led_classdev *led_cdev)
{
	return led_cdev->hw_control_set(led_cdev, 1);
}
/* …（同文件无关代码省略）… */
static void uniwill_lightbar_trigger_deactivate(struct led_classdev *led_cdev)
{
	int ret;

	ret = led_cdev->hw_control_set(led_cdev, 0);
	if (ret < 0)
		dev_err(led_cdev->dev, "Failed to stop rainbow animation: %d\n", ret);
}

static struct led_hw_trigger_type uniwill_lightbar_trigger_type;

static struct led_trigger uniwill_lightbar_trigger = {
	.name = "uniwill-rainbow",
	.activate = uniwill_lightbar_trigger_activate,
	.deactivate = uniwill_lightbar_trigger_deactivate,
	.trigger_type = &uniwill_lightbar_trigger_type,
};
