// AUTO-DRAFT from torvalds/linux PR #d77bd0d469e1923e587bb919301c0bb29093e30a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
  // <<< BUG ANCHOR
#define DS2780_CURRENT_UNITS	1563
/* …（同文件无关代码省略）… */
struct ds2780_device_info {
	struct device *dev;
	struct power_supply *bat;
	struct power_supply_desc bat_desc;
	struct device *w1_dev;
};
/* …（同文件无关代码省略）… */
enum current_types {
	CURRENT_NOW,
	CURRENT_AVG,
};
/* …（同文件无关代码省略）… */
static inline int ds2780_battery_io(struct ds2780_device_info *dev_info,
	char *buf, int addr, size_t count, int io)
{
	return w1_ds2780_io(dev_info->w1_dev, buf, addr, count, io);
}
/* …（同文件无关代码省略）… */
static inline int ds2780_read8(struct ds2780_device_info *dev_info, u8 *val,
	int addr)
{
	return ds2780_battery_io(dev_info, val, addr, sizeof(u8), 0);
}
/* …（同文件无关代码省略）… */
static int ds2780_read16(struct ds2780_device_info *dev_info, s16 *val,
	int addr)
{
	int ret;
	u8 raw[2];

	ret = ds2780_battery_io(dev_info, raw, addr, sizeof(raw), 0);
	if (ret < 0)
		return ret;

	*val = (raw[0] << 8) | raw[1];

	return 0;
}
/* …（同文件无关代码省略）… */
static int ds2780_get_current(struct ds2780_device_info *dev_info,
	enum current_types type, int *current_uA)
{
	int ret, sense_res;
	s16 current_raw;
	u8 sense_res_raw, reg_msb;

	/*
	 * The units of measurement for current are dependent on the value of
	 * the sense resistor.
	 */
	ret = ds2780_read8(dev_info, &sense_res_raw, DS2780_RSNSP_REG);
	if (ret < 0)
		return ret;

	if (sense_res_raw == 0) {
		dev_err(dev_info->dev, "sense resistor value is 0\n");
		return -EINVAL;
	}
	sense_res = 1000 / sense_res_raw;

	if (type == CURRENT_NOW)
		reg_msb = DS2780_CURRENT_MSB_REG;
	else if (type == CURRENT_AVG)
		reg_msb = DS2780_IAVG_MSB_REG;
	else
		return -EINVAL;

	/*
	 * The current value is located in 16 bits across the current MSB
	 * and LSB registers in two's complement form
	 * Sign bit of the current value is in bit 7 of the current MSB register
	 * Bits 14 - 8 of the current value are in bits 6 - 0 of the current
	 * MSB register
	 * Bits 7 - 0 of the current value are in bits 7 - 0 of the current
	 * LSB register
	 */
	ret = ds2780_read16(dev_info, &current_raw, reg_msb);
	if (ret < 0)
		return ret;

	*current_uA = current_raw * (DS2780_CURRENT_UNITS / sense_res);
	return 0;
}
/* …（同文件无关代码省略）… */
static int ds2780_get_capacity(struct ds2780_device_info *dev_info,
	int *capacity)
{
	int ret;
	u8 raw;

	ret = ds2780_read8(dev_info, &raw, DS2780_RARC_REG);
	if (ret < 0)
		return ret;

	*capacity = raw;
	return raw;
}

static int ds2780_get_status(struct ds2780_device_info *dev_info, int *status)
{
	int ret, current_uA, capacity;

	ret = ds2780_get_current(dev_info, CURRENT_NOW, &current_uA);
	if (ret < 0)
		return ret;

	ret = ds2780_get_capacity(dev_info, &capacity);
	if (ret < 0)
		return ret;

	if (capacity == 100)
		*status = POWER_SUPPLY_STATUS_FULL;
	else if (current_uA == 0)
		*status = POWER_SUPPLY_STATUS_NOT_CHARGING;
	else if (current_uA < 0)
		*status = POWER_SUPPLY_STATUS_DISCHARGING;
	else
		*status = POWER_SUPPLY_STATUS_CHARGING;

	return 0;
}
