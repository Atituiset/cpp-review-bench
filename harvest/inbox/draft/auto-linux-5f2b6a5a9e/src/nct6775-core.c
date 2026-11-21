// AUTO-DRAFT from torvalds/linux PR #06ef54bf3ab7ab8531e8f9c197d189f44b8bd925
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
  // <<< BUG ANCHOR
#define NCT6776_TEMP_MASK	0x007ffffe
/* …（同文件无关代码省略）… */
static unsigned int fan_from_reg13(u16 reg, unsigned int divreg)
{
	if ((reg & 0xff1f) == 0xff1f)
		return 0;

	reg = (reg & 0x1f) | ((reg & 0xff00) >> 3);

	if (reg == 0)
		return 0;

	return 1350000U / reg;
}
/* …（同文件无关代码省略）… */

		if (!data->temp_tolerance[0][i] ||
		    data->pwm_enable[i] != speed_cruise)
			data->temp_tolerance[0][i] = fanmodecfg & 0x0f;
		if (!data->target_speed_tolerance[i] ||
		    data->pwm_enable[i] == speed_cruise) {
			u8 t = fanmodecfg & 0x0f;

			if (data->REG_TOLERANCE_H) {
				err = nct6775_read_value(data, data->REG_TOLERANCE_H[i], &reg);
/* …（同文件无关代码省略）… */
ssize_t
nct6775_store_beep(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	struct nct6775_data *data = dev_get_drvdata(dev);
	int nr = data->BEEP_BITS[sattr->index];
	int regindex = nr >> 3;
/* …（同文件无关代码省略）… */
store_temp_beep(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	struct nct6775_data *data = dev_get_drvdata(dev);
	int nr, bit, regindex;
	unsigned long val;
/* …（同文件无关代码省略）… */
static int pwm_update_registers(struct nct6775_data *data, int nr)
{
	u16 reg;
	int err;

	switch (data->pwm_enable[nr]) {
	case off:
	case manual:
		break;
	case speed_cruise:
		err = nct6775_read_value(data, data->REG_FAN_MODE[nr], &reg);
		if (err)
			return err;
		reg = (reg & ~data->tolerance_mask) |
		  (data->target_speed_tolerance[nr] & data->tolerance_mask);
		err = nct6775_write_value(data, data->REG_FAN_MODE[nr], reg);
		if (err)
			return err;
		err = nct6775_write_value(data, data->REG_TARGET[nr],
					  data->target_speed[nr] & 0xff);
		if (err)
			return err;
		if (data->REG_TOLERANCE_H) {
			reg = (data->target_speed[nr] >> 8) & 0x0f;
			reg |= (data->target_speed_tolerance[nr] & 0x38) << 1;
			err = nct6775_write_value(data, data->REG_TOLERANCE_H[nr], reg);
			if (err)
				return err;
		}
		break;
	case thermal_cruise:
		err = nct6775_write_value(data, data->REG_TARGET[nr], data->target_temp[nr]);
		if (err)
			return err;
		fallthrough;
	default:
		err = nct6775_read_value(data, data->REG_FAN_MODE[nr], &reg);
		if (err)
			return err;
		reg = (reg & ~data->tolerance_mask) |
		  data->temp_tolerance[0][nr];
		err = nct6775_write_value(data, data->REG_FAN_MODE[nr], reg);
		if (err)
			return err;
		break;
	}

	return 0;
}
/* …（同文件无关代码省略）… */

	mutex_lock(&data->update_lock);
	data->temp_tolerance[index][nr] = val;
	if (index)
		err = pwm_update_registers(data, nr);
	else
		err = nct6775_write_value(data, data->REG_CRITICAL_TEMP_TOLERANCE[nr], val);
/* …（同文件无关代码省略）… */

		data->fan_from_reg = fan_from_reg13;
		data->fan_from_reg_min = fan_from_reg13;

		data->temp_label = nct6776_temp_label;
		data->temp_mask = NCT6776_TEMP_MASK;
/* …（同文件无关代码省略）… */

		data->fan_from_reg = fan_from_reg13;
		data->fan_from_reg_min = fan_from_reg13;

		data->temp_label = nct6776_temp_label;
		data->temp_mask = NCT6776_TEMP_MASK;
/* …（同文件无关代码省略）… */

		data->fan_from_reg = fan_from_reg13;
		data->fan_from_reg_min = fan_from_reg13;

		data->temp_label = nct6776_temp_label;
		data->temp_mask = NCT6776_TEMP_MASK;
