// AUTO-DRAFT from torvalds/linux PR #aa31d5f8b151b786b748c6bfc9a8c4f1855c8adc
static void quirk_no_imm_ready(struct pci_dev *dev)
{
	if (!dev->imm_ready)
		return;

	pci_info(dev, "Ignoring incorrectly advertised Immediate Readiness support\n");
	dev->imm_ready = 0;
}
DECLARE_PCI_FIXUP_FINAL(PCI_VENDOR_ID_SAMSUNG, 0xa80c, quirk_no_imm_ready);
DECLARE_PCI_FIXUP_FINAL(PCI_VENDOR_ID_SAMSUNG, 0xa80d, quirk_no_imm_ready);

static void quirk_no_ext_tags(struct pci_dev *pdev)
{
