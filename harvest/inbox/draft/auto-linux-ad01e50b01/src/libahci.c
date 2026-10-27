// AUTO-DRAFT from torvalds/linux PR #fb2c3f44a7e14627b9dca8e6b568e6cdc234b40b
#include <scsi/scsi_cmnd.h>
#include <linux/libata.h>
#include <linux/pci.h>
#include "ahci.h"
#include "libata.h"
  // <<< BUG ANCHOR
/* …（同文件无关代码省略）… */

	sata_pmp_error_handler(ap);

	if (!ata_dev_enabled(ap->link.device))
		hpriv->stop_engine(ap);
}
/* …（同文件无关代码省略）… */
				0, pp->irq_desc, host->ports[i]);

		if (rc)
			return rc;
		ata_port_desc_misc(host->ports[i], irq);
	}

	return ata_host_register(host, sht);
}

/**
