// AUTO-DRAFT from torvalds/linux PR #62f482ff73bf3ae88b0c899115527752cf9cbf13
	struct amd_iommu *iommu;
	int devid = -1;

	if (x86_fwspec_is_ioapic(fwspec))
		devid = get_ioapic_devid(fwspec->param[0]);
	else if (x86_fwspec_is_hpet(fwspec))
