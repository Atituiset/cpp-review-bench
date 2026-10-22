// AUTO-DRAFT from torvalds/linux PR #4df4d2a577db2d3d57b888bcfb1206fc74661d16
{
	const struct software_node *const *swnode;
	struct fwnode_handle *fwnode;
	struct device *phys_dev;  // <<< BUG ANCHOR
	int ret;

	if (!node_group)
/* …（同文件无关代码省略）… */
		if (WARN_ON(!fwnode))
			return -ENOENT;

		phys_dev = acpi_get_first_physical_node(to_acpi_device(dev));
		if (!phys_dev)
			return dev_err_probe(parent, -ENODEV,
					     "No physical device for ACPI GPIO dev: %pfwP\n",
