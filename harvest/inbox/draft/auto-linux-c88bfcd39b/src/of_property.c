// AUTO-DRAFT from torvalds/linux PR #fddfc3ec31799a932bb92f1b8a84cb3d1f963be9
				 struct of_changeset *ocs,
				 struct device_node *np)
{  // <<< BUG ANCHOR
	u32 bus_range[] = { pdev->subordinate->busn_res.start,
			    pdev->subordinate->busn_res.end };

	return of_changeset_add_prop_u32_array(ocs, np, "bus-range", bus_range,
					       ARRAY_SIZE(bus_range));
}
/* …（同文件无关代码省略）… */
	int ret;
	u8 pin;

	pnode = pci_device_to_OF_node(pdev->bus->self);
	if (!pnode)
		pnode = pci_bus_to_OF_node(pdev->bus);
