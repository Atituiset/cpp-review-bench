// AUTO-DRAFT from torvalds/linux PR #69e2b55fc3b4fa3639b8c320374c255f046ddda5
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

struct virtrng_info {
	struct hwrng hwrng;
	struct virtqueue *vq;
	char name[25];
	int index;
	bool hwrng_register_done;
	bool hwrng_removed;
	/* data transfer */
	struct completion have_data;
	unsigned int data_avail;
	unsigned int data_idx;
	/* minimal size returned by rng_buffer_size() */
	__dma_from_device_group_begin();
#if SMP_CACHE_BYTES < 32
	u8 data[32];
#else
	u8 data[SMP_CACHE_BYTES];
#endif
	__dma_from_device_group_end();
};
/* …（同文件无关代码省略）… */
err_find:
	ida_free(&rng_index_ida, index);
err_ida:
	kfree(vi);
	return err;
}
/* …（同文件无关代码省略）… */
{
	struct virtrng_info *vi = vdev->priv;

	vi->hwrng_removed = true;
	vi->data_avail = 0;
	vi->data_idx = 0;
/* …（同文件无关代码省略）… */
	virtio_reset_device(vdev);
	vdev->config->del_vqs(vdev);
	ida_free(&rng_index_ida, vi->index);
	kfree(vi);
}
