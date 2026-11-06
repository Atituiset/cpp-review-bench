// AUTO-DRAFT from torvalds/linux PR #3e516eae690b3f24c19168cc0fe605e1c610eb21
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define ACPI_POWER_METER_NAME		"power_meter"
/* …（同文件无关代码省略）… */
#define POWER_METER_CAN_MEASURE	(1 << 0)
/* …（同文件无关代码省略）… */
#define POWER_METER_CAN_CAP	(1 << 2)
#define POWER_METER_CAN_NOTIFY	(1 << 3)
#define POWER_METER_IS_BATTERY	(1 << 8)
#define UNKNOWN_HYSTERESIS	0xFFFFFFFF
/*
 * ACPI integers can either be 32-bit or 64-bit wide, so
 * we have to check for both values.
 */
#define UNKNOWN_POWER_32	U32_MAX
#define UNKNOWN_POWER_64	U64_MAX

#define METER_NOTIFY_CONFIG	0x80
#define METER_NOTIFY_TRIP	0x81
/* …（同文件无关代码省略）… */

static DEFINE_MUTEX(acpi_notify_lock);

static int can_cap_in_hardware(void)
{
	return force_cap_on || cap_in_hardware;
/* …（同文件无关代码省略）… */
struct acpi_power_meter_resource {
	struct acpi_device	*acpi_dev;
	acpi_bus_id		name;
	struct mutex		lock;
	struct device		*hwmon_dev;
	struct acpi_power_meter_capabilities	caps;
	acpi_string		model_number;
	acpi_string		serial_number;
	acpi_string		oem_info;
	u64		power;
	u64		cap;
	u64		avg_interval;
	bool		power_alarm;
	int			sensors_valid;
	unsigned long		sensors_last_updated;
#define POWER_METER_TRIP_AVERAGE_MIN_IDX	0
#define POWER_METER_TRIP_AVERAGE_MAX_IDX	1
	s64			trip[2];
	int			num_domain_devices;
	struct acpi_device	**domain_devices;
	struct kobject		*holders_dir;
};
/* …（同文件无关代码省略）… */
static int update_avg_interval(struct acpi_power_meter_resource *resource)
{
	unsigned long long data;
	acpi_status status;

	status = acpi_evaluate_integer(resource->acpi_dev->handle, "_GAI",
				       NULL, &data);
	if (ACPI_FAILURE(status)) {
		acpi_evaluation_failure_warn(resource->acpi_dev->handle, "_GAI",
					     status);
		return -ENODEV;
	}

	resource->avg_interval = data;
	return 0;
}
/* …（同文件无关代码省略）… */
static int update_cap(struct acpi_power_meter_resource *resource)
{
	unsigned long long data;
	acpi_status status;

	status = acpi_evaluate_integer(resource->acpi_dev->handle, "_GHL",
				       NULL, &data);
	if (ACPI_FAILURE(status)) {
		acpi_evaluation_failure_warn(resource->acpi_dev->handle, "_GHL",
					     status);
		return -ENODEV;
	}

	resource->cap = data;
	return 0;
}
/* …（同文件无关代码省略）… */
static int update_meter(struct acpi_power_meter_resource *resource)
{
	unsigned long long data;
	acpi_status status;
	unsigned long local_jiffies = jiffies;

	if (time_before(local_jiffies, resource->sensors_last_updated +
			msecs_to_jiffies(resource->caps.sampling_time)) &&
			resource->sensors_valid)
		return 0;

	status = acpi_evaluate_integer(resource->acpi_dev->handle, "_PMM",
				       NULL, &data);
	if (ACPI_FAILURE(status)) {
		acpi_evaluation_failure_warn(resource->acpi_dev->handle, "_PMM",
					     status);
		return -ENODEV;
	}

	resource->power = data;
	resource->sensors_valid = 1;
	resource->sensors_last_updated = jiffies;
	return 0;
}
/* …（同文件无关代码省略）… */
	return 0;
}

static int get_power_alarm_state(struct acpi_power_meter_resource *resource,
				 long *val)
{
	int ret;

	ret = update_meter(resource);
	if (ret)
		return ret;

	/* need to update cap if not to support the notification. */
	if (!(resource->caps.flags & POWER_METER_CAN_NOTIFY)) {
		ret = update_cap(resource);
		if (ret)
			return ret;
		resource->power_alarm = resource->power > resource->cap;
		*val = resource->power_alarm;
	} else {
		*val = resource->power_alarm || resource->power > resource->cap;
		resource->power_alarm = resource->power > resource->cap;
	}

	return 0;
}

static umode_t power_meter_is_visible(const void *data,
				      enum hwmon_sensor_types type,
				      u32 attr, int channel)
{
	const struct acpi_power_meter_resource *res = data;

	if (type != hwmon_power)
		return 0;

	switch (attr) {
	case hwmon_power_average:
	case hwmon_power_average_interval_min:
	case hwmon_power_average_interval_max:
		if (res->caps.flags & POWER_METER_CAN_MEASURE)
			return 0444;
		break;
	case hwmon_power_average_interval:
		if (res->caps.flags & POWER_METER_CAN_MEASURE)
			return 0644;
		break;
	case hwmon_power_cap_min:
	case hwmon_power_cap_max:
	case hwmon_power_alarm:
		if (res->caps.flags & POWER_METER_CAN_CAP && can_cap_in_hardware())
			return 0444;
		break;
	case hwmon_power_cap:
		if (res->caps.flags & POWER_METER_CAN_CAP && can_cap_in_hardware()) {
			if (res->caps.configurable_cap)
				return 0644;
			else
				return 0444;
		}
		break;
	default:
		break;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
		ret = update_meter(res);
		if (ret)
			return ret;
		if (res->power == UNKNOWN_POWER_32 || res->power == UNKNOWN_POWER_64)
			return -ENODATA;
		*val = res->power * 1000;
		break;
	case hwmon_power_average_interval_min:
		*val = res->caps.min_avg_interval;
		break;
	case hwmon_power_average_interval_max:
		*val = res->caps.max_avg_interval;
		break;
	case hwmon_power_average_interval:
		ret = update_avg_interval(res);
		if (ret)
			return ret;
		*val = (res)->avg_interval;
		break;
	case hwmon_power_cap_min:
		*val = res->caps.min_cap * 1000;
		break;
	case hwmon_power_cap_max:
		*val = res->caps.max_cap * 1000;
		break;
	case hwmon_power_alarm:
		ret = get_power_alarm_state(res, val);
		if (ret)
			return ret;
		break;
	case hwmon_power_cap:
		ret = update_cap(res);
		if (ret)
			return ret;
		*val = res->cap * 1000;
		break;
	default:
/* …（同文件无关代码省略）… */
{
	struct acpi_power_meter_resource *res = dev_get_drvdata(dev);

	if (res->trip[POWER_METER_TRIP_AVERAGE_MIN_IDX] < 0)
		return sysfs_emit(buf, "unknown\n");

/* …（同文件无关代码省略）… */
{
	struct acpi_power_meter_resource *res = dev_get_drvdata(dev);

	if (res->trip[POWER_METER_TRIP_AVERAGE_MAX_IDX] < 0)
		return sysfs_emit(buf, "unknown\n");

/* …（同文件无关代码省略）… */
{
	struct acpi_power_meter_resource *res = dev_get_drvdata(dev);

	if (res->caps.hysteresis == UNKNOWN_HYSTERESIS)
		return sysfs_emit(buf, "unknown\n");

	return sysfs_emit(buf, "%llu\n", res->caps.hysteresis * 1000);
/* …（同文件无关代码省略）… */
				    char *buf)
{
	struct acpi_power_meter_resource *res = dev_get_drvdata(dev);
	unsigned int acc = res->caps
