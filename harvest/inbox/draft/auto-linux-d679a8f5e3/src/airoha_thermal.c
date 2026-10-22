// AUTO-DRAFT from torvalds/linux PR #4403278e4ea2f654f8130a5c03eb8b4516a0f201
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
/* …（同文件无关代码省略）… */
#define EN7581_PLLRG_PROTECT			0x268
/* …（同文件无关代码省略）… */
#define EN7581_DOUT_TADC			0x2f8
#define   EN7581_DOUT_TADC_MASK			GENMASK(15, 0)

#define AN7583_MUX_SENSOR			0x2a0
#define   AN7583_LOAD_ADJ			GENMASK(3, 2)
#define AN7583_MUX_TADC				0x2e4
#define   AN7583_MUX_TADC_MASK			GENMASK(3, 1)
#define AN7583_DOUT_TADC			0x2f0

/* PTP_THERMAL regs */
/* …（同文件无关代码省略）… */
#define EN7581_TEMPMONCTL1			0x804
/* …（同文件无关代码省略）… */
#define   EN7581_PERIOD_UNIT			GENMASK(9, 0)
#define EN7581_TEMPMONCTL2			0x808
#define   EN7581_FILT_INTERVAL			GENMASK(25, 16)
#define   EN7581_SEN_INTERVAL			GENMASK(9, 0)
#define EN7581_TEMPMONINT			0x80C
/* …（同文件无关代码省略）… */
#define   EN7581_LOFSINTEN0			BIT(3) /* In documentation: BIT(2) */
#define   EN7581_HOFSINTEN0			BIT(2) /* In documentation: BIT(3) */
/* …（同文件无关代码省略）… */
#define EN7581_TEMPMONINTSTS			0x810
/* …（同文件无关代码省略）… */
#define   EN7581_LOFSINTSTS0			BIT(3) /* In documentation: BIT(2) */
#define   EN7581_HOFSINTSTS0			BIT(2) /* In documentation: BIT(3) */
/* …（同文件无关代码省略）… */
#define EN7581_TEMPMSRCTL0			0x838
/* …（同文件无关代码省略）… */
#define   EN7581_MSRCTL0			GENMASK(2, 0)
#define EN7581_TEMPADCVALIDADDR			0x878
/* …（同文件无关代码省略）… */
#define EN7581_TEMPADCVALIDMASK			0x884
/* …（同文件无关代码省略）… */
#define   EN7581_ADV_RD_VALID_POS		GENMASK(4, 0)
#define EN7581_TEMPADCVOLTAGESHIFT		0x888
#define   EN7581_ADC_VOLTAGE_SHIFT		GENMASK(4, 0)
/* …（同文件无关代码省略）… */
#define   EN7581_MSRCTL_6SAMPLE_MAX_MIX_AVG4	0x3
/* …（同文件无关代码省略）… */
#define EN7581_TEMPAHBPOLL			0x840
#define   EN7581_ADC_POLL_INTVL			GENMASK(31, 0)
/* …（同文件无关代码省略）… */
#define EN7581_EFUSE_TEMP_OFFSET_REG		0xf20 /* PTPSPARE0 */
#define   EN7581_EFUSE_TEMP_OFFSET		GENMASK(31, 16)
/* …（同文件无关代码省略）… */
#define EN7581_EFUSE_TEMP_CPU_SENSOR_REG	0xf28 /* PTPSPARE2 */
/* …（同文件无关代码省略）… */
#define EN7581_SLOPE_X100_DIO_DEFAULT		5645
#define EN7581_SLOPE_X100_DIO_AVS		5645
/* …（同文件无关代码省略）… */
#define EN7581_INIT_TEMP_CPK_X10		300
#define EN7581_INIT_TEMP_FTK_X10		620
#define EN7581_INIT_TEMP_NONK_X10		550
/* …（同文件无关代码省略）… */
#define EN7581_SCU_THERMAL_PROTECT_KEY		0x12
#define EN7581_SCU_THERMAL_MUX_DIODE1		0x7
/* …（同文件无关代码省略）… */
#define AN7583_SCU_THERMAL_PROTECT_KEY		0x80
#define AN7583_NUM_SENSOR			3

#define AIROHA_THERMAL_NO_MUX_SENSOR		-1

/* Convert temp to raw value as read from ADC	((((temp / 100) - init) * slope) / 1000) + offset */
#define TEMP_TO_RAW(priv, temp)			((((((temp) / 100) - (priv)->init_temp) * \
/* …（同文件无关代码省略）… */
	AIROHA_THERMAL_FIELD_MAX,
};

struct airoha_thermal_priv {
	struct regmap *map;
	struct regmap *chip_scu;
	struct regmap_field *chip_scu_fields[AIROHA_THERMAL_FIELD_MAX];
	struct resource scu_adc_res;

	u32 pllrg_protect;
	int current_adc;

	struct thermal_zone_device *tz;
	int init_temp;
/* …（同文件无关代码省略）… */

struct airoha_thermal_soc_data {
	u32 pllrg_protect;

	const struct thermal_zone_device_ops *thdev_ops;
	int (*probe)(struct platform_device *pdev,
/* …（同文件无关代码省略）… */
static int airoha_get_thermal_ADC(struct airoha_thermal_priv *priv)
{
	u32 val;

	regmap_field_read(priv->chip_scu_fields[AIROHA_THERMAL_DOUT_TADC],
			  &val);
	return val;
}

static void airoha_set_thermal_mux(struct airoha_thermal_priv *priv,
				   int tdac_idx, int sensor_idx)
{
	u32 pllrg;

	/* Save PLLRG current value */
/* …（同文件无关代码省略）… */
		     priv->pllrg_protect);

	/*
	 * Configure Thermal Sensor mux to sensor_idx.
	 * (if not supported, sensor_idx is AIROHA_THERMAL_NO_MUX_SENSOR)
	 */
	if (sensor_idx != AIROHA_THERMAL_NO_MUX_SENSOR)
		regmap_field_write(priv->chip_scu_fields[AIROHA_THERMAL_MUX_SENSOR],
				   sensor_idx);

	/* Configure Thermal ADC mux to tdac_idx */
	if (priv->current_adc != tdac_idx) {
		regmap_field_write(priv->chip_scu_fields[AIROHA_THERMAL_MUX_TADC],
				   tdac_idx);
		priv->current_adc = tdac_idx;
	}

	/* Restore PLLRG value on exit */
	regmap_write(priv->chip_scu, EN7581_PLLRG_PROTECT, pllrg);
/* …（同文件无关代码省略）… */

static int en7581_thermal_get_temp(struct thermal_zone_device *tz, int *temp)
{
	struct airoha_thermal_priv *priv = thermal_zone_device_priv(tz);
	int min_value, max_value, avg_value, value;
	int i;

/* …（同文件无关代码省略）… */
static int en7581_thermal_set_trips(struct thermal_zone_device *tz, int low,
				    int high)
{
	struct airoha_thermal_priv *priv = thermal_zone_device_priv(tz);
	bool enable_monitor = false;

	if (high != INT_MAX) {
/* …（同文件无关代码省略）… */
static irqreturn_t en7581_thermal_irq(int irq, void *data)
{
	struct airoha_thermal_priv *priv = data;
	enum thermal_notify_event event;
	bool update = false;
	u32 status = 0;

	regmap_read(priv->map, EN7581_TEMPMONINTSTS, &status);
	switch (status & (EN7581_HOFSINTSTS0 | EN7581_LOFSINTSTS0)) {
	case EN7581_HOFSINTSTS0:
		event = THERMAL_TRIP_VIOLATED;
		update = true;
		break;
	case EN7581_LOFSINTSTS0:
		event = THERMAL_EVENT_UNSPECIFIED;
		update = true;
		break;
	default:
		/* Should be impossible as we enable only these Interrupt */
		break;
	}

	/* Reset Interrupt */
	regmap_write(priv->map, EN7581_TEMPMONINTSTS, status);

	if (update)
		thermal_zone_device_update(priv->tz, event);

	return IRQ_HANDLED;
}
/* …（同文件无关代码省略）… */
static void en7581_thermal_setup_adc_val(struct device *dev,
					 struct airoha_thermal_priv *priv)
{
	u32 efuse_calib_info = 0;
	u32 cpu_sensor = 0;

	/* Setup Thermal Sensor to ADC mode and setup the mux to DIODE1 */
	airoha_set_thermal_mux(priv, EN7581_SCU_THERMAL_MUX_DIODE1,
			       AIROHA_THERMAL_NO_MUX_SENSOR);

	regmap_read(priv->map, EN7581_EFUSE_TEMP_OFFSET_REG, &efuse_calib_info);
	if (efuse_calib_info) {
		priv->default_offset = FIELD_GET(EN7581_EFUSE_TEMP_OFFSET, efuse_calib_info);
		/* Different slope are applied if the sensor is used for CPU or for package */
		regmap_read(
