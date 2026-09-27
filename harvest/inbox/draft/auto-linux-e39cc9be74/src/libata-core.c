// AUTO-DRAFT from torvalds/linux PR #fd179f8a05be3ccae366b9b96e176b51fbe54aab
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static bool ata_dev_check_adapter(struct ata_device *dev,
				  unsigned short vendor_id)
{
	struct pci_dev *pcidev = NULL;
	struct device *parent_dev = NULL;

	for (parent_dev = dev->tdev.parent; parent_dev != NULL;
	     parent_dev = parent_dev->parent) {
		if (dev_is_pci(parent_dev)) {
			pcidev = to_pci_dev(parent_dev);
			if (pcidev->vendor == vendor_id)
				return true;
			break;
		}
	}

	return false;
}
/* …（同文件无关代码省略）… */
		    (dev->id[ATA_ID_SATA_CAPABILITY] & 0xe) == 0x2)
			dev->quirks |= ATA_QUIRK_NOLPM;

		/* ATI specific quirk */
		if ((dev->quirks & ATA_QUIRK_NO_LPM_ON_ATI) &&
		    ata_dev_check_adapter(dev, PCI_VENDOR_ID_ATI))
			dev->quirks |= ATA_QUIRK_NOLPM;
	}

/* …（同文件无关代码省略）… */
	[__ATA_QUIRK_MAX_SEC]		= "maxsec",
	[__ATA_QUIRK_MAX_TRIM_128M]	= "maxtrim128m",
	[__ATA_QUIRK_NO_NCQ_ON_ATI]	= "noncqonati",
	[__ATA_QUIRK_NO_LPM_ON_ATI]	= "nolpmonati",
	[__ATA_QUIRK_NO_ID_DEV_LOG]	= "noiddevlog",
	[__ATA_QUIRK_NO_LOG_DIR]	= "nologdir",
	[__ATA_QUIRK_NO_FUA]		= "nofua",
/* …（同文件无关代码省略）… */
	{ "Samsung SSD 860*",		NULL,	ATA_QUIRK_NO_NCQ_TRIM |
						ATA_QUIRK_ZERO_AFTER_TRIM |
						ATA_QUIRK_NO_NCQ_ON_ATI |
						ATA_QUIRK_NO_LPM_ON_ATI },
	{ "Samsung SSD 870*",		NULL,	ATA_QUIRK_NO_NCQ_TRIM |
						ATA_QUIRK_ZERO_AFTER_TRIM |
						ATA_QUIRK_NO_NCQ_ON_ATI |
						ATA_QUIRK_NO_LPM_ON_ATI },
	{ "SAMSUNG*MZ7LH*",		NULL,	ATA_QUIRK_NO_NCQ_TRIM |
						ATA_QUIRK_ZERO_AFTER_TRIM |
						ATA_QUIRK_NO_NCQ_ON_ATI |
						ATA_QUIRK_NO_LPM_ON_ATI },
	{ "FCCT*M500*",			NULL,	ATA_QUIRK_NO_NCQ_TRIM |
						ATA_QUIRK_ZERO_AFTER_TRIM },
