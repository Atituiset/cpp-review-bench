// AUTO-DRAFT from torvalds/linux PR #9d66f26fefacebfeed15de31c1123451a17ff119
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#define is_rproc_enabled IS_ENABLED(CONFIG_REMOTEPROC)
/* …（同文件无关代码省略）… */
struct port_buffer {
	char *buf;

	/* size of the buffer in *buf above */
	size_t size;

	/* used length of the buffer */
	size_t len;
	/* offset in the buf from which to consume data */
	size_t offset;

	/* DMA address of buffer */
	dma_addr_t dma;

	/* Device we got DMA memory from */
	struct device *dev;

	/* List of pending dma buffers to free */
	struct list_head list;

	/* If sgpages == 0 then buf is used */
	unsigned int sgpages;

	/* sg is used if spages > 0. sg must be the last in is struct */
	struct scatterlist sg[] __counted_by(sgpages);
};
/* …（同文件无关代码省略）… */
struct ports_device {
	/* Next portdev in the list, head is in the pdrvdata struct */
	struct list_head list;

	/*
	 * Workqueue handlers where we process deferred work after
	 * notification
	 */
	struct work_struct control_work;
	struct work_struct config_work;

	struct list_head ports;

	/* To protect the list of ports */
	spinlock_t ports_lock;

	/* To protect the vq operations for the control channel */
	spinlock_t c_ivq_lock;
	spinlock_t c_ovq_lock;

	/* max. number of ports this device can hold */
	u32 max_nr_ports;

	/* The virtio device we're associated with */
	struct virtio_device *vdev;

	/*
	 * A couple of virtqueues for the control channel: one for
	 * guest->host transfers, one for host->guest transfers
	 */
	struct virtqueue *c_ivq, *c_ovq;

	/*
	 * A control packet buffer for guest->host requests, protected
	 * by c_ovq_lock.
	 */
	struct virtio_console_control cpkt;

	/* Array of per-port IO virtqueues */
	struct virtqueue **in_vqs, **out_vqs;

	/* Major number for this device.  Ports will be created as minors. */
	int chr_major;

	/*
	 * Set to true during PM freeze to block TX paths that may race
	 * with virtqueue teardown (e.g. hvc put_chars with no_console_suspend).
	 */
	bool pm_freezing;
};
/* …（同文件无关代码省略）… */
static bool is_rproc_serial(const struct virtio_device *vdev)
{
	return is_rproc_enabled && vdev->id.device == VIRTIO_ID_RPROC_SERIAL;
}
/* …（同文件无关代码省略）… */
static void free_buf(struct port_buffer *buf, bool can_sleep)
{
	unsigned int i;

	for (i = 0; i < buf->sgpages; i++) {
		struct page *page = sg_page(&buf->sg[i]);
		if (!page)
			break;
		put_page(page);
	}

	if (!buf->dev) {
		kfree(buf->buf);
	} else if (is_rproc_enabled) {
		unsigned long flags;

		/* dma_free_coherent requires interrupts to be enabled. */
		if (!can_sleep) {
			/* queue up dma-buffers to be freed later */
			spin_lock_irqsave(&dma_bufs_lock, flags);
			list_add_tail(&buf->list, &pending_free_dma_bufs);
			spin_unlock_irqrestore(&dma_bufs_lock, flags);
			return;
		}
		dma_free_coherent(buf->dev, buf->size, buf->buf, buf->dma);

		/* Release device refcnt and allow it to be freed */
		put_device(buf->dev);
	}

	kfree(buf);
}
/* …（同文件无关代码省略）… */
static void reclaim_dma_bufs(void)
{
	unsigned long flags;
	struct port_buffer *buf, *tmp;
	LIST_HEAD(tmp_list);

	if (list_empty(&pending_free_dma_bufs))
		return;

	/* Create a copy of the pending_free_dma_bufs while holding the lock */
	spin_lock_irqsave(&dma_bufs_lock, flags);
	list_cut_position(&tmp_list, &pending_free_dma_bufs,
			  pending_free_dma_bufs.prev);
	spin_unlock_irqrestore(&dma_bufs_lock, flags);

	/* Release the dma buffers, without irqs enabled */
	list_for_each_entry_safe(buf, tmp, &tmp_list, list) {
		list_del(&buf->list);
		free_buf(buf, true);
	}
}
/* …（同文件无关代码省略）… */
static struct port_buffer *alloc_buf(struct virtio_device *vdev, size_t buf_size,
				     int pages, gfp_t gfp)
{
	struct port_buffer *buf;

	reclaim_dma_bufs();

	/*
	 * Allocate buffer and the sg list. The sg list array is allocated
	 * directly after the port_buffer struct.
	 */
	buf = kmalloc_flex(*buf, sg, pages, gfp);
	if (!buf)
		goto fail;

	buf->sgpages = pages;
	if (pages > 0) {
		buf->dev = NULL;
		buf->buf = NULL;
		return buf;
	}

	if (is_rproc_serial(vdev)) {
		/*
		 * Allocate DMA memory from ancestor. When a virtio
		 * device is created by remoteproc, the DMA memory is
		 * associated with the parent device:
		 * virtioY => remoteprocX#vdevYbuffer.
		 */
		buf->dev = vdev->dev.parent;
		if (!buf->dev)
			goto free_buf;

		/* Increase device refcnt to avoid freeing it */
		get_device(buf->dev);
		buf->buf = dma_alloc_coherent(buf->dev, buf_size, &buf->dma, gfp);
	} else {
		buf->dev = NULL;
		buf->buf = kmalloc(buf_size, gfp);
	}

	if (!buf->buf)
		goto free_buf;
	buf->len = 0;
	buf->offset = 0;
	buf->size = buf_size;
	return buf;

free_buf:
	kfree(buf);
fail:
	return NULL;
}
/* …（同文件无关代码省略）… */
static void reclaim_consumed_buffers(struct port *port)
{
	struct port_buffer *buf;
	unsigned int len;

	if (!port->portdev) {
		/* Device has been unplugged.  vqs are already gone. */
		return;
	}
	while ((buf = virtqueue_get_buf(port->out_vq, &len))) {
		free_buf(buf, false);
		port->outvq_full = false;
	}
}
/* …（同文件无关代码省略）… */
static ssize_t __send_to_port(struct port *port, struct scatterlist *sg,
			      int nents, size_t in_count,
			      struct port_buffer *buf, bool nonblock)
{
	struct virtqueue *out_vq;
	int err;
	unsigned long flags;
	unsigned int len;
	struct ports_device *portdev;

	spin_lock_irqsave(&port->outvq_lock, flags);

	portdev = READ_ONCE(port->portdev);

	if (!portdev) {
		in_count = 0;
		goto free_and_done;
	}

	/*
	 * Check freeze flag under the lock so that the flag check and
	 * virtqueue_add_outbuf() are atomic with respect to
	 * remove_port_data() which also takes outvq_lock.  This
	 * guarantees that once remove_port_data() returns, no new
	 * buffers can be added before remove_vqs() tears down the vq.
	 * Pairs with smp_store_release() in virtcons_freeze/restore.
	 */
	if (smp_load_acquire(&portdev->pm_freezing)) /* pairs with freeze/restore */
		goto free_and_done;

	out_vq = port->out_vq;

	reclaim_consumed_buffers(port);

	err = virtqueue_a
