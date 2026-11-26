// AUTO-DRAFT from torvalds/linux PR #ebb7a8dc1e74c705daffe11c2faf38ec2628f838
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
struct scmi_requested_dev {
	const struct scmi_device_id *id_table;
	struct list_head node;
};
/* …（同文件无关代码省略）… */
/* Track globally the SCMI SystemPower protocol device. */
static struct scmi_device *scmi_syspower_registered;

/**
 * scmi_protocol_device_request  - Helper to request a device
 *
/* …（同文件无关代码省略）… */
static int scmi_protocol_device_request(const struct scmi_device_id *id_table)
{
	int ret = 0;
	struct list_head *head, *phead = NULL;
	struct scmi_requested_dev *rdev;

	pr_debug("Requesting SCMI device (%s) for protocol %x\n",
		 id_table->name, id_table->protocol_id);

	if (IS_ENABLED(CONFIG_ARM_SCMI_RAW_MODE_SUPPORT) &&
	    !IS_ENABLED(CONFIG_ARM_SCMI_RAW_MODE_SUPPORT_COEX)) {
		pr_warn("SCMI Raw mode active. Rejecting '%s'/0x%02X\n",
			id_table->name, id_table->protocol_id);
		return -EINVAL;
	}

	/*
	 * Find the matching protocol rdev list and then search of any
	 * existent equally named device...fails if any duplicate found.
	 */
	mutex_lock(&scmi_requested_devices_mtx);
	phead = idr_find(&scmi_requested_devices, id_table->protocol_id);
	if (phead) {
		head = phead;
		list_for_each_entry(rdev, head, node) {
			if (!strcmp(rdev->id_table->name, id_table->name)) {
				pr_err("Ignoring duplicate request [%d] %s\n",
				       rdev->id_table->protocol_id,
				       rdev->id_table->name);
				ret = -EINVAL;
				goto out;
			}
		}
	}

	/*
	 * No duplicate found for requested id_table, so let's create a new
	 * requested device entry for this new valid request.
	 */
	rdev = kzalloc_obj(*rdev);
	if (!rdev) {
		ret = -ENOMEM;
		goto out;
	}
	rdev->id_table = id_table;

	/*
	 * Append the new requested device table descriptor to the head of the
	 * related protocol list, eventually creating such head if not already
	 * there.
	 */
	if (!phead) {
		phead = kzalloc_obj(*phead);
		if (!phead) {
			kfree(rdev);
			ret = -ENOMEM;
			goto out;
		}
		INIT_LIST_HEAD(phead);

		ret = idr_alloc(&scmi_requested_devices, (void *)phead,
				id_table->protocol_id,
				id_table->protocol_id + 1, GFP_KERNEL);
		if (ret != id_table->protocol_id) {
			pr_err("Failed to save SCMI device - ret:%d\n", ret);
			kfree(rdev);
			kfree(phead);
			ret = -EINVAL;
			goto out;
		}
		ret = 0;
	}
	list_add(&rdev->node, phead);

out:
	mutex_unlock(&scmi_requested_devices_mtx);

	if (!ret)
		blocking_notifier_call_chain(&scmi_requested_devices_nh,
					     SCMI_BUS_NOTIFY_DEVICE_REQUEST,
					     (void *)rdev->id_table);

	return ret;
}
/* …（同文件无关代码省略）… */
static void scmi_protocol_device_unrequest(const struct scmi_device_id *id_table)
{
	struct scmi_requested_dev *rdev, *victim = NULL;
	struct list_head *phead;

	pr_debug("Unrequesting SCMI device (%s) for protocol %x\n",
		 id_table->name, id_table->protocol_id);

	mutex_lock(&scmi_requested_devices_mtx);
	phead = idr_find(&scmi_requested_devices, id_table->protocol_id);
	if (phead) {
		list_for_each_entry(rdev, phead, node) {
			if (!strcmp(rdev->id_table->name, id_table->name)) {
				victim = rdev;
				list_del(&rdev->node);
				break;
			}
		}

		if (victim && list_empty(phead)) {
			idr_remove(&scmi_requested_devices,
				   id_table->protocol_id);
			kfree(phead);
		}
	}
	mutex_unlock(&scmi_requested_devices_mtx);

	if (victim) {
		blocking_notifier_call_chain(&scmi_requested_devices_nh,
					     SCMI_BUS_NOTIFY_DEVICE_UNREQUEST,
					     (void *)victim->id_table);
		kfree(victim);
	}
}

static int scmi_protocol_table_register(const struct scmi_device_id *id_table)
{
	const struct scmi_device_id *entry;
	int ret;

	for (entry = id_table; entry->name[0]; entry++) {
		ret = scmi_protocol_device_request(entry);
		if (ret)
			goto err_unrequest;
/* …（同文件无关代码省略）… */
	return 0;

err_unrequest:
	while (entry != id_table)
		scmi_protocol_device_unrequest(--entry);

	return ret;
}
/* …（同文件无关代码省略）… */
{
	const struct scmi_device_id *entry;

	for (entry = id_table; entry->name[0]; entry++)
		scmi_protocol_device_unrequest(entry);
}

static bool scmi_device_is_transport(const struct scmi_device *scmi_dev)
{
	return !strncmp(scmi_dev->name, SCMI_TRANSPORT_DEVNAME_PREFIX,
			strlen(SCMI_TRANSPORT_DEVNAME_PREFIX));
}

static int __scmi_dev_match_by_id_table(struct scmi_device *scmi_dev,
					const struct scmi_device_id *id_table,
					bool skip_transport)
/* …（同文件无关代码省略）… */
static int scmi_match_by_id_table(struct device *dev, const void *data)
{
	struct scmi_device *scmi_dev = to_scmi_dev(dev);
	const struct scmi_device_id *id_table = data;

	return __scmi_dev_match_by_id_table(scmi_dev, id_table, false);
}
/* …（同文件无关代码省略）… */
static struct scmi_device *
scmi_child_dev_find_get(struct device *parent, int prot_id, const char *name)
{
	struct scmi_device_id id_table[2] = { 0 };
	struct device *dev;

	id_table[0].protocol_id = prot_id;
	strscpy(id_table[0].name, name, sizeof(id_table[0].name));

	dev = device_find_child(parent, &id_table, scmi_match_by_id_table);
	if (!dev)
		return NULL;

	return to_scmi_dev(dev);
}
/* …（同文件无关代码省略）… */
static void scmi_device_release_resources(struct scmi_device *scmi_dev)
{
	if (scmi_dev->protocol_id == SCMI_PROTOCOL_SYSTEM)
		cmpxchg(&scmi_syspower_registered, scmi_dev, NULL);

	if (scmi_dev->id) {
		ida_free(&scmi_bus_id, scmi_dev->id);
		scmi_dev->id = 0;
	}
}
/* …（同文件无关代码省略）… */
static void scmi_device_release(struct device *dev)
{
	struct scmi_device *scmi_dev = to_scmi_dev(dev);

	scmi_device_release_resources(scmi_dev);
	fwnode_handle_put(dev_fwnode(dev));
	kfree_const(scmi_dev->name);
	kfree(scmi_dev);
}
/* …（同文件无关代码省略）… */
static struct scmi_device *
__scmi_device_create(struct fwnode_handle *fwnode, struct device *parent,
		     int protocol, const char *name)
{
	int id, retval;
	struct scmi_device *scmi_dev;
	bool syspower = (protocol == SCMI_PROTOCOL_SYSTEM);

	/*
	 * If the same protocol/name device already exist under the same parent
	 * (i.e. SCMI instance) just return the existent device.
	 * This avoids any race between the SCMI driver, creating devices for
	 * each fwnode defined protocol at probe time, and the concurrent
	 * registration of SCMI drivers.
	 */
	scmi_dev = scmi_child_dev_find_get(parent, protocol, name);
	if (scmi_dev) {
		put_device(&scmi_dev->dev);
		return scmi_dev;
	}

	scmi_dev = kzalloc_obj(*scmi_dev);
	if (!scmi_dev)
		return NULL;

	scmi_dev->protocol_id = protocol;

	/*
	 * Reserve the singleton SystemPower protocol device using the device
	 * pointer itself, so delayed release of an older device cannot clear
	 * a reservation owned by a newer device.
	 */
	if (syspower && cmpxchg(&scmi_syspower_registered, NULL, scmi_dev)) {
		dev_warn(parent,
			 "SCMI SystemPower protocol device must be unique !\n");
		kfree(scmi_dev);
		return NULL;
	}

	scmi_dev->name = kstrdup_const(name ?: "unknown", GFP_KERNEL);
	if (!scmi_dev->name)
		goto free_dev;

	id = ida_alloc_min(&scmi_bus_id, 1, GFP_KERNEL);
	if (id < 0)
		goto free_name;

	scmi_dev->id = id;
	scmi_dev->dev.parent = parent;
	device_set_node(&scmi_dev->dev, fwnode_handle_get(fwnode));
	scmi_dev->dev.bus = &scmi_bus_type;
	scmi_dev->dev.release = scmi_device_release;
	dev_set_name(&scmi_dev->dev, "scmi_dev.%d", id);

	retval = device_register(&scmi_dev->dev);
	if (retval)
		goto put_dev;

	pr_debug("(%pfwf) Created SCMI device '%s' - protocol 0x%x (%s)\n",
		 fwnode, dev_name(&scmi_dev->dev), protocol, name);

	return scmi_dev;
put_dev:
	scmi_device_release_resources(scmi_dev);
	put_device(&scmi_dev->dev);
	return NULL;
free_name:
	kfree_const(scmi_dev->name);
free_dev:
	scmi_device_release_resources(scmi_dev);
	kfree(scmi_dev);
	return NULL;
}
/* …（同文件无关代码省略）… */
static struct scmi_device *
_scmi_device_create(struct fwnode_handle *fwnode, struct device *parent,
		    int protocol, const char *name)
{
	struct scmi_device *sdev;

	sdev = __scmi_device_create(fwnode, parent, protocol, name);
	if (!sdev)
		pr_err("(%pfwf) Failed to create device - protocol 0x%x (%s)\n",
		       fwnode, protocol, name);

	return sdev;
}

/* Standard protocols table */
static const struct scmi_device_id scmi_std_id_table[] = {
	{ SCMI_PROTOCOL_POWER, "genpd" },
	{ SCMI_PROTOCOL_SYSTEM, "syspower" },
	{ SCMI_PROTOCOL_PERF, "perf" },
	{ SCMI_PROTOCOL_PERF, "cpufreq" },
	{ SCMI_PROTOCOL_CLOCK, "clocks" },
	{ SCMI_PROTOCOL_SENSOR, "hwmon" },
	{ SCMI_PROTOCOL_SENSOR, "iiodev" },
	{ SCMI_PROTOCOL_RESET, "reset" },
	{ SCMI_PROTOCOL_VOLTAGE, "regulator" },
	{ SCMI_PROTOCOL_POWERCAP, "powercap" },
	{ SCMI_PROTOCOL_PINCTRL, "pinctrl" },
	{ SCMI_PROTOCOL_PINCTRL, "pinctrl-imx" },
	{ },
};

static bool scmi_device_id_in_std_id_table(const struct scmi_device_id *id)
{
	for (int i = 0; scmi_std_id_table[i].name[0]; i++) {
		if (scmi_std_id_table[i].protocol_id == id->protocol_id &&
		    !strcmp(scmi_std_id_table[i].name, id->name))
			return true;
	}

	return false;
}

/**
 * scmi_device_create  - A method to create one or more SCMI devices
 *
/* …（同文件无关代码省略）… */
 *
 * Return: The created device (or one of them if @name was NOT provided and
 *	   multiple devices were created) or NULL if no device was created;
 *	   note that NULL indicates an error ONLY in case a specific @name
 *	   was provided: when @name param was not provided, a number of devices
 *	   could have been potentially created for a whole protocol, unless no
 *	   device was found to have been requested for that specific protocol.
 */
struct scmi_device *scmi_device_create(struct fwnode_handle *fwnode,
				       struct device *parent, int protocol,
				       const char *name)
{
	struct list_head *phead;
	struct scmi_requested_dev *rdev;
	struct scmi_device *sdev, *scmi_dev = NULL;

	if (name)
		return _scmi_device_create(fwnode, parent, protocol, name);

	/*
	 * Always create devices for standard protocols, even if the device-ids
	 * have not been registered into scmi_requested_devices yet. This allows
	 * auto-loading of SCMI protocol driver modules for standard protocols.
	 */
	for (int i = 0; scmi_std_id_table[i].name[0]; i++) {
		if (scmi_std_id_table[i].protocol_id != protocol)
			continue;

		sdev = _scmi_device_create(fwnode, parent, protocol,
					   scmi_std_id_table[i].name);
		if (sdev)
			scmi_dev = sdev;
	}

	mutex_lock(&scmi_requested_devices_mtx);
	phead = idr_find(&scmi_requested_devices, protocol);
	/* Nothing to do. */
	if (!phead) {
		mutex_unlock(&scmi_requested_devices_mtx);
		return scmi_dev;
	}

	/* Walk the list of requested devices for protocol and create them */
	list_for_each_entry(rdev, phead, node) {
		/* Standard proto matches already have their dev created above */
		if (scmi_device_id_in_std_id_table(rdev->id_table))
			continue;

		sdev = _scmi_device_create(fwnode, parent,
					   rdev->id_table->protocol_id,
					   rdev->id_table->name);
		if (sdev)
			scmi_dev = sdev;
	}

	mutex_unlock(&scmi_requested_devices_mtx);

	return scmi_dev;
}
