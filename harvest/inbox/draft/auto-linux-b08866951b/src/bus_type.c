// AUTO-DRAFT from torvalds/linux PR #1cb6cfab0916d18f88d44e4a23806b6c4cb355f8
	return 0;
}
  // <<< BUG ANCHOR
static int sdw_bus_probe(struct device *dev)
{
	struct sdw_slave *slave = dev_to_sdw_dev(dev);
/* …（同文件无关代码省略）… */
		return ret;
	}
	slave->index = ret;

	/* Create IRQ mapping now so the driver can get it in probe() */
	sdw_irq_create_mapping(slave);

	ret = drv->probe(slave, id);
	if (ret) {
		ida_free(&slave->bus->slave_ida, slave->index);
		return ret;
	}

	mutex_lock(&slave->sdw_dev_lock);

/* …（同文件无关代码省略）… */

	if (drv->remove)
		drv->remove(slave);

	ida_free(&slave->bus->slave_ida, slave->index);
}

static void sdw_bus_shutdown(struct device *dev)
{
	struct sdw_slave *slave = dev_to_sdw_dev(dev);
	struct sdw_driver *drv = drv_to_sdw_driver(dev->driver);

	if (dev->driver && drv->shutdown)
		drv->shutdown(slave);
}
