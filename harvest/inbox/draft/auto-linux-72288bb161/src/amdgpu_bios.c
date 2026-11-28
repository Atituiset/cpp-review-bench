// AUTO-DRAFT from torvalds/linux PR #c639ac6b9cf100b860408a3cf8d7eab268866cd7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define AMD_IS_VALID_VBIOS(p) ((p)[0] == 0x55 && (p)[1] == 0xAA)
/* …（同文件无关代码省略）… */
static bool check_atom_bios(struct amdgpu_device *adev, size_t size)
{
	uint16_t tmp, bios_header_start;
	uint8_t *bios = adev->bios;

	if (!bios || size < 0x49) {
		dev_dbg(adev->dev, "VBIOS mem is null or mem size is wrong\n");
		return false;
	}

	if (!AMD_IS_VALID_VBIOS(bios)) {
		dev_dbg(adev->dev, "VBIOS signature incorrect %x %x\n", bios[0],
			bios[1]);
		return false;
	}

	bios_header_start = bios[0x48] | (bios[0x49] << 8);
	if (!bios_header_start) {
		dev_dbg(adev->dev, "Can't locate VBIOS header\n");
		return false;
	}

	tmp = bios_header_start + 4;
	if (size < tmp) {
		dev_dbg(adev->dev, "VBIOS header is broken\n");
		return false;
	}

	if (!memcmp(bios + tmp, "ATOM", 4) ||
	    !memcmp(bios + tmp, "MOTA", 4)) {
		dev_dbg(adev->dev, "ATOMBIOS detected\n");
		return true;
	}

	return false;
}
/* …（同文件无关代码省略）… */
void amdgpu_bios_release(struct amdgpu_device *adev)
{
	kfree(adev->bios);
	adev->bios = NULL;
	adev->bios_size = 0;
}
/* …（同文件无关代码省略）… */
	bool found = false;

	/* ATRM is for on-platform devices only */
	if (dev_is_removable(&adev->pdev->dev))
		return false;

	while ((pdev = pci_get_base_class(PCI_BASE_CLASS_DISPLAY, pdev))) {
/* …（同文件无关代码省略）… */
 * VFCT entries contain the PCI bus number as recorded during BIOS POST.
 * On systems where the kernel renumbers PCI buses (e.g. pci=realloc or
 * resource conflicts), the runtime bus number may differ from the POST
 * value.  Match by device identity (vendor + device + function) and use
 * the bus number as a preference: exact bus match is preferred, but when
 * the bus numbers disagree we accept the entry if the device identity
 * matches.
 *
 * Returns: 0 on match, -ENODEV on no match
 */
static int amdgpu_acpi_vfct_match(struct amdgpu_device *adev,
				  VFCT_IMAGE_HEADER *vhdr)
{
	/* Vendor and device IDs must always match */
	if (vhdr->VendorID != adev->pdev->vendor ||
	    vhdr->DeviceID != adev->pdev->device)
		return -ENODEV;

	if (vhdr->PCIDevice != PCI_SLOT(adev->pdev->devfn) ||
	    vhdr->PCIFunction != PCI_FUNC(adev->pdev->devfn))
		return -ENODEV;

	/* Exact bus number match - preferred */
	if (vhdr->PCIBus == adev->pdev->bus->number)
		return 0;

	/* Bus mismatch but device identity matches (PCI renumbering case) */
	dev_notice(adev->dev,
		   "VFCT bus number mismatch: table %u != runtime %u, matching by device identity (vendor 0x%04x device 0x%04x)\n",
		   vhdr->PCIBus, adev->pdev->bus->number,
		   adev->pdev->vendor, adev->pdev->device);
	return 0;
}

static bool amdgpu_acpi_vfct_bios(struct amdgpu_device *adev)
{
	struct acpi_table_header *hdr;
	acpi_size tbl_size;
	UEFI_ACPI_VFCT *vfct;
	unsigned int offset;
	bool r = false;

	if (!ACPI_SUCCESS(acpi_get_table("VFCT", 1, &hdr)))
		return false;
	tbl_size = hdr->length;
	if (tbl_size < sizeof(UEFI_ACPI_VFCT)) {
		dev_info(adev->dev, "ACPI VFCT table present but broken (too short #1),skipping\n");
		goto out;
	}

	vfct = (UEFI_ACPI_VFCT *)hdr;
	offset = vfct->VBIOSImageOffset;

	while (offset < tbl_size) {
		GOP_VBIOS_CONTENT *vbios = (GOP_VBIOS_CONTENT *)((char *)hdr + offset);
		VFCT_IMAGE_HEADER *vhdr = &vbios->VbiosHeader;

		offset += sizeof(VFCT_IMAGE_HEADER);
		if (offset > tbl_size) {
			dev_info(adev->dev, "ACPI VFCT image header truncated,skipping\n");
			goto out;
		}

		offset += vhdr->ImageLength;
		if (offset > tbl_size) {
			dev_info(adev->dev, "ACPI VFCT image truncated,skipping\n");
			goto out;
		}

		if (vhdr->ImageLength &&
		    !amdgpu_acpi_vfct_match(adev, vhdr)) {
			adev->bios = kmemdup(&vbios->VbiosContent,
					     vhdr->ImageLength,
					     GFP_KERNEL);

			if (!check_atom_bios(adev, vhdr->ImageLength)) {
				amdgpu_bios_release(adev);
				goto out;
			}
			adev->bios_size = vhdr->ImageLength;
			r = true;
			goto out;
		}
	}

	dev_info(adev->dev, "ACPI VFCT table present but broken (too short #2),skipping\n");

out:
	acpi_put_table(hdr);
	return r;
}
