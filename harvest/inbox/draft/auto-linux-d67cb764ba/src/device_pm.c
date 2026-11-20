// AUTO-DRAFT from torvalds/linux PR #75099ce6829ff89464cb7dd32471024c6c17c1de
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define ACPI_D_STATE_INVALID	ACPI_D_STATE_COUNT
/* …（同文件无关代码省略）… */
const char *acpi_power_state_string(int state)
{
	switch (state) {
	case ACPI_STATE_D0:
		return "D0";
	case ACPI_STATE_D1:
		return "D1";
	case ACPI_STATE_D2:
		return "D2";
	case ACPI_STATE_D3_HOT:
		return "D3hot";
	case ACPI_STATE_D3_COLD:
		return "D3cold";
	default:
		return "(unknown)";
	}
}
/* …（同文件无关代码省略）… */
static int acpi_dev_pm_explicit_get(struct acpi_device *device, int *state)
{
	unsigned long long psc;
	acpi_status status;

	status = acpi_evaluate_integer(device->handle, "_PSC", NULL, &psc);
	if (ACPI_FAILURE(status))
		return -ENODEV;

	*state = psc;
	return 0;
}
/* …（同文件无关代码省略）… */
int acpi_device_get_power(struct acpi_device *device, int *state)
{
	int result = ACPI_STATE_UNKNOWN;
	int error;

	if (!device || !state)
		return -EINVAL;

	if (!device->flags.power_manageable) {
		struct acpi_device *parent = acpi_dev_parent(device);

		/* TBD: Non-recursive algorithm for walking up hierarchy. */
		*state = parent ? parent->power.state : ACPI_STATE_D0;
		goto out;
	}

	/*
	 * Get the device's power state from power resources settings and _PSC,
	 * if available.
	 */
	if (device->power.flags.power_resources) {
		error = acpi_power_get_inferred_state(device, &result);
		if (error)
			return error;
	}
	if (device->power.flags.explicit_get) {
		int psc;

		error = acpi_dev_pm_explicit_get(device, &psc);
		if (error)
			return error;

		/*
		 * The power resources settings may indicate a power state
		 * shallower than the actual power state of the device, because
		 * the same power resources may be referenced by other devices.
		 *
		 * For systems predating ACPI 4.0 we assume that D3hot is the
		 * deepest state that can be supported.
		 */
		if (psc > result && psc < ACPI_STATE_D3_COLD)
			result = psc;
		else if (result == ACPI_STATE_UNKNOWN)
			result = psc > ACPI_STATE_D2 ? ACPI_STATE_D3_HOT : psc;
	}

	*state = result;

 out:
	acpi_handle_debug(device->handle, "Power state: %s\n",
			  acpi_power_state_string(*state));

	return 0;
}
/* …（同文件无关代码省略）… */
static int acpi_dev_pm_explicit_set(struct acpi_device *adev, int state)
{
	if (adev->power.states[state].flags.explicit_set) {
		char method[5] = { '_', 'P', 'S', '0' + state, '\0' };
		acpi_status status;

		status = acpi_evaluate_object(adev->handle, method, NULL, NULL);
		if (ACPI_FAILURE(status))
			return -ENODEV;
	}
	return 0;
}
/* …（同文件无关代码省略）… */
static int acpi_device_init_power(struct acpi_device *device)
{
	int state;
	int result;

	result = acpi_device_get_power(device, &state);
	if (result)
		return result;

	/*
	 * If the current power state of the device is D0 and it has a parent
	 * whose power state is not ignored, and the parent's power state
	 * initialization has failed, the parent's power state can be updated to
	 * D0 for consistency.
	 */
	if (!device->power.flags.ignore_parent && state == ACPI_STATE_D0) {
		struct acpi_device *parent = acpi_dev_parent(device);

		if (parent && parent->power.state == ACPI_D_STATE_INVALID)
			parent->power.state = ACPI_STATE_D0;
	}

	if (state < ACPI_STATE_D3_COLD && device->power.flags.power_resources) {
		/* Reference count the power resources. */
		result = acpi_power_on_resources(device, state);
		if (result)
			return result;

		if (state == ACPI_STATE_D0) {
			/*
			 * If _PSC is not present and the state inferred from
			 * power resources appears to be D0, it still may be
			 * necessary to execute _PS0 at this point, because
			 * another device using the same power resources may
			 * have been put into D0 previously and that's why we
			 * see D0 here.
			 */
			result = acpi_dev_pm_explicit_set(device, state);
			if (result)
				return result;
		}
	} else if (state == ACPI_STATE_UNKNOWN) {
		/*
		 * No power resources and missing _PSC?  Cross fingers and make
		 * it D0 in hope that this is what the BIOS put the device into.
		 * [We tried to force D0 here by executing _PS0, but that broke
		 * Toshiba P870-303 in a nasty way.]
		 */
		state = ACPI_STATE_D0;
	}
	device->power.state = state;

	acpi_handle_debug(device->handle, "Initial power state: %s\n",
			  acpi_power_state_string(state));

	return 0;
}
/* …（同文件无关代码省略）… */
	 * fails, ACPI power management will not be used for the device going
	 * forward.
	 */
	result = acpi_device_init_power(device);
	if (result) {
		device->flags.power_manageable = 0;
		device->power.state = ACPI_D_STATE_INVALID;
		acpi_handle_info(device->handle,
				 "Initial power state undetermined, ACPI PM disabled\n");
	}

	return result;
}
