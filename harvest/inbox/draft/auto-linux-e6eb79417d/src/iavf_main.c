// AUTO-DRAFT from torvalds/linux PR #1e575bda30acc0bf01f799cee798d4a1c100d538
	iounmap(hw->hw_addr);
	pci_release_regions(pdev);
	kfree(adapter->vf_res);
	spin_lock_bh(&adapter->mac_vlan_list_lock);
	/* If we got removed before an up/down sequence, we've got a filter
	 * hanging out there that we need to get rid of.
