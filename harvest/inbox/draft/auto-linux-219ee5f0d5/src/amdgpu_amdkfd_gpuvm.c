// AUTO-DRAFT from torvalds/linux PR #c639ac6b9cf100b860408a3cf8d7eab268866cd7
			struct drm_gem_object *gobj;
  // <<< BUG ANCHOR
			gobj = &mem->bo->tbo.base;
			ret = drm_exec_prepare_obj(&exec, gobj, 1);
			drm_exec_retry_on_contention(&exec);
			if (unlikely(ret)) {
				pr_err("drm_exec_prepare_obj failed, ret: %d\n", ret);
