// AUTO-DRAFT from torvalds/linux PR #5b0764a53235d3c77e1ed674e836e1b3fe1a4b1f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include <drm/drm_device.h>
#include <drm/drm_gem.h>
#include <drm/drm_gem_shmem_helper.h>
#include <drm/drm_print.h>
#include <drm/drm_syncobj.h>
#include <linux/hmm.h>
/* …（同文件无关代码省略）… */
static void aie2_cmd_release(struct kref *ref)
{
	struct amdxdna_drv_cmd *drv_cmd = container_of(ref, struct amdxdna_drv_cmd, refcnt);

	kfree(drv_cmd);
}
/* …（同文件无关代码省略）… */
static void aie2_cmd_put(struct amdxdna_drv_cmd *drv_cmd)
{
	kref_put(&drv_cmd->refcnt, aie2_cmd_release);
}
/* …（同文件无关代码省略）… */
static void aie2_job_release(struct kref *ref)
{
	struct amdxdna_sched_job *job;

	job = container_of(ref, struct amdxdna_sched_job, refcnt);

	amdxdna_sched_job_cleanup(job);
	atomic64_inc(&job->hwctx->job_free_cnt);
	wake_up(&job->hwctx->priv->job_free_wq);
	if (job->out_fence)
		dma_fence_put(job->out_fence);
	if (job->drv_cmd)
		aie2_cmd_put(job->drv_cmd);
	kfree(job->aie2_job_health);
	kfree(job);
}
/* …（同文件无关代码省略）… */
static void aie2_job_put(struct amdxdna_sched_job *job)
{
	kref_put(&job->refcnt, aie2_job_release);
}
/* …（同文件无关代码省略）… */
static void aie2_hwctx_stop(struct amdxdna_dev *xdna, struct amdxdna_hwctx *hwctx,
			    struct drm_sched_job *bad_job)
{
	drm_sched_stop(&hwctx->priv->sched, bad_job);
	aie2_destroy_context(xdna->dev_handle, hwctx);
	drm_sched_start(&hwctx->priv->sched, 0);
}

static int aie2_hwctx_restart(struct amdxdna_dev *xdna, struct amdxdna_hwctx *hwctx)
{
	struct amdxdna_gem_obj *heap = hwctx->priv->heap;
	unsigned long heap_id;
	int ret;

	ret = aie2_create_context(xdna->dev_handle, hwctx);
	if (ret) {
		XDNA_ERR(xdna, "Create hwctx failed, ret %d", ret);
		goto out;
	}

	ret = aie2_map_host_buf(xdna->dev_handle, hwctx->fw_ctx_id,
				amdxdna_obj_dma_addr(heap),
				heap->mem.size);
	if (ret) {
		XDNA_ERR(xdna, "Map host buf failed, ret %d", ret);
		goto out;
	}

	xa_for_each_range(&hwctx->client->dev_heap_xa, heap_id, heap, 1,
			  hwctx->last_attached_heap) {
		ret = aie2_add_host_buf(xdna->dev_handle, hwctx->fw_ctx_id,
					amdxdna_obj_dma_addr(heap),
					heap->mem.size);
		if (ret) {
			XDNA_ERR(xdna, "Add heap %ld failed ret %d", heap_id, ret);
			goto out;
		}
	}

	ret = aie2_config_cu(hwctx, NULL);
	if (ret) {
		XDNA_ERR(xdna, "Config cu failed, ret %d", ret);
		goto out;
	}

out:
	XDNA_DBG(xdna, "%s restarted, ret %d", hwctx->name, ret);
	return ret;
}
/* …（同文件无关代码省略）… */
static struct dma_fence *aie2_cmd_get_out_fence(struct amdxdna_hwctx *hwctx, u64 seq)
{
	struct dma_fence *fence, *out_fence = NULL;
	int ret;

	fence = drm_syncobj_fence_get(hwctx->priv->syncobj);
	if (!fence)
		return NULL;

	ret = dma_fence_chain_find_seqno(&fence,  seq);
	if (ret)
		goto out;

	out_fence = dma_fence_get(dma_fence_chain_contained(fence));

out:
	dma_fence_put(fence);
	return out_fence;
}
/* …（同文件无关代码省略）… */
static void aie2_hwctx_wait_for_idle(struct amdxdna_hwctx *hwctx)
{
	struct dma_fence *fence;

	fence = aie2_cmd_get_out_fence(hwctx, hwctx->priv->seq - 1);
	if (!fence)
		return;

	/* Wait up to 2 seconds for fw to finish all pending requests */
	dma_fence_wait_timeout(fence, false, msecs_to_jiffies(2000));
	dma_fence_put(fence);
}
/* …（同文件无关代码省略）… */
	int ret = 0;
	u32 status;

	cmd_abo = job->cmd_bo;

	if (unlikely(job->job_timeout)) {
/* …（同文件无关代码省略）… */
	struct amdxdna_sched_job *job = handle;
	int ret = 0;

	if (unlikely(!data || size != sizeof(u32))) {
		job->drv_cmd->result = U32_MAX;
		ret = -EINVAL;
/* …（同文件无关代码省略）… */
	u32 cmd_status;
	int ret = 0;

	cmd_abo = job->cmd_bo;

	if (unlikely(job->job_timeout)) {
/* …（同文件无关代码省略）… */
	kref_get(&job->refcnt);
	fence = dma_fence_get(job->fence);

	if (job->drv_cmd) {
		switch (job->drv_cmd->opcode) {
		case SYNC_DEBUG_BO:
/* …（同文件无关代码省略）… */

out:
	if (ret) {
		dma_fence_put(job->fence);
		aie2_job_put(job);
		mmput(job->mm);
/* …（同文件无关代码省略）… */
static int aie2_hwctx_col_list(struct amdxdna_hwctx *hwctx)
{
	struct amdxdna_dev *xdna = hwctx->client->xdna;
	struct amdxdna_dev_hdl *ndev;
	int start, end, first, last;
	u32 width = 1, entries = 0;
	int i;

	if (!hwctx->num_tiles) {
		XDNA_ERR(xdna, "Number of tiles is zero");
		return -EINVAL;
	}

	ndev = xdna->dev_handle;
	if (unlikely(!ndev->aie.metadata.core.row_count)) {
		XDNA_WARN(xdna, "Core tile row count is zero");
		return -EINVAL;
	}

	hwctx->num_col = hwctx->num_tiles / ndev->aie.metadata.core.row_count;
	if (!hwctx->num_col || hwctx->num_col > ndev->total_col) {
		XDNA_ERR(xdna, "Invalid num_col %d", hwctx->num_col);
		return -EINVAL;
	}

	if (ndev->priv->col_align == COL_ALIGN_NATURE)
		width = hwctx->num_col;

	/*
	 * In range [start, end], find out columns that is multiple of width.
	 *	'first' is the first column,
	 *	'last' is the last column,
	 *	'entries' is the total number of columns.
	 */
	start =  xdna->dev_info->first_col;
	end =  ndev->total_col - hwctx->num_col;
	if (start > 0 && end == 0) {
		XDNA_DBG(xdna, "Force start from col 0");
		start = 0;
	}
	first = start + (width - start % width) % width;
	last = end - end % width;
	if (last >= first)
		entries = (last - first) / width + 1;
	XDNA_DBG(xdna, "start %d end %d first %d last %d",
		 start, end, first, last);

	if (unlikely(!entries)) {
		XDNA_ERR(xdna, "Start %d end %d width %d",
			 start, end, width);
		return -EINVAL;
	}

	hwctx->col_list = kmalloc_array(entries, sizeof(*hwctx->col_list), GFP_KERNEL);
	if (!hwctx->col_list)
		return -ENOMEM;

	hwctx->col_list_len = entries;
	hwctx->col_list[0] = first;
	for (i = 1; i < entries; i++)
		hwctx->col_list[i] = hwctx->col_list[i - 1] + width;

	print_hex_dump_debug("col_list: ", DUMP_PREFIX_OFFSET, 16, 4, hwctx->col_list,
			     entries * sizeof(*hwctx->col_list), false);
	return 0;
}
/* …（同文件无关代码省略）… */
static void aie2_release_resource(struct amdxdna_hwctx *hwctx)
{
	struct amdxdna_dev *xdna = hwctx->client->xdna;
	int ret;

	ret = xrs_release_resource(xdna->xrs_hdl, (uintptr_t)hwctx);
	if (ret)
		XDNA_ERR(xdna, "Release AIE res
