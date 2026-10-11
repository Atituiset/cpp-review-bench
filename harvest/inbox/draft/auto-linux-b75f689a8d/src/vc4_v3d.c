// AUTO-DRAFT from torvalds/linux PR #c61e23ed14359e6289c08e55940dd3401fe27db0
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdint.h>
  // <<< BUG ANCHOR
static void vc4_v3d_init_hw(struct drm_device *dev)
{
	struct vc4_dev *vc4 = to_vc4_dev(dev);

	/* Take all the memory that would have been reserved for user
	 * QPU programs, since we don't have an interface for running
	 * them, anyway.
	 */
	V3D_WRITE(V3D_VPMBASE, 0);
}

int vc4_v3d_get_bin_slot(struct vc4_dev *vc4)
{
	struct drm_device *dev = &vc4->base;
	unsigned long irqflags;
	int slot;
	uint64_t seqno = 0;
	struct vc4_exec_info *exec;

	if (WARN_ON_ONCE(vc4->gen > VC4_GEN_4))
		return -ENODEV;

try_again:
	spin_lock_irqsave(&vc4->job_lock, irqflags);
	slot = ffs(~vc4->bin_alloc_used);
	if (slot != 0) {
		/* Switch from ffs() bit index to a 0-based index. */
		slot--;
		vc4->bin_alloc_used |= BIT(slot);
		spin_unlock_irqrestore(&vc4->job_lock, irqflags);
		return slot;
	}

	/* Couldn't find an open slot.  Wait for render to complete
	 * and try again.
	 */
	exec = vc4_last_render_job(vc4);
	if (exec)
		seqno = exec->seqno;
	spin_unlock_irqrestore(&vc4->job_lock, irqflags);

	if (seqno) {
		int ret = vc4_wait_for_seqno(dev, seqno, ~0ull, true);

		if (ret == 0)
			goto try_again;

		return ret;
	}

	return -ENOMEM;
}

/*
/* …（同文件无关代码省略）… */
	vc4_v3d_init_hw(&vc4->base);

	vc4_irq_enable(&vc4->base);

	return 0;
}
/* …（同文件无关代码省略）… */
		return ret;
	vc4->irq = ret;

	ret = devm_pm_runtime_enable(dev);
	if (ret)
		return ret;
/* …（同文件无关代码省略）… */
	V3D_WRITE(V3D_BPOA, 0);
	V3D_WRITE(V3D_BPOS, 0);

	ret = vc4_irq_install(drm, vc4->irq);
	if (ret) {
		drm_err(drm, "Failed to install IRQ handler\n");
		goto err_put_runtime_pm;
	}

	pm_runtime_use_autosuspend(dev);
	pm_runtime_set_autosuspend_delay(dev, 40); /* a little over 2 frames. */
	pm_runtime_put_autosuspend(dev);
