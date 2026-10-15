// AUTO-DRAFT from torvalds/linux PR #d77bd0d469e1923e587bb919301c0bb29093e30a
enum gab_chan_type {
	GAB_VOLTAGE = 0,
	GAB_CURRENT,
	GAB_POWER,
	GAB_TEMP,
	GAB_MAX_CHAN_TYPE
};
/* …（同文件无关代码省略）… */
struct gab {
	struct power_supply *psy;
	struct power_supply_desc psy_desc;
	struct iio_channel *channel[GAB_MAX_CHAN_TYPE];
	struct delayed_work bat_work;
	int status;
	struct gpio_desc *charge_finished;
};
/* …（同文件无关代码省略）… */
static struct gab *to_generic_bat(struct power_supply *psy)
{
	return power_supply_get_drvdata(psy);
}
/* …（同文件无关代码省略）… */
static int gab_read_channel(struct gab *adc_bat, enum gab_chan_type channel,
		int *result)
{
	int ret;
  // <<< BUG ANCHOR
	ret = iio_read_channel_processed(adc_bat->channel[channel], result);
	if (ret < 0)
		dev_err(&adc_bat->psy->dev, "read channel error: %d\n", ret);
	else
		*result *= 1000;

	return ret;
}

static int gab_get_property(struct power_supply *psy,
		enum power_supply_property psp, union power_supply_propval *val)
{
	struct gab *adc_bat = to_generic_bat(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = adc_bat->status;
		return 0;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		return gab_read_channel(adc_bat, GAB_VOLTAGE, &val->intval);
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		return gab_read_channel(adc_bat, GAB_CURRENT, &val->intval);
	case POWER_SUPPLY_PROP_POWER_NOW:
		return gab_read_channel(adc_bat, GAB_POWER, &val->intval);
	case POWER_SUPPLY_PROP_TEMP:
		return gab_read_channel(adc_bat, GAB_TEMP, &val->intval);
	default:
		return -EINVAL;
	}
}
