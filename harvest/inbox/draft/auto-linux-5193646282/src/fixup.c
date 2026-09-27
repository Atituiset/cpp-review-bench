// AUTO-DRAFT from torvalds/linux PR #fddfc3ec31799a932bb92f1b8a84cb3d1f963be9
#define AMD_15B8_RCC_DEV2_EPF0_STRAP2                                  0x10136008
#define AMD_15B8_RCC_DEV2_EPF0_STRAP2_NO_SOFT_RESET_DEV2_F0_MASK       0x00000080L
/* …（同文件无关代码省略）… */
static void quirk_clear_strap_no_soft_reset_dev2_f0(struct pci_dev *dev)
{
	u32 data;

	if (!amd_smn_read(0, AMD_15B8_RCC_DEV2_EPF0_STRAP2, &data)) {
		data &= ~AMD_15B8_RCC_DEV2_EPF0_STRAP2_NO_SOFT_RESET_DEV2_F0_MASK;
		if (amd_smn_write(0, AMD_15B8_RCC_DEV2_EPF0_STRAP2, data))
			pci_err(dev, "Failed to write data 0x%x\n", data);
	} else {
		pci_err(dev, "Failed to read data\n");
	}
}
DECLARE_PCI_FIXUP_FINAL(PCI_VENDOR_ID_AMD, 0x15b8, quirk_clear_strap_no_soft_reset_dev2_f0);
#endif

/*
