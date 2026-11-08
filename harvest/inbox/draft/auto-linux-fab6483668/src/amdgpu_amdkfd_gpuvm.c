// AUTO-DRAFT from torvalds/linux PR #543b7d2ca09b4b08d518d668ac2fdc361f231785
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
static int amdgpu_amdkfd_remove_eviction_fence(struct amdgpu_bo *bo,
					struct amdgpu_amdkfd_fence *ef)
{
	struct dma_fence *replacement;

	if (!ef)
		return -EINVAL;

	/* TODO: Instead of block before we should use the fence of the page
	 * table update and TLB flush here directly.
	 */
	replacement = dma_fence_get_stub();
	dma_resv_replace_fences(bo->tbo.base.resv, ef->base.context,
				replacement, DMA_RESV_USAGE_BOOKKEEP);
	dma_fence_put(replacement);
	return 0;
}
/* …（同文件无关代码省略）… */
			ret = amdgpu_vm_lock_pd(vm, &exec, 0);
			drm_exec_retry_on_contention(&exec);
			if (unlikely(ret))
				goto unwind;
			ret = drm_exec_lock_obj(&exec, &bo[i]->tbo.base);
			drm_exec_retry_on_contention(&exec);
			if (unlikely(ret))
				goto unwind;
		}

		bo_va = amdgpu_vm_bo_find(vm, bo[i]);
/* …（同文件无关代码省略）… */

	return 0;

unwind:
	for (; i >= 0; i--) {
		if (!attachment[i])
/* …（同文件无关代码省略）… */
static void remove_kgd_mem_from_kfd_bo_list(struct kgd_mem *mem,
		struct amdkfd_process_info *process_info)
{
	mutex_lock(&process_info->lock);
	list_del(&mem->validate_list);
	mutex_unlock(&process_info->lock);
}
/* …（同文件无关代码省略）… */
 * amdgpu_amdkfd_map_gtt_bo_to_gart - Map BO to GART and increment reference count
 * @bo: Buffer object to be mapped
 * @bo_gart: Return bo reference
 *
 * Before return, bo reference count is incremented. To release the reference and unpin/
 * unmap the BO, call amdgpu_amdkfd_free_kernel_mem.
 */
int amdgpu_amdkfd_map_gtt_bo_to_gart(struct amdgpu_bo *bo, struct amdgpu_bo **bo_gart)
{
	int ret;

	ret = amdgpu_bo_reserve(bo, true);
	if (ret) {
		pr_err("Failed to reserve bo. ret %d\n", ret);
		goto err_reserve_bo_failed;
	}

	ret = amdgpu_bo_pin(bo, AMDGPU_GEM_DOMAIN_GTT);
	if (ret) {
		pr_err("Failed to pin bo. ret %d\n", ret);
		goto err_pin_bo_failed;
	}

	ret = amdgpu_ttm_alloc_gart(&bo->tbo);
	if (ret) {
		pr_err("Failed to bind bo to GART. ret %d\n", ret);
		goto err_map_bo_gart_failed;
	}

	amdgpu_amdkfd_remove_eviction_fence(
		bo, bo->vm_bo->vm->process_info->eviction_fence);

	amdgpu_bo_unreserve(bo);

	*bo_gart = amdgpu_bo_ref(bo);

	return 0;

err_map_bo_gart_failed:
	amdgpu_bo_unpin(bo);
err_pin_bo_failed:
	amdgpu_bo_unreserve(bo);
err_reserve_bo_failed:

	return ret;
}
/* …（同文件无关代码省略）… */

err_remove_mem:
	remove_kgd_mem_from_kfd_bo_list(*mem, avm->process_info);
	drm_vma_node_revoke(&obj->vma_node, drm_priv);
err_free_mem:
	kfree(*mem);
