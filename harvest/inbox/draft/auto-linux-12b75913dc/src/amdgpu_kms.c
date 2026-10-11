// AUTO-DRAFT from torvalds/linux PR #492c4ded0ff7c7f162f35eea37984db5b0f2b5ba
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

#include <drm/amdgpu_drm.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h>
#include "amdgpu_uvd.h"
#include "amdgpu_vce.h"
#include "atom.h"
/* …（同文件无关代码省略）… */
		DRM_ERROR("Invalid crtc %u\n", pipe);
		return -EINVAL;
	}

	/* The hw increments its frame counter at start of vsync, not at start
	 * of vblank, as is required by DRM core vblank counter handling.
/* …（同文件无关代码省略）… */
		DRM_DEBUG_VBL("NULL mode info! Returned count may be wrong.\n");
	}

	return count;
}
