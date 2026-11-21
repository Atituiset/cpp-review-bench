// AUTO-DRAFT from torvalds/linux PR #c44104f3642da720164042623e4832836aefdd8e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static void ivpu_pm_prepare_cold_boot(struct ivpu_device *vdev)
{
	struct ivpu_fw_info *fw = vdev->fw;

	ivpu_cmdq_reset_all_contexts(vdev);
	ivpu_ipc_reset(vdev);
	ivpu_fw_log_reset(vdev);
	fw->last_heartbeat = 0;

	ivpu_dbg(vdev, FW_BOOT, "Cold boot entry point 0x%llx", vdev->fw->cold_boot_entry_point);
	fw->next_boot_mode = VPU_BOOT_TYPE_COLDBOOT;
}
/* …（同文件无关代码省略）… */
static int ivpu_suspend(struct ivpu_device *vdev)
{
	int ret;

	ivpu_prepare_for_reset(vdev);

	ret = ivpu_shutdown(vdev);
	if (ret)
		ivpu_err(vdev, "Failed to shutdown NPU: %d\n", ret);

	return ret;
}
/* …（同文件无关代码省略）… */
static int ivpu_resume(struct ivpu_device *vdev)
{
	int ret;

retry:
	pci_set_power_state(to_pci_dev(vdev->drm.dev), PCI_D0);
	pci_restore_state(to_pci_dev(vdev->drm.dev));

	ret = ivpu_hw_power_up(vdev);
	if (ret) {
		ivpu_err(vdev, "Failed to power up HW: %d\n", ret);
		goto err_power_down;
	}

	ret = ivpu_mmu_enable(vdev);
	if (ret) {
		ivpu_err(vdev, "Failed to resume MMU: %d\n", ret);
		goto err_power_down;
	}

	ret = ivpu_boot(vdev);
	if (ret)
		goto err_mmu_disable;

	return 0;

err_mmu_disable:
	ivpu_mmu_disable(vdev);
err_power_down:
	ivpu_hw_power_down(vdev);
	pci_set_power_state(to_pci_dev(vdev->drm.dev), PCI_D3hot);

	if (ivpu_fw_is_warm_boot(vdev)) {
		ivpu_pm_prepare_cold_boot(vdev);
		goto retry;
	} else {
		ivpu_err(vdev, "Failed to resume the FW: %d\n", ret);
	}

	return ret;
}
/* …（同文件无关代码省略）… */
static void ivpu_pm_reset_begin(struct ivpu_device *vdev)
{
	pm_runtime_disable(vdev->drm.dev);

	atomic_inc(&vdev->pm->reset_counter);
	atomic_set(&vdev->pm->reset_pending, 1);
	down_write(&vdev->pm->reset_lock);
}
/* …（同文件无关代码省略）… */
static void ivpu_pm_reset_complete(struct ivpu_device *vdev)
{
	int ret;

	ivpu_pm_prepare_cold_boot(vdev);
	ivpu_jobs_abort_all(vdev);
	ivpu_ms_cleanup_all(vdev);

	ret = ivpu_resume(vdev);
	if (ret) {
		ivpu_err(vdev, "Failed to resume NPU: %d\n", ret);
		pm_runtime_set_suspended(vdev->drm.dev);
	} else {
		pm_runtime_set_active(vdev->drm.dev);
	}

	up_write(&vdev->pm->reset_lock);
	atomic_set(&vdev->pm->reset_pending, 0);

	pm_runtime_mark_last_busy(vdev->drm.dev);
	pm_runtime_enable(vdev->drm.dev);
}

static void ivpu_pm_recovery_work(struct work_struct *work)
{
	struct ivpu_pm_info *pm = container_of(work, struct ivpu_pm_info, recovery_work);
	struct ivpu_device *vdev = pm->vdev;
	char *evt[2] = {"IVPU_PM_EVENT=IVPU_RECOVER", NULL};

	ivpu_err(vdev, "Recovering the NPU (reset #%d)\n", atomic_read(&vdev->pm->reset_counter));

	ivpu_pm_reset_begin(vdev);

	if (!pm_runtime_status_suspended(vdev->drm.dev)) {
		ivpu_jsm_state_dump_no_reply(vdev);
		ivpu_dev_coredump(vdev);
		ivpu_suspend(vdev);
	}

	ivpu_pm_reset_complete(vdev);

	kobject_uevent_env(&vdev->drm.dev->kobj, KOBJ_CHANGE, evt);
}
/* …（同文件无关代码省略）… */
void ivpu_pm_trigger_recovery(struct ivpu_device *vdev, const char *reason)
{
	ivpu_err(vdev, "Recovery triggered by %s\n", reason);

	if (ivpu_disable_recovery) {
		ivpu_err(vdev, "Recovery not available when disable_recovery param is set\n");
		return;
	}

	/* Trigger recovery if it's not in progress */
	if (atomic_cmpxchg(&vdev->pm->reset_pending, 0, 1) == 0) {
		ivpu_hw_diagnose_failure(vdev);
		ivpu_hw_irq_disable(vdev); /* Disable IRQ early to protect from IRQ storm */
		queue_work(system_dfl_wq, &vdev->pm->recovery_work);
	}
}
/* …（同文件无关代码省略）… */
static void ivpu_job_timeout_work(struct work_struct *work)
{
	struct ivpu_pm_info *pm = container_of(work, struct ivpu_pm_info, job_timeout_work.work);
	struct ivpu_device *vdev = pm->vdev;
	unsigned long timeout_ms = ivpu_tdr_timeout_ms ? ivpu_tdr_timeout_ms : vdev->timeout.tdr;
	unsigned long inference_timeout_ms = ivpu_inference_timeout_ms ? ivpu_inference_timeout_ms :
					     vdev->timeout.inference;
	u64 inference_max_retries;
	u64 heartbeat;

	if (ivpu_jsm_get_heartbeat(vdev, 0, &heartbeat) || heartbeat <= vdev->fw->last_heartbeat) {
		ivpu_err(vdev, "Job timeout detected, heartbeat not progressed\n");
		goto abort;
	}

	inference_max_retries = DIV_ROUND_UP(inference_timeout_ms, timeout_ms);
	if (atomic_fetch_inc(&vdev->job_timeout_counter) >= inference_max_retries) {
		ivpu_err(vdev, "Job timeout detected, heartbeat limit (%lld) exceeded\n",
			 inference_max_retries);
		goto abort;
	}

	vdev->fw->last_heartbeat = heartbeat;
	ivpu_start_job_timeout_detection(vdev);
	return;

abort:
	atomic_set(&vdev->job_timeout_counter, 0);

	if (vdev->fw->sched_mode == VPU_SCHEDULING_MODE_OS) {
		ivpu_pm_trigger_recovery(vdev, "Job timeout");
		return;
	}

	ivpu_jsm_state_dump(vdev);
	ivpu_dev_coredump(vdev);
	atomic_set(&vdev->job_timeout_detected, 1);
	queue_work(system_percpu_wq, &vdev->context_abort_work);
}
/* …（同文件无关代码省略）… */
void ivpu_start_job_timeout_detection(struct ivpu_device *vdev)
{
	unsigned long timeout_ms = ivpu_tdr_timeout_ms ? ivpu_tdr_timeout_ms : vdev->timeout.tdr;

	/* No-op if already queued */
	queue_delayed_work(system_percpu_wq, &vdev->pm->job_timeout_work,
			   msecs_to_jiffies(timeout_ms));
}
/* …（同文件无关代码省略）… */
	atomic_set(&pm->engine_reset_counter, 0);

	INIT_WORK(&pm->recovery_work, ivpu_pm_recovery_work);
	INIT_DELAYED_WORK(&pm->job_timeout_work, ivpu_job_timeout_work);

	if (ivpu_disable_recovery)
/* …（同文件无关代码省略）… */
{
	struct device *dev = vdev->drm.dev;

	pm_runtime_allow(dev);
	pm_runtime_put_autosuspend(dev);
}
