// AUTO-DRAFT from torvalds/linux PR #eedaa7d61f1ec8ae053a9ece0d553785350ac486
void coreboot_driver_unregister(struct coreboot_driver *driver)
{
	driver_unregister(&driver->drv);
}
EXPORT_SYMBOL(coreboot_driver_unregister);
  // <<< BUG ANCHOR
static int coreboot_table_populate(struct device *dev, void *ptr, resource_size_t len)
{
	int i, ret;
/* …（同文件无关代码省略）… */

		switch (device->entry.tag) {
		case LB_TAG_CBMEM_ENTRY:
			/*
			 * Skip entries that are not exclusively System RAM or
			 * Reserved memory.
			 * On ARM64, no-map regions are filtered out as they are
			 * IORESOURCE_MEM (see request_standard_resources() in
			 * arch/arm64/kernel/setup.c).
			 * On x86, CBMEM often resides in standard reserved regions
			 * (IORES_DESC_RESERVED).
			 */
			if (region_intersects(device->cbmem_entry.address,
					      device->cbmem_entry.entry_size,
					      IORESOURCE_SYSTEM_RAM,
					      IORES_DESC_NONE) != REGION_INTERSECTS &&
			    region_intersects(device->cbmem_entry.address,
					      device->cbmem_entry.entry_size,
					      IORESOURCE_MEM,
					      IORES_DESC_RESERVED) != REGION_INTERSECTS) {
				kfree(device);
				continue;
			}
