// AUTO-DRAFT from torvalds/linux PR #3e516eae690b3f24c19168cc0fe605e1c610eb21
#include <linux/hwmon.h>
#include <linux/jiffies.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/unaligned.h>
/* …（同文件无关代码省略）… */
#define USB_PRODUCT_ID_FARBWERK		0xf00a
#define USB_PRODUCT_ID_QUADRO		0xf00d
#define USB_PRODUCT_ID_D5NEXT		0xf00e
#define USB_PRODUCT_ID_FARBWERK360	0xf010
#define USB_PRODUCT_ID_OCTO		0xf011
#define USB_PRODUCT_ID_HIGHFLOWNEXT	0xf012
#define USB_PRODUCT_ID_LEAKSHIELD	0xf014
#define USB_PRODUCT_ID_AQUASTREAMXT	0xf0b6
/* …（同文件无关代码省略）… */
#define USB_PRODUCT_ID_POWERADJUST3	0xf0bd
/* …（同文件无关代码省略）… */
enum kinds {
	d5next, farbwerk, farbwerk360, octo, quadro,
	highflownext, aquaero, poweradjust3, aquastreamult,
	aquastreamxt, leakshield, highflow
};
/* …（同文件无关代码省略）… */
#define AQUAERO_FAN_CURRENT_OFFSET		0x06
#define AQUAERO_FAN_POWER_OFFSET		0x08
#define AQUAERO_FAN_SPEED_OFFSET		0x00
static u16 aquaero_sensor_fan_offsets[] = { 0x167, 0x173, 0x17f, 0x18B };
  // <<< BUG ANCHOR
/* Control report offsets for the Aquaero fan controllers */
/* …（同文件无关代码省略）… */
#define D5NEXT_PUMP_OFFSET		0x6c
#define D5NEXT_FAN_OFFSET		0x5f
#define D5NEXT_5V_VOLTAGE		0x39
#define D5NEXT_12V_VOLTAGE		0x37
#define D5NEXT_VIRTUAL_SENSORS_START	0x3f
static u16 d5next_sensor_fan_offsets[] = { D5NEXT_PUMP_OFFSET, D5NEXT_FAN_OFFSET };

/* Control report offsets for the D5 Next pump */
/* …（同文件无关代码省略）… */
#define AQUASTREAMULT_PUMP_VOLTAGE		0x3D
/* …（同文件无关代码省略）… */
#define AQUASTREAMULT_PUMP_POWER		0x55
#define AQUASTREAMULT_FAN_OFFSET		0x41
#define AQUASTREAMULT_PRESSURE_OFFSET		0x57
#define AQUASTREAMULT_FLOW_SENSOR_OFFSET	0x37
/* …（同文件无关代码省略）… */
#define AQUASTREAMULT_FAN_CURRENT_OFFSET	0x00
#define AQUASTREAMULT_FAN_POWER_OFFSET		0x04
#define AQUASTREAMULT_FAN_SPEED_OFFSET		0x06
static u16 aquastreamult_sensor_fan_offsets[] = { AQUASTREAMULT_FAN_OFFSET };

/* Spec and sensor report offset for the Farbwerk RGB controller */
#define FARBWERK_NUM_SENSORS		4
#define FARBWERK_SENSOR_START		0x2f

/* Specs of the Farbwerk 360 RGB controller */
#define FARBWERK360_NUM_SENSORS			4
/* …（同文件无关代码省略）… */
/* Sensor report offsets for the Farbwerk 360 */
#define FARBWERK360_SENSOR_START		0x32
#define FARBWERK360_VIRTUAL_SENSORS_START	0x3a

/* Control report offsets for the Farbwerk 360 */
#define FARBWERK360_TEMP_CTRL_OFFSET		0x8
/* …（同文件无关代码省略）… */
#define OCTO_SENSOR_START		0x3D
#define OCTO_VIRTUAL_SENSORS_START	0x45
#define OCTO_FLOW_SENSOR_OFFSET		0x7B
static u16 octo_sensor_fan_offsets[] = { 0x7D, 0x8A, 0x97, 0xA4, 0xB1, 0xBE, 0xCB, 0xD8 };

/* Control report offsets for the Octo */
/* …（同文件无关代码省略）… */
#define QUADRO_SENSOR_START		0x34
#define QUADRO_VIRTUAL_SENSORS_START	0x3c
#define QUADRO_FLOW_SENSOR_OFFSET	0x6e
static u16 quadro_sensor_fan_offsets[] = { 0x70, 0x7D, 0x8A, 0x97 };

/* Control report offsets for the Quadro */
/* …（同文件无关代码省略）… */
#define HIGHFLOWNEXT_POWER		91
#define HIGHFLOWNEXT_CONDUCTIVITY	95
#define HIGHFLOWNEXT_5V_VOLTAGE		97
#define HIGHFLOWNEXT_5V_VOLTAGE_USB	99

/* Specs of the Leakshield */
#define LEAKSHIELD_NUM_SENSORS		2
/* …（同文件无关代码省略）… */
#define LEAKSHIELD_FLOW_IN		111
#define LEAKSHIELD_RESERVOIR_VOLUME	313
#define LEAKSHIELD_RESERVOIR_FILLED	311

/* Specs of the Aquastream XT pump */
#define AQUASTREAMXT_SERIAL_START		0x3a
/* …（同文件无关代码省略）… */
#define AQUASTREAMXT_PUMP_VOLTAGE_OFFSET	0x9
#define AQUASTREAMXT_PUMP_CURR_OFFSET		0xb
/* …（同文件无关代码省略）… */
struct aqc_fan_structure_offsets {
	u8 voltage;
	u8 curr;
	u8 power;
	u8 speed;
};
/* …（同文件无关代码省略）… */
struct aqc_data {
	struct hid_device *hdev;
	struct device *hwmon_dev;
	struct dentry *debugfs;
	enum kinds kind;
	const char *name;

	int status_report_id;	/* Used for legacy devices, report is stored in buffer */
	int ctrl_report_id;
	int secondary_ctrl_report_id;
	int secondary_ctrl_report_size;
	u8 *secondary_ctrl_report;

	ktime_t last_ctrl_report_op;
	int ctrl_report_delay;	/* Delay between two ctrl report operations, in ms */

	int buffer_size;
	u8 *buffer;
	int checksum_start;
	int checksum_length;
	int checksum_offset;

	int num_fans;
	u16 *fan_sensor_offsets;
	u16 *fan_ctrl_offsets;
	int num_temp_sensors;
	int temp_sensor_start_offset;
	int num_virtual_temp_sensors;
	int virtual_temp_sensor_start_offset;
	int num_calc_virt_temp_sensors;
	int calc_virt_temp_sensor_start_offset;
	u16 temp_ctrl_offset;
	u16 power_cycle_count_offset;
	int num_flow_sensors;
	u8 flow_sensors_start_offset;
	u8 flow_pulses_ctrl_offset;
	struct aqc_fan_structure_offsets *fan_structure;

	/* General info, same across all devices */
	u8 serial_number_start_offset;
	u32 serial_number[2];
	u8 firmware_version_offset;
	u16 firmware_version;

	/* How many times the device was powered on, if available */
	u32 power_cycles;

	/* Sensor values */
	s32 temp_input[20];	/* Max 4 physical and 16 virtual or 8 physical and 12 virtual */
	s32 speed_input[9];
	u32 speed_input_min[1];
	u32 speed_input_target[1];
	u32 speed_input_max[1];
	u32 power_input[8];
	u16 voltage_input[8];
	u16 current_input[8];

	/* Label values */
	const char *const *temp_label;
	const char *const *virtual_temp_label;
	const char *const *calc_virt_temp_label;	/* For Aquaero */
	const char *const *speed_label;
	const char *const *power_label;
	const char *const *voltage_label;
	const char *const *current_label;

	unsigned long updated;
};
/* …（同文件无关代码省略）… */

		/* Calculation derived from linear regression */
		sensor_value = get_unaligned_le16(priv->buffer + AQUASTREAMXT_PUMP_CURR_OFFSET);
		priv->current_input[0] = DIV_ROUND_CLOSEST(sensor_value * 176, 100) - 52;

		sensor_value = get_unaligned_le16(priv->buffer + AQUASTREAMXT_PUMP_VOLTAGE_OFFSET);
		priv->voltage_input[0] = DIV_ROUND_CLOSEST(sensor_value * 1000, 61);
/* …（同文件无关代码省略）… */
static int aqc_read(struct device *dev, enum hwmon_sensor_types type, u32 attr,
		    int channel, long *val)
{
	int ret;
	struct aqc_data *priv = dev_get_drvdata(dev);

/* …（同文件无关代码省略）… */
		}
		break;
	case hwmon_power:
		*val = priv->power_input[channel];
		break;
	case hwmon_pwm:
		switch (priv->kind) {
/* …（同文件无关代码省略）… */
		return 0;

	priv = hid_get_drvdata(hdev);

	/* Info provided with every report */
	priv->serial_number[0] = get_unaligned_be16(data + priv->serial_number_start_offset);
/* …（同文件无关代码省略）… */
		priv->speed_input[i] =
		    get_unaligned_be16(data + priv->fan_sensor_offsets[i] +
				       priv->fan_structure->speed);
		priv->power_input[i] =
		    get_unaligned_be16(data + priv->fan_sensor_offsets[i] +
				       priv->fan_structure->power) * 10000;
		priv->voltage_input[i] =
		    get_unaligned_be16(data + priv->fan_sensor_offsets[i] +
				       priv->fan_structure->voltage) * 10;
/* …（同文件无关代码省略）… */
		priv->speed_input[2] = get_unaligned_be16(data + AQUASTREAMULT_PRESSURE_OFFSET);
		priv->speed_input[3] = get_unaligned_be16(data + AQUASTREAMULT_FLOW_SENSOR_OFFSET);

		priv->power_input[1] = get_unaligned_be16(data + AQUASTREAMULT_PUMP_POWER) * 10000;

		priv->voltage_input[1] = get_unaligned_be16(data + AQUASTREAMULT_PUMP_VOLTAGE) * 10;

/* …（同文件无关代码省略）… */
	case highflownext:
		/* If external temp sensor is not connected, its power reading is also N/A */
		if (priv->temp_input[1] == -ENODATA)
			priv->power_input[0] = -ENODATA;
		else
			priv->power_input[0] =
			    get_unaligned_be16(data + HIGHFLOWNEXT_POWER) * 1000000;

		priv->voltage_input[0] = get_unaligned_be16(data + HIGHFLOWNEXT_5V_VOLTAGE) * 10;
		priv->voltage_input[1] =
/* …（同文件无关代码省略）… */
static int aqc_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	struct aqc_data *priv;
	int ret;

	priv = devm_kzalloc(&hdev->dev, sizeof(*priv), GFP_KERNEL);
/* …（同文件无关代码省略）… */
		 * they present. The two other devices have the type of the second element in
		 * their respective collections set to 1, while the real device has it set to 0.
		 */
		if (hdev->collection[1].type != 0) {
			ret = -ENODEV;
			goto fail_and_close;
		}
/* …（同文件无关代码省略）… */
		priv->power_label = label_fan_power;
		priv->voltage_label = label_fan_voltage;
		priv->current_label = label_fan_current;
		break;
	case USB_PRODUCT_ID_D5NEXT:
		priv->kind = d5next;
/* …（同文件无关代码省略）… */
		priv->power_label = label_d5next_power;
		priv->voltage_label = label_d5next_voltages;
		priv->current_label = label_d5next_current;
		break;
	case USB_PRODUCT_ID_FARBWERK:
		priv->kind = farbwerk;
/* …（同文件无关代码省略）… */
		priv->temp_sensor_start_offset = FARBWERK_SENSOR_START;

		priv->temp_label = label_temp_sensors;
		break;
	case USB_PRODUCT_ID_FARBWERK360:
		priv->kind = farbwerk360;
/* …（同文件无关代码省略）… */

		priv->temp_label = label_temp_sensors;
		priv->virtual_temp_label = label_virtual_temp_sensors;
		break;
	case USB_PRODUCT_ID_OCTO:
		priv->kind = octo;
/* …（同文件无关代码省略）… */
		priv->power_label = label_fan_power;
		priv->voltage_label = label_fan_voltage;
		priv->current_label = label_fan_current;
		break;
	case USB_PRODUCT_ID_QUADRO:
		priv->kind = quadro;
/* …（同文件无关代码省略）… */
		priv->power_label = label_fan_power;
		priv->voltage_label = label_fan_voltage;
		priv->current_label = label_fan_current;
		break;
	case USB_PRODUCT_ID_HIGHFLOWNEXT:
		priv->kind = highflownext;
/* …（同文件无关代码省略）… */
		priv->speed_label = label_highflownext_fan_speed;
		priv->power_label = label_highflownext_power;
		priv->voltage_label = label_highflownext_voltage;
		break;
	case USB_PRODUCT_ID_LEAKSHIELD:
		/*
/* …（同文件无关代码省略）… */

		priv->temp_label = label_leakshield_temp_sensors;
		priv->speed_label = label_leakshield_fan_speed;
		break;
	case USB_PRODUCT_ID_AQUASTREAMXT:
		priv->kind = aquastreamxt;
/* …（同文件无关代码省略）… */
		priv->power_label = label_aquastreamult_power;
		priv->voltage_label = label_aquastreamult_voltages;
		priv->current_label = label_aquastreamult_current;
		break;
	case USB_PRODUCT_ID_POWERADJUST3:
		priv->kind = poweradjust3;
/* …（同文件无关代码省略）… */
		break;
	}

	if (priv->buffer_size != 0) {
		priv->checksum_start = 0x01;
		priv->checksum_length = priv->buffer_size - 3;
