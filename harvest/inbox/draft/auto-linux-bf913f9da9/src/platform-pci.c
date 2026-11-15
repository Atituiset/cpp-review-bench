// AUTO-DRAFT from torvalds/linux PR #23c62304dde29abe92013d85910d03a77dada35d
#define DRV_NAME    "xen-platform-pci"
/* …（同文件无关代码省略）… */
			      const struct pci_device_id *ent)
{
	int i, ret;
	long ioaddr;
	long mmio_addr, mmio_len;
	unsigned int max_nr_gframes;
/* …（同文件无关代码省略）… */
	if (mmio_addr == 0 || ioaddr == 0) {
		dev_err(&pdev->dev, "no resources found\n");
		ret = -ENOENT;
		goto pci_out;  // <<< BUG ANCHOR
	}

	ret = pci_request_region(pdev, 1, DRV_NAME);
	if (ret < 0)
		goto pci_out;

	ret = pci_request_region(pdev, 0, DRV_NAME);
	if (ret < 0)
		goto mem_out;

	platform_mmio = mmio_addr;
	platform_mmiolen = mmio_len;
/* …（同文件无关代码省略）… */
	if (!xen_have_vector_callback)
		free_irq(pdev->irq, pdev);
out:
	pci_release_region(pdev, 0);
mem_out:
	pci_release_region(pdev, 1);
pci_out:
	pci_disable_device(pdev);
	return ret;
}
