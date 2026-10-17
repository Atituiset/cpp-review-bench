// AUTO-DRAFT from torvalds/linux PR #f91a4ac69629e1479745e82100863157e6967ec0
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
static void trace_amdgpu_cs_ibs(struct amdgpu_cs_parser *p)
{
	int i, j;

	if (!trace_amdgpu_cs_enabled())
		return;

	for (i = 0; i < p->gang_size; ++i) {
		struct amdgpu_job *job = p->jobs[i];

		for (j = 0; j < job->num_ibs; ++j)
			trace_amdgpu_cs(p, job, &job->ibs[j]);
	}
}
/* …（同文件无关代码省略）… */
static int amdgpu_cs_patch_ibs(struct amdgpu_cs_parser *p,
			       struct amdgpu_job *job)
{
	struct amdgpu_ring *ring = amdgpu_job_ring(job);
	struct amdgpu_device *adev = ring->adev;
	unsigned int i;
	int r;

	/* Only for UVD/VCE VM emulation */
	if (!ring->funcs->parse_cs && !ring->funcs->patch_cs_in_place)
		return 0;

	for (i = 0; i < job->num_ibs; ++i) {
		struct amdgpu_ib *ib = &job->ibs[i];
		struct amdgpu_bo_va_mapping *m;
		struct amdgpu_bo *aobj;
		uint64_t va_start;
		uint8_t *kptr;

		va_start = ib->gpu_addr & AMDGPU_GMC_HOLE_MASK;
		r = amdgpu_cs_find_mapping(p, va_start, &aobj, &m);
		if (r) {
			drm_err(adev_to_drm(p->adev), "IB va_start is invalid\n");
			return r;
		}

		if ((va_start + ib->length_dw * 4) >
		    (m->last + 1) * AMDGPU_GPU_PAGE_SIZE) {
			drm_err(adev_to_drm(p->adev), "IB va_start+ib_bytes is invalid\n");
			return -EINVAL;
		}

		/* the IB should be reserved at this point */
		r = amdgpu_bo_kmap(aobj, (void **)&kptr);
		if (r)
			return r;

		kptr += va_start - (m->start * AMDGPU_GPU_PAGE_SIZE);

		if (ring->funcs->parse_cs) {
			memcpy(ib->ptr, kptr, ib->length_dw * 4);
			amdgpu_bo_kunmap(aobj);

			r = amdgpu_ring_parse_cs(ring, p, job, ib);
			if (r)
				return r;

			if (ib->sa_bo)
				ib->gpu_addr =  amdgpu_sa_bo_gpu_addr(ib->sa_bo);
		} else {
			ib->ptr = (uint32_t *)kptr;
			r = amdgpu_ring_patch_cs_in_place(ring, p, job, ib);
			amdgpu_bo_kunmap(aobj);
			if (r)
				return r;
		}
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static int amdgpu_cs_patch_jobs(struct amdgpu_cs_parser *p)
{
	unsigned int i;
	int r;

	for (i = 0; i < p->gang_size; ++i) {
		r = amdgpu_cs_patch_ibs(p, p->jobs[i]);
		if (r)
			return r;
	}
	return 0;
}
/* …（同文件无关代码省略）… */
static int amdgpu_cs_vm_handling(struct amdgpu_cs_parser *p)
{
	struct amdgpu_fpriv *fpriv = p->filp->driver_priv;
	struct amdgpu_job *job = p->gang_leader;
	struct amdgpu_device *adev = p->adev;
	struct amdgpu_vm *vm = &fpriv->vm;
	struct amdgpu_bo_list_entry *e;
	struct amdgpu_bo_va *bo_va;
	unsigned int i;
	int r;

	/*
	 * We can't use gang submit on with reserved VMIDs when the VM changes
	 * can't be invalidated by more than one engine at the same time.
	 */
	if (p->gang_size > 1 && !adev->vm_manager.concurrent_flush) {
		for (i = 0; i < p->gang_size; ++i) {
			struct drm_sched_entity *entity = p->entities[i];
			struct drm_gpu_scheduler *sched = entity->rq->sched;
			struct amdgpu_ring *ring = to_amdgpu_ring(sched);

			if (amdgpu_vmid_uses_reserved(vm, ring->vm_hub))
				return -EINVAL;
		}
	}

	if (!amdgpu_vm_ready(vm))
		return -EINVAL;

	r = amdgpu_vm_clear_freed(adev, vm, NULL);
	if (r)
		return r;

	r = amdgpu_vm_bo_update(adev, fpriv->prt_va, false);
	if (r)
		return r;

	r = amdgpu_sync_fence(&p->sync, fpriv->prt_va->last_pt_update,
			      GFP_KERNEL);
	if (r)
		return r;

	if (fpriv->csa_va) {
		bo_va = fpriv->csa_va;
		if (!bo_va)
			return -ENOMEM;
		r = amdgpu_vm_bo_update(adev, bo_va, false);
		if (r)
			return r;

		r = amdgpu_sync_fence(&p->sync, bo_va->last_pt_update,
				      GFP_KERNEL);
		if (r)
			return r;
	}

	/* FIXME: In theory this loop shouldn't be needed any more when
	 * amdgpu_vm_handle_moved handles all moved BOs that are reserved
	 * with p->ticket. But removing it caused test regressions, so I'm
	 * leaving it here for now.
	 */
	amdgpu_bo_list_for_each_entry(e, p->bo_list) {
		bo_va = e->bo_va;
		if (bo_va == NULL)
			continue;

		r = amdgpu_vm_bo_update(adev, bo_va, false);
		if (r)
			return r;

		r = amdgpu_sync_fence(&p->sync, bo_va->last_pt_update,
				      GFP_KERNEL);
		if (r)
			return r;
	}

	r = amdgpu_vm_handle_moved(adev, vm, drm_exec_ticket(&p->exec));
	if (r)
		return r;

	r = amdgpu_vm_update_pdes(adev, vm, false);
	if (r)
		return r;

	r = amdgpu_sync_fence(&p->sync, vm->last_update, GFP_KERNEL);
	if (r)
		return r;

	for (i = 0; i < p->gang_size; ++i) {
		job = p->jobs[i];

		if (!job->vm)
			continue;

		job->vm_pd_addr = amdgpu_gmc_pd_addr(vm->root.bo);
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static int amdgpu_cs_sync_rings(struct amdgpu_cs_parser *p)
{
	struct amdgpu_fpriv *fpriv = p->filp->driver_priv;
	struct drm_gpu_scheduler *sched;
	struct drm_gem_object *obj;
	struct dma_fence *fence;
	unsigned int i;
	int r;

	r = amdgpu_ctx_wait_prev_fence(p->ctx, p->entities[p->gang_leader_idx]);
	if (r) {
		if (r != -ERESTARTSYS)
			drm_err(adev_to_drm(p->adev), "amdgpu_ctx_wait_prev_fence failed.\n");
		return r;
	}

	drm_exec_for_each_locked_object(&p->exec, obj) {
		struct amdgpu_bo *bo = gem_to_amdgpu_bo(obj);

		struct dma_resv *resv = bo->tbo.base.resv;
		enum amdgpu_sync_mode sync_mode;

		sync_mode = amdgpu_bo_explicit_sync(bo) ?
			AMDGPU_SYNC_EXPLICIT : AMDGPU_SYNC_NE_OWNER;
		r = amdgpu_sync_resv(p->adev, &p->sync, resv, sync_mode,
				     &fpriv->vm);
		if (r)
			return r;
	}

	for (i = 0; i < p->gang_size; ++i) {
		r = amdgpu_sync_push_to_job(&p->sync, p->jobs[i]);
		if (r)
			return r;
	}

	sched = p->gang_leader->base.entity->rq->sched;
	while ((fence = amdgpu_sync_get_fence(&p->sync))) {
		struct drm_sched_fence *s_fence = to_drm_sched_fence(fence);

		/*
		 * When we have an dependency it might be necessary to insert a
		 * pipeline sync to make sure that all caches etc are flushed and the
		 * next job actually sees the results from the previous one
		 * before we start executing on the same scheduler ring.
		 */
		if (!s_fence || s_fence->sched != sched) {
			dma_fence_put(fence);
			continue;
		}

		r = amdgpu_sync_fence(&p->gang_leader->explicit_sync, fence,
				      GFP_KERNEL);
		dma_fence_put(fence);
		if (r)
			return r;
	}
	return 0;
}
/* …（同文件无关代码省略）… */
static void amdgpu_cs_post_dependencies(struct amdgpu_cs_parser *p)
{
	int i;

	for (i = 0; i < p->num_post_deps; ++i) {
		if (p->post_deps[i].chain && p->post_deps[i].point) {
			drm_syncobj_add_point(p->post_deps[i].syncobj,
					      p->post_deps[i].chain,
					      p->fence, p->post_deps[i].point);
			p->post_deps[i].chain = NULL;
		} else {
			drm_syncobj_replace_fence(p->post_deps[i].syncobj,
						  p->fence);
		}
	}
}
/* …（同文件无关代码省略）… */
static int amdgpu_cs_submit(struct amdgpu_cs_parser *p,
			    union drm_amdgpu_cs *cs)
{
	struct amdgpu_fpriv *fpriv = p->filp->driver_priv;
	struct amdgpu_job *leader = p->gang_leader;
	struct amdgpu_vm *vm = &fpriv->vm;
	struct amdgpu_bo_list_entry *e;
	struct drm_gem_object *gobj;
	unsigned int i;
	uint64_t seq;
	int r;

	for (i = 0; i < p->gang_size; ++i)
		drm_sched_job_arm(&p->jobs[i]->base);

	for (i = 0; i < p->gang_size; ++i) {
		struct dma_fence *fence;

		if (p->jobs[i] == leader)
			continue;

		fence = &p->jobs[i]->base.s_fence->scheduled;
		dma_fence_get(fence);
		r = drm_sched_job_add_dependency(&leader->base, fence);
		if (r) {
			dma_fence_put(fence);
			return r;
		}
	}

	if (p->gang_size > 1) {
		for (i = 0; i < p->gang_size; ++i)
			amdgpu_job_set_gang_leader(p->jobs[i], leader);
	}

	/* No memory allocation is allowed while holding the notifier lock.
	 * The lock is held until amdgpu_cs_submit is finished and fence is
	 * added to BOs.
	 */
	mutex_lock(&p->adev->notifier_lock);

	/* If userptr are invalidated after amdgpu_cs_parser_bos(), return
	 * -EAGAIN, drmIoctl in libdrm will restart the amdgpu_cs_ioctl.
	 */
	r = 0;
	amdgpu_bo_list_for_each_userptr_entry(e, p->bo_list) {
		r |= !amdgpu_hmm_range_valid(e->range);
		amdgpu_hmm_range_free(e->range);
		e->range = NULL;
	}

	if (r || !list_empty(&vm->individual.needs_update)) {
		r = -EAGAIN;
		mutex_unlock(&p->adev->notifier_lock);
		return r;
	}

	p->fence = dma_fence_get(&leader->base.s_fence->finished);
	drm_exec_for_each_locked_object(&p->exec, gobj) {

		ttm_bo_move_to_lru_tail_unlocked(&gem_to_amdgpu_bo(gobj)->tbo);

		/* Everybody except for the gang leader uses READ */
		for (i = 0; i < p->gang_size; ++i) {
			if (p->jobs[i] == leader)
				continue;

			dma_resv_add_fence(gobj->resv,
					   &p->jobs[i]->base.s_fence->finished,
					   DMA_RESV_USAGE_READ);
		}

		/* The gang leader as remembered as writer */
		dma_resv_add_fence(gobj->resv, p->fence, DMA_RESV_USAGE_WRITE);
	}

	seq = amdgpu_ctx_add_fence(p->ctx, p->entities[p->gang_leader_idx],
				   p->fence);
	amdgpu_cs_post_dependencies(p);

	if ((leader->preamble_status & AMDGPU_PREAMBLE_IB_PRESENT) &&
	    !p->ctx->preamble_presented) {
		leader->preamble_status |= AMDGPU_PREAMBLE_IB_PRESENT_FIRST;
		p->ctx->preamble_presented = true;
	}

	cs->out.handle = seq;
	leader->uf_sequence = seq;

	amdgpu_vm_bo_trace_cs(&fpriv->vm, drm_exec_ticket(&p->exec));
	for (i = 0; i < p->gang_size; ++i) {
		amdgpu_job_free_resources(p->jobs[i]);
		trace_amdgpu_cs_ioctl(p->jobs[i]);
		drm_sched_entity_push_job(&p->jobs[i]->base);
		p->jobs[i] = NULL;
	}

	amdgpu_vm_move_to_lru_tail(p->adev, &fpriv->vm);

	mutex_unlock(&p->adev->notifier_lock);
	return 0;
}
/* …（同文件无关代码省略）… */
static void amdgpu_cs_parser_fini(struct amdgpu_cs_parser *parser)
{
	struct amdgpu_device *adev = parser->adev;
	struct amdgpu_bo_list_entry *e;
	unsigned int i;

	amdgpu_sync_free(&parser->sync);
	drm_exec_fini(&parser->exec);

	for (i = 0; i < parser->num_post_deps; i++) {
		drm_syncobj_put(parser->post_deps[i].syncobj);
		kfree(parser->post_deps[i].chain);
	}
	kfree(parser->post_deps);

	dma_fence_put(parser->fence);

	if (parser->ctx)
		amdgpu_ctx_put(parser->ctx);
	if (parser->bo_list) {
		if (adev->debug_vm) {
			/* Invalidate all BOs to test for userspace bugs */
			amdgpu_bo_list_for_each_entry(e, parser->bo_list) {
				struct amdgpu_bo *bo = e->bo;

				/* ignore duplicates */
				if (!bo)
					continue;

				amdgpu_vm_bo_invalidate(bo, false);
			}
		}
		amdgpu_bo_list_put(parser->bo_list);
	}

	for (i = 0; i < parser->nchunks; i++)
		kvfree(parser->chunks[i].kdata);
	kvfree(parser->chunks);
	for (i = 0; i < parser->gang_size; ++i) {
		if (parser->jobs[i])
			amdgpu_job_free(parser->jobs[i]);
	}
	amdgpu_bo_unref(&parser->uf_bo);
}
/* …（同文件无关代码省略）… */
int amdgpu_cs_ioctl(struct drm_device *dev, void *data, struct drm_file *filp)
{
	struct amdgpu_device *adev = drm_to_adev(dev);
	struct amdgpu_cs_parser parser;
	int r;

/* …（同文件无关代码省略）… */

	r = amdgpu_cs_patch_jobs(&parser);
	if (r)
		goto error_fini;

	r = amdgpu_cs_vm_handling(&parser);
	if (r)
		goto error_fini;

	r = amdgpu_cs_sync_rings(&parser);
	if (r)
		goto error_fini;

	trace_amdgpu_cs_ibs(&parser);

	r = amdgpu_cs_submit(&parser, data);
	if (r)
		goto error_fini;

	amdgpu_cs_parser_fini(&parser);
	return 0;

error_fini:
	amdgpu_cs_parser_fini(&parser);
	return r;
/* …（同文件无关代码省略）… */
int amdgpu_cs_find_mapping(struct amdgpu_cs_parser *parser,
			   uint64_t addr, struct amdgpu_bo **bo,
			   struct amdgpu_bo_va_mapping **map)
{
	struct amdgpu_fpriv *fpriv = parser->filp->driver_priv;
	struct ttm_operation_ctx ctx = { false, false };
	struct amdgpu_vm *vm = &fpriv->vm;
	struct amdgpu_bo_va_mapping *mapping;
	int i, r;

	addr /= AMDGPU_GPU_PAGE_SIZE;

	mapping = amdgpu_vm_bo_lookup_mapping(vm, addr);
	if (!mapping || !mapping->bo_va || !mapping->bo_va->base.bo)
		return -EINVAL;

	*bo = mapping->bo_va->base.bo;
	*map = mapping;

	/* Double check that the BO is reserved by this CS */
	if (dma_resv_locking_ctx((*bo)->tbo.base.resv) != drm_exec_ticket(&parser->exec))
		return -EINVAL;

	/* Make sure VRAM is allocated contigiously */
	(*bo)->flags |= AMDGPU_GEM_CREATE_VRAM_CONTIGUOUS;
	if ((*bo)->tbo.resource->mem_type == TTM_PL_VRAM &&
	    !((*bo)->tbo.resource->placement & TTM_PL_FLAG_CONTIGUOUS)) {

		amdgpu_bo_placement_from_domain(*bo, (*bo)->allowed_domains);
		for (i = 0; i < (*bo)->placement.num_placement; i++)
			(*bo)->placements[i].flags |= TTM_PL_FLAG_CONTIGUOUS;
		r = ttm_bo_validate(&(*bo)->tbo, &(*bo)->placement, &ctx);
		if (r)
			return r;
	}

	return amdgpu_ttm_alloc_gart(&(*bo)->tbo);
}
