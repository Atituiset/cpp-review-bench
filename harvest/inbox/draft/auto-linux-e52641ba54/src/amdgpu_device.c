// AUTO-DRAFT from torvalds/linux PR #4da9dedf3fc59137fd86d664693776aa38cb97f5
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

#include <linux/console.h>
#include <linux/slab.h>
#include <linux/iommu.h>
#include <linux/pci.h>
#include <linux/pci-p2pdma.h>
#include <linux/apple-gmux.h>
/* …（同文件无关代码省略）… */
		       apple_gmux_detect(NULL, NULL)));
}

/**
 * amdgpu_device_init - initialize the driver
 *
/* …（同文件无关代码省略）… */
	if (r)
		return r;

	/*
	 * No need to remove conflicting FBs for non-display class devices.
	 * This prevents the sysfb from being freed accidently.
/* …（同文件无关代码省略）… */
static void amdgpu_device_unmap_mmio(struct amdgpu_device *adev)
{

	/* Clear all CPU mappings pointing to this device */
	unmap_mapping_range(adev->ddev.anon_inode->i_mapping, 0, 0, 1);

	/* Unmap all mapped bars - Doorbell, registers and VRAM */
	amdgpu_doorbell_fini(adev);

	iounmap(adev->rmmio);
	adev->rmmio = NULL;
	if (adev->mman.aper_base_kaddr)
		iounmap(adev->mman.aper_base_kaddr);
	adev->mman.aper_base_kaddr = NULL;

	/* Memory manager related */
	if (!adev->gmc.xgmi.connected_to_cpu && !adev->gmc.is_app_apu) {
		arch_phys_wc_del(adev->gmc.vram_mtrr);
		arch_io_free_memtype_wc(adev->gmc.aper_base, adev->gmc.aper_size);
	}
}
/* …（同文件无关代码省略）… */

	amdgpu_gart_dummy_page_fini(adev);

	if (pci_dev_is_disconnected(adev->pdev))
		amdgpu_device_unmap_mmio(adev);

/* …（同文件无关代码省略）… */
	if (dev->switch_power_state == DRM_SWITCH_POWER_OFF)
		return 0;

	if (adev->in_s0ix)
		amdgpu_dpm_gfx_state_change(adev, sGpuChangeState_D0Entry);
