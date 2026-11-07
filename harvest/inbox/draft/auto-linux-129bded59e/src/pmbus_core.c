// AUTO-DRAFT from torvalds/linux PR #3e516eae690b3f24c19168cc0fe605e1c610eb21
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#define PMBUS_ATTR_ALLOC_SIZE	32
#define PMBUS_NAME_SIZE		24
/* …（同文件无关代码省略）… */
struct pmbus_sensor {
	struct pmbus_sensor *next;
	char name[PMBUS_NAME_SIZE];	/* sysfs sensor name */
	struct sensor_device_attribute attribute;
	u8 page;		/* page number */
	u8 phase;		/* phase number, 0xff for all phases */
	u16 reg;		/* register */
	enum pmbus_sensor_classes class;	/* sensor class */
	bool update;		/* runtime sensor update needed */
	bool convert;		/* Whether or not to apply linear/vid/direct */
	int data;		/* Sensor data; negative if there was a read error */
};
#define to_pmbus_sensor(_attr) \
	container_of(_attr, struct pmbus_sensor, attribute)
/* …（同文件无关代码省略）… */
struct pmbus_boolean {
	char name[PMBUS_NAME_SIZE];	/* sysfs boolean name */
	struct sensor_device_attribute attribute;
	struct pmbus_sensor *s1;
	struct pmbus_sensor *s2;
};
#define to_pmbus_boolean(_attr) \
	container_of(_attr, struct pmbus_boolean, attribute)
/* …（同文件无关代码省略）… */
#define PB_STATUS_MASK	0xffff
#define PB_REG_SHIFT	16
#define PB_REG_MASK	0x3ff
#define PB_PAGE_SHIFT	26
#define PB_PAGE_MASK	0x3f
/* …（同文件无关代码省略）… */
#define pb_reg_to_index(page, reg, mask)	(((page) << PB_PAGE_SHIFT) | \
						 ((reg) << PB_REG_SHIFT) | (mask))
/* …（同文件无关代码省略）… */
#define pb_index_to_page(index)			(((index) >> PB_PAGE_SHIFT) & PB_PAGE_MASK)
#define pb_index_to_reg(index)			(((index) >> PB_REG_SHIFT) & PB_REG_MASK)
#define pb_index_to_mask(index)			((index) & PB_STATUS_MASK)
/* …（同文件无关代码省略）… */
struct pmbus_data {
	struct device *dev;
	struct device *hwmon_dev;
	struct regulator_dev **rdevs;

	u32 flags;		/* from platform data */

	bool have_pmbus_revision;
	u8 revision;		/* The PMBus revision the device is compliant with */

	int exponent[PMBUS_PAGES];
				/* linear mode: exponent for output voltages */

	const struct pmbus_driver_info *info;

	int max_attributes;
	int num_attributes;
	struct attribute_group group;
	const struct attribute_group **groups;

	struct pmbus_sensor *sensors;

	struct mutex update_lock;

#if IS_ENABLED(CONFIG_REGULATOR)
	atomic_t regulator_events[PMBUS_PAGES];
	struct work_struct regulator_notify_work;
#endif

	bool has_status_word;		/* device uses STATUS_WORD register */
	int (*read_status)(struct i2c_client *client, int page);

	s16 currpage;	/* current page, -1 for unknown/unset */
	s16 currphase;	/* current phase, 0xff for all, -1 for unknown/unset */

	int vout_low[PMBUS_PAGES];	/* voltage low margin */
	int vout_high[PMBUS_PAGES];	/* voltage high margin */

	ktime_t next_access_backoff;	/* Wait until at least this time */
};
/* …（同文件无关代码省略）… */
void pmbus_wait(struct i2c_client *client)
{
	struct pmbus_data *data = i2c_get_clientdata(client);
	s64 delay;

	if (!data)
		return;

	delay = ktime_us_delta(data->next_access_backoff, ktime_get());

	if (delay > 0)
		fsleep(delay);
}
/* …（同文件无关代码省略）… */
void pmbus_update_ts(struct i2c_client *client, int op)
{
	struct pmbus_data *data = i2c_get_clientdata(client);
	const struct pmbus_driver_info *info;
	int delay;

	if (!data)
		return;

	info = data->info;
	delay = info->access_delay;

	if (op & PMBUS_OP_WRITE)
		delay = max(delay, info->write_delay);
	if (op & PMBUS_OP_PAGE_CHANGE)
		delay = max(delay, info->page_change_delay);

	if (delay > 0)
		data->next_access_backoff = ktime_add_us(ktime_get(), delay);
}
/* …（同文件无关代码省略）… */
int pmbus_set_page(struct i2c_client *client, int page, int phase)
{
	struct pmbus_data *data = i2c_get_clientdata(client);
	int rv;

	if (page < 0)
		return 0;

	if (!(data->info->func[page] & PMBUS_PAGE_VIRTUAL) &&
	    data->info->pages > 1 && page != data->currpage) {
		pmbus_wait(client);
		rv = i2c_smbus_write_byte_data(client, PMBUS_PAGE, page);
		pmbus_update_ts(client, PMBUS_OP_WRITE | PMBUS_OP_PAGE_CHANGE);
		if (rv < 0)
			return rv;

		pmbus_wait(client);
		rv = i2c_smbus_read_byte_data(client, PMBUS_PAGE);
		pmbus_update_ts(client, 0);
		if (rv < 0)
			return rv;

		if (rv != page)
			return -EIO;
	}
	data->currpage = page;

	if (data->info->phases[page] && data->currphase != phase &&
	    !(data->info->func[page] & PMBUS_PHASE_VIRTUAL)) {
		pmbus_wait(client);
		rv = i2c_smbus_write_byte_data(client, PMBUS_PHASE,
					       phase);
		pmbus_update_ts(client, PMBUS_OP_WRITE);
		if (rv)
			return rv;
	}
	data->currphase = phase;

	return 0;
}
/* …（同文件无关代码省略）… */
int pmbus_write_byte(struct i2c_client *client, int page, u8 value)
{
	int rv;

	rv = pmbus_set_page(client, page, 0xff);
	if (rv < 0)
		return rv;

	pmbus_wait(client);
	rv = i2c_smbus_write_byte(client, value);
	pmbus_update_ts(client, PMBUS_OP_WRITE);

	return rv;
}
/* …（同文件无关代码省略）… */
static int _pmbus_write_byte(struct i2c_client *client, int page, u8 value)
{
	struct pmbus_data *data = i2c_get_clientdata(client);
	const struct pmbus_driver_info *info = data->info;
	int status;

	if (info->write_byte) {
		status = info->write_byte(client, page, value);
		if (status != -ENODATA)
			return status;
	}
	return pmbus_write_byte(client, page, value);
}
/* …（同文件无关代码省略）… */
int pmbus_write_word_data(struct i2c_client *client, int page, u8 reg,
			  u16 word)
{
	int rv;

	rv = pmbus_set_page(client, page, 0xff);
	if (rv < 0)
		return rv;

	pmbus_wait(client);
	rv = i2c_smbus_write_word_data(client, reg, word);
	pmbus_update_ts(client, PMBUS_OP_WRITE);

	return rv;
}
/* …（同文件无关代码省略）… */
static int pmbus_write_virt_reg(struct i2c_client *client, int page, int reg,
				u16 word)
{
	int bit;
	int id;
	int rv;

	switch (reg) {
	case PMBUS_VIRT_FAN_TARGET_1 ... PMBUS_VIRT_FAN_TARGET_4:
		id = reg - PMBUS_VIRT_FAN_TARGET_1;
		bit = pmbus_fan_rpm_mask[id];
		rv = pmbus_update_fan(client, page, id, bit, bit, word);
		break;
	default:
		rv = -ENXIO;
		break;
	}

	return rv;
}
/* …（同文件无关代码省略）… */
static int _pmbus_write_word_data(struct i2c_client *client, int page, int reg,
				  u16 word)
{
	struct pmbus_data *data = i2c_get_clientdata(client);
	const struct pmbus_driver_info *info = data->info;
	int status;

	if (info->write_word_data) {
		status = info->write_word_data(client, page, reg, word);
		if (status != -ENODATA)
			return status;
	}

	if (reg >= PMBUS_VIRT_BASE)
		return pmbus_write_virt_reg(client, page, reg, word);

	return pmbus_write_word_data(client, page, reg, word);
}
/* …（同文件无关代码省略）… */
static int _pmbus_write_byte_data(struct i2c_client *client, int page, int reg, u8 value)
{
	struct pmbus_data *data = i2c_get_clientdata(client);
	const struct pmbus_driver_info *info = data->info;
	int status;

	if (info->write_byte_data) {
		status = info->write_byte_data(client, page, reg, value);
		if (status != -ENODATA)
			return status;
	}
	return pmbus_write_byte_data(client, page, reg, value);
}
/* …（同文件无关代码省略）… */
static int _pmbus_read_byte_data(struct i2c_client *client, int page, int reg)
{
	struct pmbus_data *data = i2c_get_clientdata(client);
	const struct pmbus_driver_info *info = data->info;
	int status;

	if (info->read_byte_data) {
		status = info->read_byte_data(client, page, reg);
		if (status != -ENODATA)
			return status;
	}
	return pmbus_read_byte_data(client, page, reg);
}
/* …（同文件无关代码省略）… */
int pmbus_update_fan(struct i2c_client *client, int page, int id,
		     u8 config, u8 mask, u16 command)
{
	int from;
	int rv;
	u8 to;

	from = _pmbus_read_byte_data(client, page,
				     pmbus_fan_config_registers[id]);
	if (from < 0)
		return from;

	to = (from & ~mask) | (config & mask);
	if (to != from) {
		rv = _pmbus_write_byte_data(client, page,
					    pmbus_fan_config_registers[id], to);
		if (rv < 0)
			return rv;
	}

	return _pmbus_write_word_data(client, page,
				      pmbus_fan_command_registers[id], command);
}
/* …（同文件无关代码省略）… */
int pmbus_read_word_data(struct i2c_client *client, int page, int phase, u8 reg)
{
	int rv;

	rv = pmbus_set_page(client, page, phase);
	if (rv < 0)
		return rv;

	pmbus_wait(client);
	rv = i2c_smbus_read_word_data(client, reg);
	pmbus_update_ts(client, 0);

	return rv;
}
/* …（同文件无关代码省略）… */
static int pmbus_read_virt_reg(struct i2c_client *client, int page, int reg)
{
	int rv;
	int id;

	switch (reg) {
	case PMBUS_VIRT_FAN_TARGET_1 ... PMBUS_VIRT_FAN_TARGET_4:
		id = reg - PMBUS_VIRT_FAN_TARGET_1;
		rv = pmbus_get_fan_rate_device(client, page, id, rpm);
		break;
	default:
		rv = -ENXIO;
		break;
	}

	return rv;
}
/* …（同文件无关代码省略）… */
static int _pmbus_read_word_data(struct i2c_client *client, int page,
				 int phase, int reg)
{
	struct pmbus_data *data = i2c_get_clientdata(client);
	const struct pmbus_driver_info *info = data->info;
	int status;

	if (info->read_word_data) {
		status = info->read_word_data(client, page, phase, reg);
		if (status != -ENODATA)
			return status;
	}

	if (reg >= PMBUS_VIRT_BASE)
		return pmbus_read_virt_reg(client, page, reg);

	return pmbus_read_word_data(client, page, phase, reg);
}
/* …（同文件无关代码省略）… */
static int __pmbus_read_word_data(struct i2c_client *client, int page, int reg)
{
	return _pmbus_read_word_data(client, page, 0xff, reg);
}
/* …（同文件无关代码省略）… */
int pmbus_read_byte_data(struct i2c_client *client, int page, u8 reg)
{
	int rv;

	rv = pmbus_set_page(client, page, 0xff);
	if (rv < 0)
		return rv;

	pmbus_wait(client);
	rv = i2c_smbus_read_byte_data(client, reg);
	pmbus_update_ts(client, 0);

	return rv;
}
/* …（同文件无关代码省略）… */
int pmbus_write_byte_data(struct i2c_client *client, int page, u8 reg, u8 value)
{
	int rv;

	rv = pmbus_set_page(client, page, 0xff);
	if (rv < 0)
		return rv;

	pmbus_wait(client);
	rv = i2c_smbus_write_byte_data(client, reg, value);
	pmbus_update_ts(client, PMBUS_OP_WRITE);

	return rv;
}
/* …（同文件无关代码省略）… */
int pmbus_get_fan_rate_device(struct i2c_client *client, int page, int id,
			      enum pmbus_fan_mode mode)
{
	return pmbus_get_fan_rate(client, page, id, mode, false);
}
/* …（同文件无关代码省略）… */
static void pmbus_clear_fault_page(struct i2c_client *client, int page)
{
	_pmbus_write_byte(client, page, PMBUS_CLEAR_FAULTS);
}
/* …（同文件无关代码省略）… */
void pmbus_clear_faults(struct i2c_client *client)
{
	struct pmbus_data *data = i2c_get_clientdata(client);
	int i;

	for (i = 0; i < data->info->pages; i++)
		pmbus_clear_fault_page(client, i);
}
/* …（同文件无关代码省略）… */
static int pmbus_check_status_cml(struct i2c_client *client)
{
	struct pmbus_data *data = i2c_get_clientdata(client);
	int status, status2;

	status = data->read_status(client, -1);
	if (status < 0 || (status & PB_STATUS_CML)) {
		status2 = _pmbus_read_byte_data(client, -1, PMBUS_STATUS_CML);
		if (status2 < 0 || (status2 & PB_CML_FAULT_INVALID_COMMAND))
			return -EIO;
	}
	return 0;
}
/* …（同文件无关代码省略）… */
static bool pmbus_check_register(struct i2c_client *client,
				 int (*func)(struct i2c_client *client,
					     int page, int reg),
				 int page, int reg)
{
	int rv;
	struct pmbus_data *data = i2c_get_clientdata(client);

	rv = func(client, page, reg);
	if (rv >= 0 && !(data->flags & PMBUS_SKIP_STATUS_CHECK))
		rv = pmbus_check_status_cml(client);
	if (rv < 0 && (data->flags & PMBUS_READ_STATUS_AFTER_FAILED_CHECK))
		data->read_status(client, -1);
	if (reg < PMBUS_VIRT_BASE)
		pmbus_clear_fault_page(client, -1);
	return rv >= 0;
}
/* …（同文件无关代码省略）… */
bool pmbus_check_word_register(struct i2c_client *client, int page, int reg)
{
	return pmbus_check_register(client, __pmbus_read_word_data, page, reg);
}
/* …（同文件无关代码省略）… */
static int pmbus_get_status(struct i2c_client *client, int page, int reg)
{
	struct pmbus_data *data = i2c_get_clientdata(client);
	int status;

	switch (reg) {
	case PMBUS_STATUS_WORD:
		status = data->read_status(client, page);
		break;
	default:
		status = _pmbus_read_byte_data(client, page, reg);
		break;
	}
	if (status < 0)
		pmbus_clear_faults(client);
	return status;
}
/* …（同文件无关代码省略）… */
static void pmbus_update_sensor_data(struct i2c_client *client, struct pmbus_sensor *sensor)
{
	if (sensor->data < 0 || sensor->update)
		sensor->data = _pmbus_read_word_data(client, sensor->page,
						     sensor->phase, sensor->reg);
}
/* …（同文件无关代码省略）… */
static long pmbus_reg2data_ieee754(struct pmbus_data *data,
				   struct pmbus_sensor *sensor)
{
	int exponent;
	bool sign;
	long val;

	/* only support half precision for now */
	sign = sensor->data & 0x8000;
	exponent = (sensor->data >> 10) & 0x1f;
	val = sensor->data & 0x3ff;

	if (exponent == 0) {			/* subnormal */
		exponent = -(14 + 10);
	} else if (exponent ==  0x1f) {		/* NaN, convert to min/max */
		exponent = 0;
		val = 65504;
	} else {
		exponent -= (15 + 10);		/* normal */
		val |= 0x400;
	}

	/* scale result to milli-units for all sensors except fans */
	if (sensor->class != PSC_FAN)
		val = val * 1000L;

	/* scale result to micro-units for power sensors */
	if (sensor->class == PSC_POWER)
		val = val * 1000L;

	if (exponent >= 0)
		val <<= exponent;
	else
		val >>= -exponent;

	if (sign)
		val = -val;

	return val;
}
/* …（同文件无关代码省略）… */
static s64 pmbus_reg2data_linear(struct pmbus_data *data,
				 struct pmbus_sensor *sensor)
{
	s16 exponent;
	s32 mantissa;
	s64 val;

	if (sensor->class == PSC_VOLTAGE_OUT) {	/* LINEAR16 */
		exponent = data->exponent[sensor->page];
		mantissa = (u16)sensor->data;
	} else {				/* LINEAR11 */
		exponent = ((s16)sensor->data) >> 11;
		mantissa = ((s16)((sensor->data & 0x7ff) << 5)) >> 5;
	}

	val = mantissa;

	/* scale result to milli-units for all sensors except fans */
	if (sensor->class != PSC_FAN)
		val = val * 1000LL;

	/* scale result to micro-units for power sensors */
	if (sensor->class == PSC_POWER)
		val = val * 1000LL;

	if (exponent >= 0)
		val <<= exponent;
	else
		val >>= -exponent;

	return val;
}
/* …（同文件无关代码省略）… */
static s64 pmbus_reg2data_direct(struct pmbus_data *data,
				 struct pmbus_sensor *sensor)
{
	s64 b, val = (s16)sensor->data;
	s32 m, R;

	m = data->info->m[sensor->class];
	b = data->info->b[sensor->class];
	R = data->info->R[sensor->class];

	if (m == 0)
		return 0;

	/* X = 1/m * (Y * 10^-R - b) */
	R = -R;
	/* scale result to milli-units for everything but fans */
	if (!(sensor->class == PSC_FAN || sensor->class == PSC_PWM)) {
		R += 3;
		b *= 1000;
	}

	/* scale result to micro-units for power sensors */
	if (sensor->class == PSC_POWER) {
		R += 3;
		b *= 1000;
	}

	while (R > 0) {
		val *= 10;
		R--;
	}
	while (R < 0) {
		val = div_s64(val + 5LL, 10L);  /* round closest */
		R++;
	}

	val = div_s64(val - b, m);
	return val;
}
/* …（同文件无关代码省略）… */
static s64 pmbus_reg2data_vid(struct pmbus_data *data,
			      struct pmbus_sensor *sensor)
{
	long val = sensor->data;
	long rv = 0;

	switch (data->info->vrm_version[sensor->page]) {
	case vr11:
		if (val >= 0x02 && val <= 0xb2)
			rv = DIV_ROUND_CLOSEST(160000 - (val - 2) * 625, 100);
		break;
	case vr12:
		if (val >= 0x01)
			rv = 250 + (val - 1) * 5;
		break;
	case vr13:
		if (val >= 0x01)
			rv = 500 + (val - 1) * 10;
		break;
	case imvp9:
		if (val >= 0x01)
			rv = 200 + (val - 1) * 10;
		break;
	case amd625mv:
		if (val >= 0x0 && val <= 0xd8)
			rv = DIV_ROUND_CLOSEST(155000 - val * 625, 100);
		break;
	case nvidia195mv:
		if (val >= 0x01)
			rv = 195 + (val - 1) * 5;  /* VID step is 5mv */
		break;
	}
	return rv;
}
/* …（同文件无关代码省略）… */
static s64 pmbus_reg2data(struct pmbus_data *data, struct pmbus_sensor *sensor)
{
	s64 val;

	if (!sensor->convert)
		return sensor->data;

	switch (data->info->format[sensor->class]) {
	case direct:
		val = pmbus_reg2data_direct(data, sensor);
		break;
	case vid:
		val = pmbus_reg2data_vid(data, sensor);
		break;
	case ieee754:
		val = pmbus_reg2data_ieee754(data, sensor);
		break;
	case linear:
	default:
		val = pmbus_reg2data_linear(data, sensor);
		break;
	}
	return val;
}
/* …（同文件无关代码省略）… */
#define MAX_IEEE_MANTISSA	(0x7ff * 1000)
#define MIN_IEEE_MANTISSA	(0x400 * 1000)
/* …（同文件无关代码省略）… */
static u16 pmbus_data2reg_ieee754(struct pmbus_data *data,
				  struct pmbus_sensor *sensor, long val)
{
	u16 exponent = (15 + 10);
	long mantissa;
	u16 sign = 0;

	/* simple case */
	if (val == 0)
		return 0;

	if (val < 0) {
		sign = 0x8000;
		val = -val;
	}

	/* Power is in uW. Convert to mW before converting. */
	if (sensor->class == PSC_POWER)
		val = DIV_ROUND_CLOSEST(val, 1000L);

	/*
	 * For simplicity, convert fan data to milli-units
	 * before calculating the exponent.
	 */
	if (sensor->class == PSC_FAN)
		val = val * 1000;

	/* Reduce large mantissa until it fits into 10 bit */
	while (val > MAX_IEEE_MANTISSA && exponent < 30) {
		exponent++;
		val >>= 1;
	}
	/*
	 * Increase small mantissa to generate valid 'normal'
	 * number
	 */
	while (val < MIN_IEEE_MANTISSA && exponent > 1) {
		exponent--;
		val <<= 1;
	}

	/* Convert mantissa from milli-units to units */
	mantissa = DIV_ROUND_CLOSEST(val, 1000);

	/*
	 * Ensure that the resulting number is within range.
	 * Valid range is 0x400..0x7ff, where bit 10 reflects
	 * the implied high bit in normalized ieee754 numbers.
	 * Set the range to 0x400..0x7ff to reflect this.
	 * The upper bit is then removed by the mask against
	 * 0x3ff in the final assignment.
	 */
	if (mantissa > 0x7ff)
		mantissa = 0x7ff;
	else if (mantissa < 0x400)
		mantissa = 0x400;

	/* Convert to sign, 5 bit exponent, 10 bit mantissa */
	return sign | (mantissa & 0x3ff) | ((exponent << 10) & 0x7c00);
}
/* …（同文件无关代码省略）… */
#define MAX_LIN_MANTISSA	(1023 * 1000)
#define MIN_LIN_MANTISSA	(511 * 1000)
/* …（同文件无关代码省略）… */
static u16 pmbus_data2reg_linear(struct pmbus_data *data,
				 struct pmbus_sensor *sensor, s64 val)
{
	s16 exponent = 0, mantissa;
	bool negative = false;

	/* simple case */
	if (val == 0)
		return 0;

	if (sensor->class == PSC_VOLTAGE_OUT) {
		/* LINEAR16 does not support negative voltages */
		if (val < 0)
			return 0;

		/*
		 * For a static exponents, we don't have a choice
		 * but to adjust the value to it.
		 */
		if (data->exponent[sensor->page] < 0)
			val <<= -data->exponent[sensor->page];
		else
			val >>= data->exponent[sensor->page];
		val = DIV_ROUND_CLOSEST_ULL(val, 1000);
		return clamp_val(val, 0, 0xffff);
	}

	if (val < 0) {
		negative = true;
		val = -val;
	}

	/* Power is in uW. Convert to mW before converting. */
	if (sensor->class == PSC_POWER)
		val = DIV_ROUND_CLOSEST_ULL(val, 1000);

	/*
	 * For simplicity, convert fan data to milli-units
	 * before calculating the exponent.
	 */
	if (sensor->class == PSC_FAN)
		val = val * 1000LL;

	/* Reduce large mantissa until it fits into 10 bit */
	while (val >= MAX_LIN_MANTISSA && exponent < 15) {
		exponent++;
		val >>= 1;
	}
	/* Increase small mantissa to improve precision */
	while (val < MIN_LIN_MANTISSA && exponent > -15) {
		exponent--;
		val <<= 1;
	}

	/* Convert mantissa from milli-units to units */
	mantissa = clamp_val(DIV_ROUND_CLOSEST_ULL(val, 1000), 0, 0x3ff);

	/* restore sign */
	if (negative)
		mantissa = -mantissa;

	/* Convert to 5 bit exponent, 11 bit mantissa */
	return (mantissa & 0x7ff) | ((exponent << 11) & 0xf800);
}
/* …（同文件无关代码省略）… */
static u16 pmbus_data2reg_direct(struct pmbus_data *data,
				 struct pmbus_sensor *sensor, s64 val)
{
	s64 b;
	s32 m, R;

	m = data->info->m[sensor->class];
	b = data->info->b[sensor->class];
	R = data->info->R[sensor->class];

	/* Power is in uW. Adjust R and b. */
	if (sensor->class == PSC_POWER) {
		R -= 3;
		b *= 1000;
	}

	/* Calculate Y = (m * X + b) * 10^R */
	if (!(sensor->class == PSC_FAN || sensor->class == PSC_PWM)) {
		R -= 3;		/* Adjust R and b for data in milli-units */
		b *= 1000;
	}
	val = val * m + b;

	while (R > 0) {
		val *= 10;
		R--;
	}
	while (R < 0) {
		val = div_s64(val + 5LL, 10L);  /* round closest */
		R++;
	}

	return (u16)clamp_val(val, S16_MIN, S16_MAX);
}
/* …（同文件无关代码省略）… */
static u16 pmbus_data2reg_vid(struct pmbus_data *data,
			      struct pmbus_sensor *sensor, s64 val)
{
	switch (data->info->vrm_version[sensor->page]) {
	case vr12:
		val = clamp_val(val, 250, 1520);
		return 1 + DIV_ROUND_CLOSEST_ULL(val - 250, 5);
	case vr13:
		val = clamp_val(val, 500, 3040);
		return 1 + DIV_ROUND_CLOSEST_ULL(val - 500, 10);
	case imvp9:
		val = clamp_val(val, 200, 2740);
		return 1 + DIV_ROUND_CLOSEST_ULL(val - 200, 10);
	case amd625mv:
		val = clamp_val(val, 200, 1550);
		return DIV_ROUND_CLOSEST_ULL((1550LL - val) * 100LL, 625);
	case nvidia195mv:
		val = clamp_val(val, 195, 1465);
		return 1 + DIV_ROUND_CLOSEST_ULL(val - 195, 5);
	case vr11:
	default:
		val = clamp_val(val, 500, 1600);
		return 2 + DIV_ROUND_CLOSEST_ULL((1600LL - val) * 100LL, 625);
	}
}
/* …（同文件无关代码省略）… */
static u16 pmbus_data2reg(struct pmbus_data *data,
			  struct pmbus_sensor *sensor, s64 val)
{
	u16 regval;

	if (!sensor->convert)
		return val;

	switch (data->info->format[sensor->class]) {
	case direct:
		regval = pmbus_data2reg_direct(data, sensor, val);
		break;
	case vid:
		regval = pmbus_data2reg_vid(data, sensor, val);
		break;
	case ieee754:
		regval = pmbus_data2reg_ieee754(data, sensor, val);
		break;
	case linear:
	default:
		regval = pmbus_data2reg_linear(data, sensor, val);
		break;
	}
	return regval;
}
/* …（同文件无关代码省略）… */
static int pmbus_get_boolean(struct i2c_client *client, struct pmbus_boolean *b,
			     int index)
{
	struct pmbus_data *data = i2c_get_clientdata(client);
	struct pmbus_sensor *s1 = b->s1;
	struct pmbus_sensor *s2 = b->s2;
	u16 mask = pb_index_to_mask(index);
	u8 page = pb_index_to_page(index);
	u16 reg = pb_index_to_reg(index);
	int ret, status;
	u16 regval;

	guard(pmbus_lock)(client);

	status = pmbus_get_status(client, page, reg);
	if (status < 0)
		return status;

	if (s1)
		pmbus_update_sensor_data(client, s1);
	if (s2)
		pmbus_update_sensor_data(client, s2);

	regval = status & mask;
	if (regval) {
		/* Generic STATUS_WORD alarms are not individually clearable. */
		if (data->revision >= PMBUS_REV_12 &&
		    reg != PMBUS_STATUS_WORD) {
			ret = _pmbus_write_byte_data(client, page, reg, regval);
			if (ret)
				return ret;
		} else {
			pmbus_clear_fault_page(client, page);
		}
	}
	if (s1 && s2) {
		s64 v1, v2;

		if (s1->data < 0)
			return s1->data;
		if (s2->data < 0)
			return s2->data;

		v1 = pmbus_reg2data(data, s1);
		v2 = pmbus_reg2data(data, s2);
		ret = !!(regval && v1 >= v2);
	} else {
		ret = !!regval;
	}
	return ret;
}
/* …（同文件无关代码省略）… */
static ssize_t pmbus_show_boolean(struct device *dev,
				  struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct pmbus_boolean *boolean = to_pmbus_boolean(attr);
	struct i2c_client *client = to_i2c_client(dev->parent);
	int val;

	val = pmbus_get_boolean(client, boolean, attr->index);
	if (val < 0)
		return val;
	return sysfs_emit(buf, "%d\n", val);
}
/* …（同文件无关代码省略）… */
static ssize_t pmbus_show_zero(struct device *dev,
			       struct device_attribute *devattr, char *buf)
{
	return sysfs_emit(buf, "0\n");
}
/* …（同文件无关代码省略）… */
static ssize_t pmbus_show_sensor(struct device *dev,
				 struct device_attribute *devattr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev->parent);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct pmbus_sensor *sensor = to_pmbus_sensor(attr);
	struct pmbus_data *data = i2c_get_clientdata(client);
	s64 val;

	scoped_guard(pmbus_lock, client) {
		pmbus_update_sensor_data(client, sensor);
		if (sensor->data < 0)
			return sensor->data;
		val = pmbus_reg2data(data, sensor);
	}

	return sysfs_emit(buf, "%lld\n", val);
}
/* …（同文件无关代码省略）… */
static ssize_t pmbus_set_sensor(struct device *dev,
				struct device_attribute *devattr,
				const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev->parent);
	struct pmbus_data *data = i2c_get_clientdata(client);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct pmbus_sensor *sensor = to_pmbus_sensor(attr);
	s64 val;
	int ret;
	u16 regval;

	if (kstrtos64(buf, 10, &val) < 0)
		return -EINVAL;

	guard(pmbus_lock)(client);

	regval = pmbus_data2reg(data, sensor, val);
	ret = _pmbus_write_word_data(client, sensor->page, sensor->reg, regval);
	if (ret < 0)
		return ret;

	sensor->data = -ENODATA;
	return count;
}
/* …（同文件无关代码省略）… */
static int pmbus_add_attribute(struct pmbus_data *data, struct attribute *attr)
{
	if (data->num_attributes >= data->max_attributes - 1) {
		int new_max_attrs = data->max_attributes + PMBUS_ATTR_ALLOC_SIZE;
		void *new_attrs = devm_krealloc_array(data->dev, data->group.attrs,
						      new_max_attrs, sizeof(void *),
						      GFP_KERNEL);
		if (!new_attrs)
			return -ENOMEM;
		data->group.attrs = new_attrs;
		data->max_attributes = new_max_attrs;
	}

	data->group.attrs[data->num_attributes++] = attr;
	data->group.attrs[data->num_attributes] = NULL;
	return 0;
}
/* …（同文件无关代码省略）… */
static void pmbus_dev_attr_init(struct device_attribute *dev_attr,
				const char *name,
				umode_t mode,
				ssize_t (*show)(struct device *dev,
						struct device_attribute *attr,
						char *buf),
				ssize_t (*store)(struct device *dev,
						 struct device_attribute *attr,
						 const char *buf, size_t count))
{
	sysfs_attr_init(&dev_attr->attr);
	dev_attr->attr.name = name;
	dev_attr->attr.mode = mode;
	dev_attr->show = show;
	dev_attr->store = store;
}
/* …（同文件无关代码省略）… */
static void pmbus_attr_init(struct sensor_device_attribute *a,
			    const char *name,
			    umode_t mode,
			    ssize_t (*show)(struct device *dev,
					    struct device_attribute *attr,
					    char *buf),
			    ssize_t (*store)(struct device *dev,
					     struct device_attribute *attr,
					     const char *buf, size_t count),
			    int idx)
{
	pmbus_dev_attr_init(&a->dev_attr, name, mode, show, store);
	a->index = idx;
}
/* …（同文件无关代码省略）… */
static int pmbus_add_boolean(struct pmbus_data *data,
			     const char *name, const char *type, int seq,
			     struct pmbus_sensor *s1,
			     struct pmbus_sensor *s2,
			     u8 page, u16 reg, u16 mask)
{
	struct pmbus_boolean *boolean;
	struct sensor_device_attribute *a;

	if (WARN((s1 && !s2) || (!s1 && s2), "Bad s1/s2 parameters\n"))
		return -EINVAL;

	boolean = devm_kzalloc(data->dev, sizeof(*boolean), GFP_KERNEL);
	if (!boolean)
		return -ENOMEM;

	a = &boolean->attribute;

	snprintf(boolean->name, sizeof(boolean->name), "%s%d_%s",
		 name, seq, type);
	boolean->s1 = s1;
	boolean->s2 = s2;
	pmbus_attr_init(a, boolean->name, 0444, pmbus_show_boolean, NULL,
			pb_reg_to_index(page, reg, mask));

	return pmbus_add_attribute(data, &a->dev_attr.attr);
}
/* …（同文件无关代码省略）… */
struct pmbus_thermal_data {
	struct pmbus_data *pmbus_data;
	struct pmbus_sensor *sensor;
};
/* …（同文件无关代码省略）… */
static int pmbus_thermal_add_sensor(struct pmbus_data *pmbus_data,
				    struct pmbus_sensor *sensor, int index)
{
	struct device *dev = pmbus_data->dev;
	struct pmbus_thermal_data *tdata;
	struct thermal_zone_device *tzd;

	tdata = devm_kzalloc(dev, sizeof(*tdata), GFP_KERNEL);
	if (!tdata)
		return -ENOMEM;

	tdata->sensor = sensor;
	tdata->pmbus_data = pmbus_data;

	tzd = devm_thermal_of_zone_register(dev, index, tdata,
					    &pmbus_thermal_ops);
	/*
	 * If CONFIG_THERMAL_OF is disabled, this returns -ENODEV,
	 * so ignore that error but forward any other error.
	 */
	if (IS_ERR(tzd) && (PTR_ERR(tzd) != -ENODEV))
		return PTR_ERR(tzd);

	return 0;
}
/* …（同文件无关代码省略）… */
static struct pmbus_sensor *pmbus_add_sensor(struct pmbus_data *data,
					     const char *name, const char *type,
					     int seq, int page, int phase,
					     int reg,
					     enum pmbus_sensor_classes class,
					     bool update, bool readonly,
					     bool writeonly, bool convert)
{
	struct sensor_device_attribute *a;
	struct pmbus_sensor *sensor;

	sensor = devm_kzalloc(data->dev, sizeof(*sensor), GFP_KERNEL);
	if (!sensor)
		return NULL;
	a = &sensor->attribute;

	if (type)
		snprintf(sensor->name, sizeof(sensor->name), "%s%d_%s",
			 name, seq, type);
	else
		snprintf(sensor->name, sizeof(sensor->name), "%s%d",
			 name, seq);

	if (data->flags & PMBUS_WRITE_PROTECTED)
		readonly = true;

	sensor->page = page;
	sensor->phase = phase;
	sensor->reg = reg;
	sensor->class = class;
	sensor->update = update;
	sensor->convert = convert;
	sensor->data = -ENODATA;
	pmbus_attr_init(a, sensor->name, readonly ? 0444 : 0644,
			writeonly ? pmbus_show_zero : pmbus_show_sensor,
			pmbus_set_sensor, -1);

	if (pmbus_add_attribute(data, &a->dev_attr.attr))
		return NULL;

	sensor->next = data->sensors;
	data->sensors = sensor;

	/* temperature sensors with _input values are registered with thermal */
	if (class == PSC_TEMPERATURE && strcmp(type, "input") == 0)
		pmbus_thermal_add_sensor(data, sensor, seq);

	return sensor;
}
/* …（同文件无关代码省略）… */
struct pmbus_limit_attr {
	u16 reg;		/* Limit register */
	u16 sbit;		/* Alarm attribute status bit */
	bool readonly:1;	/* True if the attribute is read-only */
	bool writeonly:1;	/* True if the attribute is write-only */
	bool update:1;		/* True if register needs updates */
	bool low:1;		/* True if low limit; for limits with compare functions only */
	const char *attr;	/* Attribute name */
	const char *alarm;	/* Alarm attribute name */
};
/* …（同文件无关代码省略）… */
struct pmbus_sensor_attr {
	u16 reg;			/* sensor register */
	u16 gbit;			/* generic status bit */
	u8 nlimit;			/* # of limit registers */
	enum pmbus_sensor_classes class;/* sensor class */
	const char *label;		/* sensor label */
	bool paged:1;			/* true if paged sensor */
	bool update:1;			/* true if update needed */
	bool compare:1;			/* true if compare function needed */
	u32 func;			/* sensor mask */
	u32 sfunc;			/* sensor status mask */
	int sreg;			/* status register */
	const struct pmbus_limit_attr *limit;/* limit registers */
};
/* …（同文件无关代码省略）… */
static int pmbus_add_limit_attrs(struct i2c_client *client,
				 struct pmbus_data *data,
				 const struct pmbus_driver_info *info,
				 const char *name, int index, int page,
				 struct pmbus_sensor *base,
				 const struct pmbus_sensor_attr *attr)
{
	const struct pmbus_limit_attr *l = attr->limit;
	int nlimit = attr->nlimit;
	int have_alarm = 0;
	int i, ret;
	struct pmbus_sensor *curr;

	for (i = 0; i < nlimit; i++) {
		if (pmbus_check_word_register(client, page, l->reg)) {
			curr = pmbus_add_sensor(data, name, l->attr, index,
						page, 0xff, l->reg, attr->class,
						attr->update || l->update,
						l->readonly, l->writeonly, true);
			if (!curr)
				return -ENOMEM;
			if (l->sbit && (info->func[page] & attr->sfunc)) {
				ret = pmbus_add_boolean(data, name,
					l->alarm, index,
					attr->compare ?  l->low ? curr : base
						      : NULL,
					attr->compare ? l->low ? base : curr
						      : NULL,
					page, attr->sreg, l->sbit);
				if (ret)
					return ret;
				have_alarm = 1;
			}
		}
		l++;
	}
	return have_alarm;
}
/* …（同文件无关代码省略）… */
	}
	base = pmbus_add_sensor(data, name, "input", index, page, phase,
				attr->reg, attr->class, true, true, false, true);
	if (!base)
		return -ENOMEM;
	/* No limit and alarm attributes for phase specific sensors */
	if (attr->sfunc && phase == 0xff) {
		ret = pmbus_add_limit_attrs(client, data, info, name,
/* …（同文件无关代码省略）… */
				  0xff, PMBUS_VIRT_FAN_TARGET_1 + id, PSC_FAN,
				  false, false, false, true);

	if (!sensor)
		return -ENOMEM;

	if (!((data->info->func[page] & PMBUS_HAVE_PWM12) ||
	      (data->info->func[page] & PMBUS_HAVE_PWM34)))
/* …（同文件无关代码省略）… */
				  0xff, PMBUS_VIRT_PWM_1 + id, PSC_PWM,
				  false, false, false, true);

	if (!sensor)
		return -ENOMEM;

	sensor = pmbus_add_sensor(data, "pwm", "enable", index, page,
				  0xff, PMBUS_VIRT_PWM_ENABLE_1 + id, PSC_PWM,
				  true, false, false, false);

	if (!sensor)
		return -ENOMEM;

	return 0;
}
/* …（同文件无关代码省略）… */
		int f;

		for (f = 0; f < ARRAY_SIZE(pmbus_fan_registers); f++) {
			int regval;

			if (!(info->func[page] & pmbus_fan_flags[f]))
/* …（同文件无关代码省略）… */
			    (!(regval & (PB_FAN_1_INSTALLED >> ((f & 1) * 4)))))
				continue;

			if (pmbus_add_sensor(data, "fan", "input", index,
					     page, 0xff, pmbus_fan_registers[f],
					     PSC_FAN, true, true, false, true) == NULL)
				return -ENOMEM;

			/* Fan control */
			if (pmbus_check_word_register(client, page,
/* …（同文件无关代码省略）… */
void pmbus_lock(struct i2c_client *client)
{
	struct pmbus_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->update_lock);
}
