// AUTO-DRAFT from torvalds/linux PR #bb934c29fda8dcd409145a0deddc1a8095fec930
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdint.h>
  // <<< BUG ANCHOR

	*pos &= (1UL << 22) - 1;

	r = pm_runtime_get_sync(adev_to_drm(adev)->dev);
	if (r < 0) {
		pm_runtime_put_autosuspend(adev_to_drm(adev)->dev);
/* …（同文件无关代码省略）… */
		mutex_lock(&adev->pm.mutex);

	while (size) {
		uint32_t value;

		if (read) {
			value = RREG32(*pos >> 2);
			r = put_user(value, (uint32_t *)buf);
		} else {
			r = get_user(value, (uint32_t *)buf);
			if (!r)
				amdgpu_mm_wreg_mmio_rlc(adev, *pos >> 2, value, 0);
		}
		if (r) {
			result = r;
			goto end;
		}

		result += 4;
		buf += 4;
		*pos += 4;
		size -= 4;
	}

end:
	if (use_bank) {
		amdgpu_gfx_select_se_sh(adev, 0xffffffff, 0xffffffff, 0xffffffff, 0);
		mutex_unlock(&adev->grbm_idx_mutex);
/* …（同文件无关代码省略）… */
	pm_runtime_put_autosuspend(adev_to_drm(adev)->dev);

	amdgpu_virt_disable_access_debugfs(adev);
	return result;
}

/* …（同文件无关代码省略）… */
	struct amdgpu_device *adev = rd->adev;
	ssize_t result = 0;
	int r;
	uint32_t value;

	if (size & 0x3 || offset & 0x3)
		return -EINVAL;

	r = pm_runtime_get_sync(adev_to_drm(adev)->dev);
	if (r < 0) {
		pm_runtime_put_autosuspend(adev_to_drm(adev)->dev);
/* …（同文件无关代码省略）… */
		mutex_lock(&adev->pm.mutex);

	while (size) {
		if (!write_en) {
			value = RREG32(offset >> 2);
			r = put_user(value, (uint32_t *)buf);
		} else {
			r = get_user(value, (uint32_t *)buf);
			if (!r)
				amdgpu_mm_wreg_mmio_rlc(adev, offset >> 2, value, rd->id.xcc_id);
		}
		if (r) {
			result = r;
			goto end;
		}
		offset += 4;
		size -= 4;
		result += 4;
		buf += 4;
	}
end:
	if (rd->id.use_grbm) {
		amdgpu_gfx_select_se_sh(adev, 0xffffffff, 0xffffffff, 0xffffffff, rd->id.xcc_id);
		mutex_unlock(&adev->grbm_idx_mutex);
/* …（同文件无关代码省略）… */
	pm_runtime_put_autosuspend(adev_to_drm(adev)->dev);

	amdgpu_virt_disable_access_debugfs(adev);
	return result;
}
