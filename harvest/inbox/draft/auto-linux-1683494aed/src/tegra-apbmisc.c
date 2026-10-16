// AUTO-DRAFT from torvalds/linux PR #2f2f7eeef8b0a1a9b9ca998231419873ccae8d4e
  // <<< BUG ANCHOR
	ret = acpi_dev_get_memory_resources(adev, &resource_list);
	if (ret < 0) {
		pr_err("failed to get APBMISC memory resources");
		goto out_put_acpi_dev;
	}
