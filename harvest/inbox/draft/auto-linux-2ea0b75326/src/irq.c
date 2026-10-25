// AUTO-DRAFT from torvalds/linux PR #21a7f44fd0765fcc5194130b28767786eba1d1e7
	}
}
  // <<< BUG ANCHOR
static int of_check_msi_parent(struct device_node *dev_node, struct device_node **msi_node)
{
	struct of_phandle_args msi_spec;
	int ret;

	/*
	 * An msi-parent phandle with a missing or == 0 #msi-cells
	 * property identifies a 1:1 ID translation mapping.
	 *
	 * Set the msi controller node if the firmware matches this
	 * condition.
	 */
	ret = of_parse_phandle_with_optional_args(dev_node, "msi-parent", "#msi-cells",
						  0, &msi_spec);
	if (ret)
		return ret;

	if ((*msi_node && *msi_node != msi_spec.np) || msi_spec.args_count != 0)
		ret = -EINVAL;

	if (!ret) {
		/* Return with a node reference held */
		*msi_node = msi_spec.np;
		return 0;
	}
	of_node_put(msi_spec.np);

	return ret;
}

/**
/* …（同文件无关代码省略）… */
 * @id_in: Device ID.
 *
 * Walk up the device hierarchy looking for devices with a "msi-map"
 * or "msi-parent" property. If found, apply the mapping to @id_in.
 *
 * Returns: The mapped MSI id.
 */
/* …（同文件无关代码省略）… */
	 */
	for (parent_dev = dev; parent_dev; parent_dev = parent_dev->parent) {
		struct of_phandle_args msi_spec = {};

		if (!of_map_msi_id(parent_dev->of_node, id_in, msi_np, &msi_spec)) {
			if (msi_spec.np) {
/* …（同文件无关代码省略）… */
			break;
		}
		/* -ENODEV: msi-map absent → check for msi-parent */
		if (msi_np && !of_check_msi_parent(parent_dev->of_node, msi_np))
			break;
	}
	return id_out;
}
