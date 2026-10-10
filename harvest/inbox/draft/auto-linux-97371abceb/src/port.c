// AUTO-DRAFT from torvalds/linux PR #af32da41b0327b9c6a37856ba82b6760d6c8d10e
  // <<< BUG ANCHOR
	xa_erase(&pf->sf_nums, devlink_port->attrs.pci_sf.sf);
	ice_eswitch_detach_sf(pf, dyn_port);
	ice_vsi_free(dyn_port->vsi);
	xa_erase(&pf->dyn_ports, dyn_port->vsi->idx);
	kfree(dyn_port);
}
