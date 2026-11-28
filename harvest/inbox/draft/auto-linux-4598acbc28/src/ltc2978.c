// AUTO-DRAFT from torvalds/linux PR #ad5b48d7188721ba3a7a351d06a78f8030148e61
enum chips {
	/* Managers */
	ltc2972, ltc2974, ltc2975, ltc2977, ltc2978, ltc2979, ltc2980,
	/* Controllers */
	lt7170, lt7171, ltc3880, ltc3882, ltc3883, ltc3884, ltc3886, ltc3887,
	ltc3889, ltc7132, ltc7841, ltc7880,
	/* Modules */
	ltm2987, ltm4664, ltm4673, ltm4675, ltm4676, ltm4677, ltm4678, ltm4680,
	ltm4686, ltm4700,
};
/* …（同文件无关代码省略）… */
#define LTC2978_MFR_VOUT_PEAK		0xdd
#define LTC2978_MFR_VIN_PEAK		0xde
#define LTC2978_MFR_TEMPERATURE_PEAK	0xdf
/* …（同文件无关代码省略）… */
#define LTC2978_MFR_COMMON		0xef
/* …（同文件无关代码省略）… */
#define LTC3880_MFR_IOUT_PEAK		0xd7
#define LTC3880_MFR_CLEAR_PEAKS		0xe3
#define LTC3880_MFR_TEMPERATURE2_PEAK	0xf4
/* …（同文件无关代码省略）… */
#define LTC3883_MFR_IIN_PEAK		0xe1
/* …（同文件无关代码省略）… */
#define LTC2974_NUM_PAGES		4
#define LTC2978_NUM_PAGES		8
/* …（同文件无关代码省略）… */
#define LTC3883_NUM_PAGES		1
/* …（同文件无关代码省略）… */
#define LTC_POLL_TIMEOUT		100	/* in milli-seconds */
/* …（同文件无关代码省略）… */
#define LTC_NOT_BUSY			BIT(6)
#define LTC_NOT_PENDING			BIT(5)
/* …（同文件无关代码省略）… */
struct ltc2978_data {
	enum chips id;
	u16 vin_min, vin_max;
	u16 temp_min[LTC2974_NUM_PAGES], temp_max[LTC2974_NUM_PAGES];
	u16 vout_min[LTC2978_NUM_PAGES], vout_max[LTC2978_NUM_PAGES];
	u16 iout_min[LTC2974_NUM_PAGES], iout_max[LTC2974_NUM_PAGES];
	u16 iin_min, iin_max;
	u16 pin_min, pin_max;
	u16 temp2_max;
	struct pmbus_driver_info info;
	u32 features;
};
#define to_ltc2978_data(x)  container_of(x, struct ltc2978_data, info)
  // <<< BUG ANCHOR
#define FEAT_CLEAR_PEAKS	BIT(0)
#define FEAT_NEEDS_POLLING	BIT(1)

#define has_clear_peaks(d)	((d)->features & FEAT_CLEAR_PEAKS)
#define needs_polling(d)	((d)->features & FEAT_NEEDS_POLLING)

static int ltc_wait_ready(struct i2c_client *client)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(LTC_POLL_TIMEOUT);
	const struct pmbus_driver_info *info = pmbus_get_driver_info(client);
	struct ltc2978_data *data = to_ltc2978_data(info);
	int status;
	u8 mask;

	if (!needs_polling(data))
		return 0;

	/*
	 * LTC3883 does not support LTC_NOT_PENDING, even though
	 * the datasheet claims that it does.
	 */
	mask = LTC_NOT_BUSY;
	if (data->id != ltc3883)
		mask |= LTC_NOT_PENDING;

	do {
		status = pmbus_read_byte_data(client, 0, LTC2978_MFR_COMMON);
		if (status == -EBADMSG || status == -ENXIO) {
			/* PEC error or NACK: chip may be busy, try again */
			usleep_range(50, 100);
			continue;
		}
		if (status < 0)
			return status;

		if ((status & mask) == mask)
			return 0;

		usleep_range(50, 100);
	} while (time_before(jiffies, timeout));

	return -ETIMEDOUT;
}
/* …（同文件无关代码省略）… */
static int ltc_read_word_data(struct i2c_client *client, int page, int phase,
			      int reg)
{
	int ret;

	ret = ltc_wait_ready(client);
	if (ret < 0)
		return ret;

	return pmbus_read_word_data(client, page, 0xff, reg);
}
/* …（同文件无关代码省略）… */
static int ltc_write_byte(struct i2c_client *client, int page, u8 byte)
{
	int ret;

	ret = ltc_wait_ready(client);
	if (ret < 0)
		return ret;

	return pmbus_write_byte(client, page, byte);
}
/* …（同文件无关代码省略）… */
static inline int lin11_to_val(int data)
{
	s16 e = ((s16)data) >> 11;
	s32 m = (((s16)(data << 5)) >> 5);

	/*
	 * mantissa is 10 bit + sign, exponent adds up to 15 bit.
	 * Add 6 bit to exponent for maximum accuracy (10 + 15 + 6 = 31).
	 */
	e += 6;
	return (e < 0 ? m >> -e : m << e);
}

static int ltc_get_max(struct ltc2978_data *data, struct i2c_client *client,
		       int page, int reg, u16 *pmax)
{
	int ret;

	ret = ltc_read_word_data(client, page, 0xff, reg);
	if (ret >= 0) {
		if (lin11_to_val(ret) > lin11_to_val(*pmax))
			*pmax = ret;
		ret = *pmax;
	}
/* …（同文件无关代码省略）… */

	ret = ltc_read_word_data(client, page, 0xff, reg);
	if (ret >= 0) {
		if (lin11_to_val(ret) < lin11_to_val(*pmin))
			*pmin = ret;
		ret = *pmin;
	}
/* …（同文件无关代码省略）… */
static int ltc2978_read_word_data_common(struct i2c_client *client, int page,
					 int reg)
{
	const struct pmbus_driver_info *info = pmbus_get_driver_info(client);
	struct ltc2978_data *data = to_ltc2978_data(info);
	int ret;

	switch (reg) {
	case PMBUS_VIRT_READ_VIN_MAX:
		ret = ltc_get_max(data, client, page, LTC2978_MFR_VIN_PEAK,
				  &data->vin_max);
		break;
	case PMBUS_VIRT_READ_VOUT_MAX:
		ret = ltc_read_word_data(client, page, 0xff,
					 LTC2978_MFR_VOUT_PEAK);
		if (ret >= 0) {
			/*
			 * VOUT is 16 bit unsigned with fixed exponent,
			 * so we can compare it directly
			 */
			if (ret > data->vout_max[page])
				data->vout_max[page] = ret;
			ret = data->vout_max[page];
		}
		break;
	case PMBUS_VIRT_READ_TEMP_MAX:
		ret = ltc_get_max(data, client, page,
				  LTC2978_MFR_TEMPERATURE_PEAK,
				  &data->temp_max[page]);
		break;
	case PMBUS_VIRT_RESET_VOUT_HISTORY:
	case PMBUS_VIRT_RESET_VIN_HISTORY:
	case PMBUS_VIRT_RESET_TEMP_HISTORY:
		ret = 0;
		break;
	default:
		ret = ltc_wait_ready(client);
		if (ret < 0)
			return ret;
		ret = -ENODATA;
		break;
	}
	return ret;
}
/* …（同文件无关代码省略）… */
static int ltc3880_read_word_data(struct i2c_client *client, int page,
				  int phase, int reg)
{
	const struct pmbus_driver_info *info = pmbus_get_driver_info(client);
	struct ltc2978_data *data = to_ltc2978_data(info);
	int ret;

	switch (reg) {
	case PMBUS_VIRT_READ_IOUT_MAX:
		ret = ltc_get_max(data, client, page, LTC3880_MFR_IOUT_PEAK,
				  &data->iout_max[page]);
		break;
	case PMBUS_VIRT_READ_TEMP2_MAX:
		ret = ltc_get_max(data, client, page,
				  LTC3880_MFR_TEMPERATURE2_PEAK,
				  &data->temp2_max);
		break;
	case PMBUS_VIRT_READ_VIN_MIN:
	case PMBUS_VIRT_READ_VOUT_MIN:
	case PMBUS_VIRT_READ_TEMP_MIN:
		ret = -ENXIO;
		break;
	case PMBUS_VIRT_RESET_IOUT_HISTORY:
	case PMBUS_VIRT_RESET_TEMP2_HISTORY:
		ret = 0;
		break;
	default:
		ret = ltc2978_read_word_data_common(client, page, reg);
		break;
	}
	return ret;
}
/* …（同文件无关代码省略）… */
static int ltc3883_read_word_data(struct i2c_client *client, int page,
				  int phase, int reg)
{
	const struct pmbus_driver_info *info = pmbus_get_driver_info(client);
	struct ltc2978_data *data = to_ltc2978_data(info);
	int ret;

	switch (reg) {
	case PMBUS
