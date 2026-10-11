// AUTO-DRAFT from torvalds/linux PR #bb934c29fda8dcd409145a0deddc1a8095fec930
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
	case CHIP_RENOIR:
		adev->mman.keep_stolen_vga_memory = true;
		break;
	case CHIP_POLARIS10:
	case CHIP_POLARIS11:
	case CHIP_POLARIS12:
		/* MacBookPros with switchable graphics put VRAM at 0 when
		 * the iGPU is enabled which results in cursor issues if
		 * the cursor ends up at 0.  Reserve vram at 0 in that case.
		 */
		if (adev->gmc.vram_start == 0)
			adev->mman.keep_stolen_vga_memory = true;
		break;
	default:
		adev->mman.keep_stolen_vga_memory = false;
		break;
	}

	if (amdgpu_sriov_vf(adev) ||
	    !amdgpu_device_has_display_hardware(adev)) {
