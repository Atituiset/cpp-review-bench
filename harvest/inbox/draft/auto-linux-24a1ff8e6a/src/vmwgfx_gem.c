// AUTO-DRAFT from torvalds/linux PR #c44104f3642da720164042623e4832836aefdd8e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	struct vmw_ttm_tt *vmw_tt =
		container_of(bo->ttm, struct vmw_ttm_tt, dma_ttm);

	if (vmw_tt->vsgt.sgt)
		return vmw_tt->vsgt.sgt;

	return drm_prime_pages_to_sg(obj->dev, vmw_tt->dma_ttm.pages, vmw_tt->dma_ttm.num_pages);
}

static int vmw_gem_vmap(struct drm_gem_object *obj, struct iosys_map *map)
/* …（同文件无关代码省略）… */
{
	int ret;
	struct vmw_private *dev_priv = vmw_priv(dev);
	struct drm_gem_object *gem = NULL;
	struct vmw_bo *vbo;
	struct vmw_bo_params params = {
		.domain = (dev_priv->has_mob) ? VMW_BO_DOMAIN_SYS : VMW_BO_DOMAIN_VRAM,
/* …（同文件无关代码省略）… */
	dma_resv_lock(params.resv, NULL);

	ret = vmw_bo_create(dev_priv, &params, &vbo);
	if (ret != 0)
		goto out_no_bo;

	vbo->tbo.base.funcs = &vmw_gem_object_funcs;
