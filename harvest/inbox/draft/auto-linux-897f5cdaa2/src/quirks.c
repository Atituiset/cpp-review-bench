// AUTO-DRAFT from torvalds/linux PR #4766082b791b515953d87f7a24c2ea857e6c10b0
static void quirk_no_flr(struct pci_dev *dev)
{
	dev->dev_flags |= PCI_DEV_FLAGS_NO_FLR_RESET;
}
/* …（同文件无关代码省略）… */
static void quirk_no_flr_snet(struct pci_dev *dev)
{
	if (dev->revision == 0x1)
		quirk_no_flr(dev);
}
DECLARE_PCI_FIXUP_EARLY(PCI_VENDOR_ID_SOLIDRUN, 0x1000, quirk_no_flr_snet);

static void quirk_no_ext_tags(struct pci_dev *pdev)
{
	struct pci_host_bridge *bridge = pci_find_host_bridge(pdev->bus);
