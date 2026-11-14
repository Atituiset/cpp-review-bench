// AUTO-DRAFT from torvalds/linux PR #e93fd809fb5692d1571bd008988ba734fd7fc976
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
#define UBLK_MINORS		(1U << MINORBITS)
/* …（同文件无关代码省略）… */
struct ublk_batch_fetch_cmd {
	struct list_head node;
	struct io_uring_cmd *cmd;
	unsigned short buf_group;
};
/* …（同文件无关代码省略）… */
#define UBLK_IO_FLAG_ACTIVE	0x01
/* …（同文件无关代码省略）… */
#define UBLK_IO_FLAG_CANCELED	0x80000000
/* …（同文件无关代码省略）… */
union ublk_io_buf {
	__u64	addr;
	struct ublk_auto_buf_reg auto_reg;
};
/* …（同文件无关代码省略）… */
struct ublk_io {
	union ublk_io_buf buf;
	unsigned int flags;
	int res;

	union {
		/* valid if UBLK_IO_FLAG_ACTIVE is set */
		struct io_uring_cmd *cmd;
		/* valid if UBLK_IO_FLAG_OWNED_BY_SRV is set */
		struct request *req;
	};

	struct task_struct *task;

	/*
	 * The number of uses of this I/O by the ublk server
	 * if user copy or zero copy are enabled:
	 * - UBLK_REFCOUNT_INIT from dispatch to the server
	 *   until UBLK_IO_COMMIT_AND_FETCH_REQ
	 * - 1 for each inflight ublk_ch_{read,write}_iter() call not on task
	 * - 1 for each io_uring registered buffer not registered on task
	 * The I/O can only be completed once all references are dropped.
	 * User copy and buffer registration operations are only permitted
	 * if the reference count is nonzero.
	 */
	refcount_t ref;
	/* Count of buffers registered on task and not yet unregistered */
	unsigned task_registered_buffers;

	void *buf_ctx_handle;
	spinlock_t lock;
} ____cacheline_aligned_in_smp;
/* …（同文件无关代码省略）… */
struct ublk_queue {
	u16 q_id;
	u16 q_depth;
	u16 io_desc_size;

	unsigned long flags;
	struct ublksrv_io_desc *io_cmd_buf;

	bool force_abort;
	bool canceling;
	bool fail_io; /* copy of dev->state == UBLK_S_DEV_FAIL_IO */
	spinlock_t		cancel_lock;
	struct ublk_device *dev;
	u16 nr_io_ready;

	/*
	 * For supporting UBLK_F_BATCH_IO only.
	 *
	 * Inflight ublk request tag is saved in this fifo
	 *
	 * There are multiple writer from ublk_queue_rq() or ublk_queue_rqs(),
	 * so lock is required for storing request tag to fifo
	 *
	 * Make sure just one reader for fetching request from task work
	 * function to ublk server, so no need to grab the lock in reader
	 * side.
	 *
	 * Batch I/O State Management:
	 *
	 * The batch I/O system uses implicit state management based on the
	 * combination of three key variables below.
	 *
	 * - IDLE: list_empty(&fcmd_head) && !active_fcmd
	 *   No fetch commands available, events queue in evts_fifo
	 *
	 * - READY: !list_empty(&fcmd_head) && !active_fcmd
	 *   Fetch commands available but none processing events
	 *
	 * - ACTIVE: active_fcmd
	 *   One fetch command actively processing events from evts_fifo
	 *
	 * Key Invariants:
	 * - At most one active_fcmd at any time (single reader)
	 * - active_fcmd is always from fcmd_head list when non-NULL
	 * - evts_fifo can be read locklessly by the single active reader
	 * - All state transitions require evts_lock protection
	 * - Multiple writers to evts_fifo require lock protection
	 */
	struct {
		DECLARE_KFIFO_PTR(evts_fifo, unsigned short);
		spinlock_t evts_lock;

		/* List of fetch commands available to process events */
		struct list_head fcmd_head;

		/* Currently active fetch command (NULL = none active) */
		struct ublk_batch_fetch_cmd  *active_fcmd;
	}____cacheline_aligned_in_smp;

	struct ublk_io ios[] __counted_by(q_depth);
};
/* …（同文件无关代码省略）… */
struct ublk_device {
	struct gendisk		*ub_disk;

	struct ublksrv_ctrl_dev_info	dev_info;

	struct blk_mq_tag_set	tag_set;

	struct cdev		cdev;
	struct device		cdev_dev;

#define UB_STATE_OPEN		0
#define UB_STATE_USED		1
#define UB_STATE_DELETED	2
/* STOP_DEV canceled the server's commands, until its release */
#define UB_STATE_STOPPING	3
	unsigned long		state;
	int			ub_number;

	struct mutex		mutex;

	spinlock_t		lock;
	struct mm_struct	*mm;

	struct ublk_params	params;

	u16			nr_queue_ready;
	bool 			unprivileged_daemons;
	struct mutex cancel_mutex;
	/* the open /dev/ublkcN, protected by cancel_mutex */
	struct file *ch_file;
	/*
	 * A cancel started in this FETCH round. Set by ublk_set_canceling(),
	 * cleared only by ublk_reset_ch_dev() when a new round starts. While
	 * it is set, no queue clears its ->canceling.
	 */
	bool canceling;
	pid_t 	ublksrv_tgid;
	struct delayed_work	exit_work;
	struct work_struct	partition_scan_work;

	bool			block_open; /* protected by open_mutex */

	/* shared memory zero copy */
	struct maple_tree	buf_tree;
	struct ida		buf_ida;

	struct ublk_queue       *queues[];
};
/* …（同文件无关代码省略）… */
static inline bool ublk_dev_support_batch_io(const struct ublk_device *ub)
{
	return ub->dev_info.flags & UBLK_F_BATCH_IO;
}
/* …（同文件无关代码省略）… */
static inline bool ublk_support_batch_io(const struct ublk_queue *ubq)
{
	return ubq->flags & UBLK_F_BATCH_IO;
}
/* …（同文件无关代码省略）… */
#define UBLK_MAX_UBLKS UBLK_MINORS
/* …（同文件无关代码省略）… */
 * by cgroup.
 */
static unsigned int unprivileged_ublks_max = 64;
static unsigned int unprivileged_ublks_added; /* protected by ublk_ctl_mutex */

static struct miscdevice ublk_misc;

/* …（同文件无关代码省略）… */
static noinline struct ublk_device *ublk_get_device(struct ublk_device *ub)
{
	if (kobject_get_unless_zero(&ub->cdev_dev.kobj))
		return ub;
	return NULL;
}
/* …（同文件无关代码省略）… */
static noinline void ublk_put_device(struct ublk_device *ub)
{
	put_device(&ub->cdev_dev);
}
/* …（同文件无关代码省略）… */
static inline struct ublk_queue *ublk_get_queue(struct ublk_device *dev,
		u16 qid)
{
	return dev->queues[qid];
}
/* …（同文件无关代码省略）… */
static void ublk_cancel_cmd(struct ublk_queue *ubq, u16 tag,
		unsigned int issue_flags)
{
	struct ublk_io *io = &ubq->ios[tag];
	struct ublk_device *ub = ubq->dev;
	struct io_uring_cmd *cmd = NULL;
	struct request *req;
	bool done;

	if (!(io->flags & UBLK_IO_FLAG_ACTIVE))
		return;

	/*
	 * Don't try to cancel this command if the request is started for
	 * avoiding race between io_uring_cmd_done() and
	 * io_uring_cmd_complete_in_task().
	 *
	 * Either the started request will be aborted via __ublk_abort_rq(),
	 * then this uring_cmd is canceled next time, or it will be done in
	 * task work function ublk_dispatch_req() because io_uring guarantees
	 * that ublk_dispatch_req() is always called
	 */
	req = blk_mq_tag_to_rq(ub->tag_set.tags[ubq->q_id], tag);
	if (req && blk_mq_request_started(req) && req->tag == tag)
		return;

	spin_lock(&ubq->cancel_lock);
	done = !!(io->flags & UBLK_IO_FLAG_CANCELED);
	if (!done) {
		io->flags |= UBLK_IO_FLAG_CANCELED;
		/* dependency ordered against smp_wmb() in ublk_prep_cancel() */
		cmd = READ_ONCE(io->cmd);
		io->cmd = NULL;
	}
	spin_unlock(&ubq->cancel_lock);

	if (!done && cmd)
		io_uring_cmd_done(cmd, UBLK_IO_RES_ABORT, issue_flags);
}
/* …（同文件无关代码省略）… */
static void ublk_batch_cancel_cmd(struct ublk_queue *ubq,
				  struct ublk_batch_fetch_cmd *fcmd,
				  unsigned int issue_flags)
{
	bool done;

	spin_lock(&ubq->evts_lock);
	done = (READ_ONCE(ubq->active_fcmd) != fcmd) && !list_empty(&fcmd->node);
	if (done)
		list_del_init(&fcmd->node);
	spin_unlock(&ubq->evts_lock);

	if (done) {
		io_uring_cmd_done(fcmd->cmd, UBLK_IO_RES_ABORT, issue_flags);
		ublk_batch_free_fcmd(fcmd);
	}
}
/* …（同文件无关代码省略）… */
static void ublk_batch_cancel_queue(struct ublk_queue *ubq)
{
	struct ublk_batch_fetch_cmd *fcmd;
	LIST_HEAD(fcmd_list);

	spin_lock(&ubq->evts_lock);
	ubq->force_abort = true;
	list_splice_init(&ubq->fcmd_head, &fcmd_list);
	fcmd = READ_ONCE(ubq->active_fcmd);
	if (fcmd)
		list_move(&fcmd->node, &ubq->fcmd_head);
	spin_unlock(&ubq->evts_lock);

	while (!list_empty(&fcmd_list)) {
		fcmd = list_first_entry(&fcmd_list,
				struct ublk_batch_fetch_cmd, node);
		ublk_batch_cancel_cmd(ubq, fcmd, IO_URING_F_UNLOCKED);
	}
}
/* …（同文件无关代码省略）… */
static void ublk_cancel_queue(struct ublk_queue *ubq)
{
	u16 i;

	if (ublk_support_batch_io(ubq)) {
		ublk_batch_cancel_queue(ubq);
		return;
	}

	for (i = 0; i < ubq->q_depth; i++)
		ublk_cancel_cmd(ubq, i, IO_URING_F_UNLOCKED);
}
/* …（同文件无关代码省略）… */
static void ublk_cancel_dev(struct ublk_device *ub)
{
	u16 i;

	for (i = 0; i < ub->dev_info.nr_hw_queues; i++)
		ublk_cancel_queue(ublk_get_queue(ub, i));
}
/* …（同文件无关代码省略）… */
static struct file *ublk_get_ch_file(struct ublk_device *ub)
{
	struct file *file;

	mutex_lock(&ub->cancel_mutex);
	file = ub->ch_file;
	if (file && !file_ref_get(&file->f_ref))
		file = NULL;
	mutex_unlock(&ub->cancel_mutex);
	return file;
}
/* …（同文件无关代码省略）… */
static void ublk_stop_dev(struct ublk_device *ub)
{
	struct file *file;

	/*
	 * FETCH, PREP and START_DEV take ub->mutex. If a server has
	 * /dev/ublkcN open, set STOPPING, which turns it away, and hold a
	 * reference on the file: the server's release, whose reset clears
	 * STOPPING and lets a new server attach, can't run before we drop it.
	 * So the cancel can run after the unlock and only meets this server's
	 * commands; it has to: io_uring_cmd_done() may take uring_lock, under
	 * which FETCH takes ub->mutex.
	 */
	mutex_lock(&ub->mutex);
	ublk_stop_dev_unlocked(ub);
	cancel_work_sync(&ub->partition_scan_work);
	file = ublk_get_ch_file(ub);
	if (file) {
		/*
		 * The server has to close /dev/ublkcN before this device can
		 * be started again: the reset in its release clears STOPPING.
		 */
		set_bit(UB_STATE_STOPPING, &ub->state);
		/* for wake_up_var() below, see wake_up_bit() */
		smp_mb__after_atomic();
	}
	mutex_unlock(&ub->mutex);

	/* no server: nothing to cancel */
	if (!file)
		return;

	/* wake a START_DEV waiting for the device to get ready */
	wake_up_var(&ub->nr_queue_ready);
	ublk_cancel_dev(ub);
	__fput_sync(file);
}
/* …（同文件无关代码省略）… */
static void __ublk_deinit_queue(struct ublk_device *ub, struct ublk_queue *ubq)
{
	size_t size;
	u16 i;

	size = ublk_queue_cmd_buf_size(ub);

	for (i = 0; i < ubq->q_depth; i++) {
		struct ublk_io *io = &ubq->ios[i];
		if (io->task)
			put_task_struct(io->task);
		WARN_ON_ONCE(refcount_read(&io->ref));
		WARN_ON_ONCE(io->task_registered_buffers);
	}

	if (ubq->io_cmd_buf)
		free_pages((unsigned long)ubq->io_cmd_buf, get_order(size));

	if (ublk_dev_support_batch_io(ub))
		ublk_io_evts_deinit(ubq);

	kvfree(ubq);
}
/* …（同文件无关代码省略）… */
static void ublk_deinit_queue(struct ublk_device *ub, u16 q_id)
{
	struct ublk_queue *ubq = ub->queues[q_id];

	if (!ubq)
		return;

	__ublk_deinit_queue(ub, ubq);
	ub->queues[q_id] = NULL;
}
/* …（同文件无关代码省略）… */
static void ublk_deinit_queues(struct ublk_device *ub)
{
	u16 i;

	for (i = 0; i < ub->dev_info.nr_hw_queues; i++)
		ublk_deinit_queue(ub, i);
}
/* …（同文件无关代码省略）… */
	int i = idx;
	int err;

	spin_lock(&ublk_idr_lock);
	/* allocate id, if @id >= 0, we're requesting that specific id */
	if (i >= 0) {
		err = idr_alloc(&ublk_index_idr, ub, i, i + 1, GFP_NOWAIT);
		if (err == -ENOSPC)
			err = -EEXIST;
	} else {
		err = idr_alloc(&ublk_index_idr, ub, 0, UBLK_MAX_UBLKS,
				GFP_NOWAIT);
	}
	spin_unlock(&ublk_idr_lock);
/* …（同文件无关代码省略）… */
	return err;
}

static void ublk_free_dev_number(struct ublk_device *ub)
{
	spin_lock(&ublk_idr_lock);
/* …（同文件无关代码省略）… */
static void ublk_cdev_rel(struct device *dev)
{
	struct ublk_device *ub = container_of(dev, struct ublk_device, cdev_dev);

	ublk_buf_cleanup(ub);
	blk_mq_free_tag_set(&ub->tag_set);
	ublk_deinit_queues(ub);
	ublk_free_dev_number(ub);
	mutex_destroy(&ub->mutex);
	mutex_destroy(&ub->cancel_mutex);
	kfree(ub);
}
/* …（同文件无关代码省略）… */
static int ublk_add_chdev(struct ublk_device *ub)
{
	struct device *dev = &ub->cdev_dev;
	int minor = ub->ub_number;
	int ret;

	dev->parent = ublk_misc.this_device;
	dev->devt = MKDEV(MAJOR(ublk_chr_devt), minor);
	dev->class = &ublk_chr_class;
	dev->release = ublk_cdev_rel;
	device_initialize(dev);

	ret = dev_set_name(dev, "ublkc%d", minor);
	if (ret)
		goto fail;

	if (ublk_dev_support_batch_io(ub))
		cdev_init(&ub->cdev, &ublk_ch_batch_io_fops);
	else
		cdev_init(&ub->cdev, &ublk_ch_fops);
	ret = cdev_device_add(&ub->cdev, dev);
	if (ret)
		goto fail;

	if (ub->dev_info.flags & UBLK_F_UNPRIVILEGED_DEV)
		unprivileged_ublks_added++;
	return 0;
 fail:
	put_device(dev);
	return ret;
}
/* …（同文件无关代码省略）… */

static void ublk_remove(struct ublk_device *ub)
{
	bool unprivileged;

	ublk_stop_dev(ub);
	cdev_device_del(&ub->cdev, &ub->cdev_dev);
	unprivileged = ub->dev_info.flags & UBLK_F_UNPRIVILEGED_DEV;
	ublk_put_device(ub);

	if (unprivileged)
		unprivileged_ublks_added--;
}

static struct ublk_device *ublk_get_device_from_id(int idx)
{
	struct ublk_device *ub = NULL;

	if (idx < 0)
		return NULL;

	spin_lock(&ublk_idr_lock);
	ub = idr_find(&ublk_index_idr, idx);
	if (ub)
		ub = ublk_get_device(ub);
	spin_unlock(&ublk_idr_lock);

	return ub;
}
/* …（同文件无关代码省略）… */

	ret = -EACCES;
	if ((info.flags & UBLK_F_UNPRIVILEGED_DEV) &&
	    unprivileged_ublks_added >= unprivileged_ublks_max)
		goto out_unlock;

	ret = -ENOMEM;
/* …（同文件无关代码省略）… */
	 * ublk_add_chdev() will cleanup everything if it fails.
	 */
	ret = ublk_add_chdev(ub);
	goto out_unlock;

out_deinit_queues:
/* …（同文件无关代码省略）… */
{
	struct ublk_device *ub = *p_ub;
	int idx = ub->ub_number;
	int ret;

	ret = mutex_lock_killable(&ublk_ctl_mutex);
	if (ret)
		return ret;

	if (!test_bit(UB_STATE_DELETED, &ub->state)) {
		ublk_remove(ub);
		set_bit(UB_STATE_DELETED, &ub->state);
	}

	mutex_unlock(&ublk_ctl_mutex);

	/*
	 * Drop the reference outside ublk_ctl_mutex: if it is the last one,
	 * the release frees the tag set, which waits for an SRCU grace period,
	 * and holding the mutex would serialize concurrent deletions on it.
	 */
	*p_ub = NULL;
	ublk_put_device(ub);
/* …（同文件无关代码省略）… */
static bool __ublk_shmem_remove_ranges(struct ublk_device *ub,
					int buf_index, int *ret)
{
	MA_STATE(mas, &ub->buf_tree, 0, ULONG_MAX);
	struct ublk_buf_range *range;
	struct ublk_unpin_range to_unpin[UBLK_REMOVE_BATCH];
	unsigned int count = 0;
	unsigned int i;
	bool done = false;

	mas_lock(&mas);
	mas_for_each(&mas, range, ULONG_MAX) {
		if (buf_index >= 0 && range->buf_index != buf_index)
			continue;

		*ret = 0;
		to_unpin[count].base_pfn = mas.index;
		to_unpin[count].nr_pages = mas.last - mas.index + 1;
		mas_erase(&mas);
		kfree(range);
		if (++count >= UBLK_REMOVE_BATCH)
			goto unlock;
	}
	done = true;
unlock:
	mas_unlock(&mas);

	for (i = 0; i < count; i++)
		ublk_unpin_range_pages(to_unpin[i].base_pfn,
				       to_unpin[i].nr_pages);

	return done;
}
/* …（同文件无关代码省略）… */
static int ublk_shmem_remove_ranges(struct ublk_device *ub, int buf_index)
{
	int ret = -ENOENT;

	while (!__ublk_shmem_remove_ranges(ub, buf_index, &ret))
		cond_resched();
	return ret;
}
/* …（同文件无关代码省略）… */
static void ublk_buf_cleanup(struct ublk_device *ub)
{
	ublk_shmem_remove_ranges(ub, -1);
	mtree_destroy(&ub->buf_tree);
	ida_destroy(&ub->buf_ida);
}
