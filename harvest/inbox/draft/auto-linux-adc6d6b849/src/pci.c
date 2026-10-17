// AUTO-DRAFT from torvalds/linux PR #4766082b791b515953d87f7a24c2ea857e6c10b0
	 * initialize.
	 *
	 * If the device is below a Root Port with Configuration RRS
	 * Software Visibility enabled, reading the Vendor ID returns a
	 * special data value if the device responded with RRS.  Read the
	 * Vendor ID until we get non-RRS status.
	 *
	 * If there's no Root Port or Configuration RRS Software Visibility
	 * is not enabled, the device may still respond with RRS, but
/* …（同文件无关代码省略）… */
  // <<< BUG ANCHOR
		if (root && root->config_rrs_sv) {
			pci_read_config_dword(dev, PCI_VENDOR_ID, &id);
			if (!pci_bus_rrs_vendor_id(id))
				break;
		} else {
			pci_read_config_dword(dev, PCI_COMMAND, &id);
			if (!PCI_POSSIBLE_ERROR(id))
