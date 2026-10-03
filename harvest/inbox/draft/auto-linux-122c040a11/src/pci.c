// AUTO-DRAFT from torvalds/linux PR #d2dbe503fd806082acb0ca79a9d6641822988c2c
	}
  // <<< BUG ANCHOR
	root = pcie_find_root_port(dev);
	if (!root)
		return -EINVAL;

	pcie_capability_read_dword(root, PCI_EXP_DEVCAP2, &cap);
	if ((cap & cap_mask) != cap_mask)
