// AUTO-DRAFT from torvalds/linux PR #5f22fb20497f4983094131f00a734362300d22d2
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define VQ_NAME_LEN	24
/* …（同文件无关代码省略）… */
struct virtio_fs_vq {
	spinlock_t lock;
	struct virtqueue *vq;     /* protected by ->lock */
	struct work_struct done_work;
	struct list_head queued_reqs;
	struct list_head end_reqs;	/* End these requests */
	struct work_struct dispatch_work;
	struct fuse_dev *fud;
	bool connected;
	long in_flight;
	struct completion in_flight_zero; /* No inflight requests */
	struct kobject *kobj;
	char name[VQ_NAME_LEN];
} ____cacheline_aligned_in_smp;
/* …（同文件无关代码省略）… */
{
	struct virtio_fs *vfs = container_of(kobj, struct virtio_fs, kobj);

	kfree(vfs->mq_map);
	kfree(vfs->vqs);
	kfree(vfs);
/* …（同文件无关代码省略）… */
static void virtio_fs_start_all_queues(struct virtio_fs *fs)
{
	struct virtio_fs_vq *fsvq;
	int i;

	for (i = 0; i < fs->nvqs; i++) {
		fsvq = &fs->vqs[i];
		spin_lock(&fsvq->lock);
		fsvq->connected = true;
		spin_unlock(&fsvq->lock);
	}
}
/* …（同文件无关代码省略）… */
static void virtio_fs_free_devs(struct virtio_fs *fs)
{
	unsigned int i;

	for (i = 0; i < fs->nvqs; i++) {
		struct virtio_fs_vq *fsvq = &fs->vqs[i];

		if (!fsvq->fud)
			continue;

		fuse_dev_put(fsvq->fud);
		fsvq->fud = NULL;
	}
}

/* Read filesystem name from virtio config into fs->tag (must kfree()). */
static int virtio_fs_read_tag(struct virtio_device *vdev, struct virtio_fs *fs)
{
	char tag_buf[sizeof_field(struct virtio_fs_config, tag)];
/* …（同文件无关代码省略）… */
		end = &tag_buf[sizeof(tag_buf)];

	len = end - tag_buf;
	fs->tag = devm_kmalloc(&vdev->dev, len + 1, GFP_KERNEL);
	if (!fs->tag)
		return -ENOMEM;
	memcpy(fs->tag, tag_buf, len);
/* …（同文件无关代码省略）… */
	unsigned int offset = 0;
	unsigned int num_in;
	unsigned int num_out;
	unsigned int len;
	unsigned int i;

	num_in = args->in_numargs - args->in_pages;
	num_out = args->out_numargs - args->out_pages;
	len = fuse_len_args(num_in, (struct fuse_arg *) args->in_args) +
	      fuse_len_args(num_out, args->out_args);

	req->argbuf = kmalloc(len, gfp);
	if (!req->argbuf)
		return -ENOMEM;

/* …（同文件无关代码省略）… */
		offset += args->in_args[i].size;
	}

	return 0;
}

/* …（同文件无关代码省略）… */
static inline void virtio_fs_ctx_set_defaults(struct fuse_fs_context *ctx)
{
	ctx->rootmode = S_IFDIR;
	ctx->default_permissions = 1;
	ctx->allow_other = 1;
	ctx->max_read = UINT_MAX;
	ctx->blksize = 512;
	ctx->destroy = true;
	ctx->no_control = true;
	ctx->no_force_umount = true;
}

static int virtio_fs_fill_super(struct super_block *sb, struct fs_context *fsc)
{
	struct fuse_mount *fm = get_fuse_mount_super(sb);
	struct fuse_conn *fc = fm->fc;
	struct virtio_fs *fs = fc->chan->iq.priv;
	struct fuse_fs_context *ctx = fsc->fs_private;
	unsigned int i;
	int err;

	virtio_fs_ctx_set_defaults(ctx);
	mutex_lock(&virtio_fs_mutex);

	/* After holding mutex, make sure virtiofs device is still there.
	 * Though we are holding a reference to it, drive ->remove might
	 * still have cleaned up virtual queues. In that case bail out.
	 */
	err = -EINVAL;
	if (list_empty(&fs->list)) {
		pr_info("virtio-fs: tag <%s> not found\n", fs->tag);
		goto err;
	}

	err = -ENOMEM;
	/* Allocate fuse_dev for hiprio and notification queues */
	for (i = 0; i < fs->nvqs; i++) {
		struct virtio_fs_vq *fsvq = &fs->vqs[i];

		fsvq->fud = fuse_dev_alloc();
		if (!fsvq->fud)
			goto err_free_fuse_devs;
	}

	if (ctx->vdax_mode != FUSE_VDAX_NEVER) {
		if (ctx->vdax_mode == FUSE_VDAX_ALWAYS && !fs->dax_dev) {
			err = -EINVAL;
			pr_err("virtio-fs: dax can't be enabled as filesystem"
			       " device does not support it.\n");
			goto err_free_fuse_devs;
		}
		ctx->vdax_dev = fs->dax_dev;
	}
	err = fuse_fill_super_common(sb, ctx);
	if (err < 0)
		goto err_free_fuse_devs;

	for (i = 0; i < fs->nvqs; i++) {
		struct virtio_fs_vq *fsvq = &fs->vqs[i];

		fuse_dev_install(fsvq->fud, fc->chan);
	}

	/* Previous unmount will stop all queues. Start these again */
	virtio_fs_start_all_queues(fs);
	fuse_send_init(fm);
	mutex_unlock(&virtio_fs_mutex);
	return 0;

err_free_fuse_devs:
	virtio_fs_free_devs(fs);
err:
	mutex_unlock(&virtio_fs_mutex);
	return err;
}
/* …（同文件无关代码省略）… */
	ctx = kzalloc_obj(struct fuse_fs_context);
	if (!ctx)
		return -ENOMEM;
	fsc->fs_private = ctx;
	fsc->ops = &virtio_fs_context_ops;
	return 0;
