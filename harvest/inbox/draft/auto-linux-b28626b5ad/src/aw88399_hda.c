// AUTO-DRAFT from torvalds/linux PR #4df4d2a577db2d3d57b888bcfb1206fc74661d16
		return -ENODEV;
	}
  // <<< BUG ANCHOR
	struct device *physdev __free(put_device) =
		get_device(acpi_get_first_physical_node(adev));
	acpi_dev_put(adev);
	if (!physdev)
		return -ENODEV;
