// AUTO-DRAFT from torvalds/linux PR #8a36f53fd5ec3eff01f1b637d42cbdc3821d0493
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define SG_MAX_CDB_SIZE 252
/* …（同文件无关代码省略）… */
#define SG_DEFAULT_TIMEOUT mult_frac(SG_DEFAULT_TIMEOUT_USER, HZ, USER_HZ)
/* …（同文件无关代码省略）… */
#define SG_SECTOR_SZ 512
/* …（同文件无关代码省略）… */
typedef struct sg_scatter_hold { /* holding area for scsi scatter gather info */
	unsigned short k_use_sg; /* Count of kernel scatter-gather pieces */
	unsigned sglist_len; /* size of malloc'd scatter-gather list ++ */
	unsigned bufflen;	/* Size of (aggregate) data buffer */
	struct page **pages;
	int page_order;
	char dio_in_use;	/* 0->indirect IO (or mmap), 1->dio */
	unsigned char cmd_opcode; /* first byte of command */
} Sg_scatter_hold;
/* …（同文件无关代码省略）… */
typedef struct sg_request {	/* SG_MAX_QUEUE requests outstanding per file */
	struct list_head entry;	/* list entry */
	struct sg_fd *parentfp;	/* NULL -> not in use */
	Sg_scatter_hold data;	/* hold buffer, perhaps scatter list */
	sg_io_hdr_t header;	/* scsi command+info, see <scsi/sg.h> */
	unsigned char sense_b[SCSI_SENSE_BUFFERSIZE];
	char res_used;		/* 1 -> using reserve buffer, 0 -> not ... */
	char orphan;		/* 1 -> drop on sight, 0 -> normal */
	char sg_io_owned;	/* 1 -> packet belongs to SG_IO */
	/* done protected by rq_list_lock */
	char done;		/* 0->before bh, 1->before read, 2->read */
	struct request *rq;
	struct bio *bio;
	struct execute_work ew;
} Sg_request;
/* …（同文件无关代码省略）… */
typedef struct sg_fd {		/* holds the state of a file descriptor */
	struct list_head sfd_siblings;  /* protected by device's sfd_lock */
	struct sg_device *parentdp;	/* owning device */
	wait_queue_head_t read_wait;	/* queue read until command done */
	rwlock_t rq_list_lock;	/* protect access to list in req_arr */
	struct mutex f_mutex;	/* protect against changes in this fd */
	int timeout;		/* defaults to SG_DEFAULT_TIMEOUT      */
	int timeout_user;	/* defaults to SG_DEFAULT_TIMEOUT_USER */
	Sg_scatter_hold reserve;	/* buffer held for this file descriptor */
	struct list_head rq_list; /* head of request list */
	struct fasync_struct *async_qp;	/* used by asynchronous notification */
	Sg_request req_arr[SG_MAX_QUEUE];	/* used as singly-linked list */
	char force_packid;	/* 1 -> pack_id input to read(), 0 -> ignored */
	char cmd_q;		/* 1 -> allow command queuing, 0 -> don't */
	unsigned char next_cmd_len; /* 0: automatic, >0: use on next write() */
	char keep_orphan;	/* 0 -> drop orphan (def), 1 -> keep for read() */
	char mmap_called;	/* 0 -> mmap() never called on this fd */
	char res_in_use;	/* 1 -> 'reserve' array in use */
	struct kref f_ref;
	struct execute_work ew;
} Sg_fd;
/* …（同文件无关代码省略）… */
typedef struct sg_device { /* holds the state of each scsi generic device */
	struct scsi_device *device;
	wait_queue_head_t open_wait;    /* queue open() when O_EXCL present */
	struct mutex open_rel_lock;     /* held when in open() or release() */
	int sg_tablesize;	/* adapter's max scatter-gather table size */
	u32 index;		/* device index number */
	struct list_head sfds;
	rwlock_t sfd_lock;      /* protect access to sfd list */
	atomic_t detaching;     /* 0->device usable, 1->device detaching */
	bool exclude;		/* 1->open(O_EXCL) succeeded and is active */
	int open_cnt;		/* count of opens (perhaps < num(sfds) ) */
	char sgdebug;		/* 0->off, 1->sense, 9->dump dev, 10-> all devs */
	char name[DISK_NAME_LEN];
	struct cdev * cdev;	/* char_dev [sysfs: /sys/cdev/major/sg<n>] */
	struct kref d_ref;
} Sg_device;
/* …（同文件无关代码省略）… */
#define SZ_SG_IO_HDR sizeof(sg_io_hdr_t)
/* …（同文件无关代码省略）… */
#define SZ_SG_REQ_INFO sizeof(sg_req_info_t)
/* …（同文件无关代码省略）… */
#define sg_printk(prefix, sdp, fmt, a...) \
	sdev_prefix_printk(prefix, (sdp)->device, (sdp)->name, fmt, ##a)
/* …（同文件无关代码省略）… */
static int sg_allow_access(struct file *filp, unsigned char *cmd)
{
	struct sg_fd *sfp = filp->private_data;

	if (sfp->parentdp->device->type == TYPE_SCANNER)
		return 0;
	if (!scsi_cmd_allowed(cmd, filp->f_mode & FMODE_WRITE))
		return -EPERM;
	return 0;
}
/* …（同文件无关代码省略）… */
static int
open_wait(Sg_device *sdp, int flags)
{
	int retval = 0;

	if (flags & O_EXCL) {
		while (sdp->open_cnt > 0) {
			mutex_unlock(&sdp->open_rel_lock);
			retval = wait_event_interruptible(sdp->open_wait,
					(atomic_read(&sdp->detaching) ||
					 !sdp->open_cnt));
			mutex_lock(&sdp->open_rel_lock);

			if (retval) /* -ERESTARTSYS */
				return retval;
			if (atomic_read(&sdp->detaching))
				return -ENODEV;
		}
	} else {
		while (sdp->exclude) {
			mutex_unlock(&sdp->open_rel_lock);
			retval = wait_event_interruptible(sdp->open_wait,
					(atomic_read(&sdp->detaching) ||
					 !sdp->exclude));
			mutex_lock(&sdp->open_rel_lock);

			if (retval) /* -ERESTARTSYS */
				return retval;
			if (atomic_read(&sdp->detaching))
				return -ENODEV;
		}
	}

	return retval;
}
/* …（同文件无关代码省略）… */
static int
sg_open(struct inode *inode, struct file *filp)
{
	int dev = iminor(inode);
	int flags = filp->f_flags;
	struct request_queue *q;
	struct scsi_device *device;
	Sg_device *sdp;
	Sg_fd *sfp;
	int retval;

	nonseekable_open(inode, filp);
	if ((flags & O_EXCL) && (O_RDONLY == (flags & O_ACCMODE)))
		return -EPERM; /* Can't lock it with read only access */
	sdp = sg_get_dev(dev);
	if (IS_ERR(sdp))
		return PTR_ERR(sdp);

	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sdp,
				      "sg_open: flags=0x%x\n", flags));

	/* This driver's module count bumped by fops_get in <linux/fs.h> */
	/* Prevent the device driver from vanishing while we sleep */
	device = sdp->device;
	retval = scsi_device_get(device);
	if (retval)
		goto sg_put;

	/* scsi_block_when_processing_errors() may block so bypass
	 * check if O_NONBLOCK. Permits SCSI commands to be issued
	 * during error recovery. Tread carefully. */
	if (!((flags & O_NONBLOCK) ||
	      scsi_block_when_processing_errors(device))) {
		retval = -ENXIO;
		/* we are in error recovery for this device */
		goto sdp_put;
	}

	mutex_lock(&sdp->open_rel_lock);
	if (flags & O_NONBLOCK) {
		if (flags & O_EXCL) {
			if (sdp->open_cnt > 0) {
				retval = -EBUSY;
				goto error_mutex_locked;
			}
		} else {
			if (sdp->exclude) {
				retval = -EBUSY;
				goto error_mutex_locked;
			}
		}
	} else {
		retval = open_wait(sdp, flags);
		if (retval) /* -ERESTARTSYS or -ENODEV */
			goto error_mutex_locked;
	}

	/* N.B. at this point we are holding the open_rel_lock */
	if (flags & O_EXCL)
		sdp->exclude = true;

	if (sdp->open_cnt < 1) {  /* no existing opens */
		sdp->sgdebug = 0;
		q = device->request_queue;
		sdp->sg_tablesize = queue_max_segments(q);
	}
	sfp = sg_add_sfp(sdp);
	if (IS_ERR(sfp)) {
		retval = PTR_ERR(sfp);
		goto out_undo;
	}

	filp->private_data = sfp;
	sdp->open_cnt++;
	mutex_unlock(&sdp->open_rel_lock);

	retval = 0;
sg_put:
	kref_put(&sdp->d_ref, sg_device_destroy);
	return retval;

out_undo:
	if (flags & O_EXCL) {
		sdp->exclude = false;   /* undo if error */
		wake_up_interruptible(&sdp->open_wait);
	}
error_mutex_locked:
	mutex_unlock(&sdp->open_rel_lock);
sdp_put:
	kref_put(&sdp->d_ref, sg_device_destroy);
	scsi_device_put(device);
	return retval;
}
/* …（同文件无关代码省略）… */
static int
sg_release(struct inode *inode, struct file *filp)
{
	Sg_device *sdp;
	Sg_fd *sfp;

	if ((!(sfp = (Sg_fd *) filp->private_data)) || (!(sdp = sfp->parentdp)))
		return -ENXIO;
	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sdp, "sg_release\n"));

	mutex_lock(&sdp->open_rel_lock);
	sdp->open_cnt--;

	/* possibly many open()s waiting on exlude clearing, start many;
	 * only open(O_EXCL)s wait on 0==open_cnt so only start one */
	if (sdp->exclude) {
		sdp->exclude = false;
		wake_up_interruptible_all(&sdp->open_wait);
	} else if (0 == sdp->open_cnt) {
		wake_up_interruptible(&sdp->open_wait);
	}
	mutex_unlock(&sdp->open_rel_lock);
	kref_put(&sfp->f_ref, sg_remove_sfp);
	return 0;
}
/* …（同文件无关代码省略）… */
static ssize_t
sg_new_read(Sg_fd * sfp, char __user *buf, size_t count, Sg_request * srp)
{
	sg_io_hdr_t *hp = &srp->header;
	int err = 0, err2;
	int len;

	if (in_compat_syscall()) {
		if (count < sizeof(struct compat_sg_io_hdr)) {
			err = -EINVAL;
			goto err_out;
		}
	} else if (count < SZ_SG_IO_HDR) {
		err = -EINVAL;
		goto err_out;
	}
	hp->sb_len_wr = 0;
	if ((hp->mx_sb_len > 0) && hp->sbp) {
		if ((CHECK_CONDITION & hp->masked_status) ||
		    (srp->sense_b[0] & 0x70) == 0x70) {
			int sb_len = SCSI_SENSE_BUFFERSIZE;
			sb_len = (hp->mx_sb_len > sb_len) ? sb_len : hp->mx_sb_len;
			len = 8 + (int) srp->sense_b[7];	/* Additional sense length field */
			len = (len > sb_len) ? sb_len : len;
			if (copy_to_user(hp->sbp, srp->sense_b, len)) {
				err = -EFAULT;
				goto err_out;
			}
			hp->driver_status = DRIVER_SENSE;
			hp->sb_len_wr = len;
		}
	}
	if (hp->masked_status || hp->host_status || hp->driver_status)
		hp->info |= SG_INFO_CHECK;
	err = put_sg_io_hdr(hp, buf);
err_out:
	err2 = sg_finish_rem_req(srp);
	sg_remove_request(sfp, srp);
	return err ? : err2 ? : count;
}
/* …（同文件无关代码省略）… */
static ssize_t
sg_new_write(Sg_fd *sfp, struct file *file, const char __user *buf,
		 size_t count, int blocking, int read_only, int sg_io_owned,
		 Sg_request **o_srp)
{
	int k;
	Sg_request *srp;
	sg_io_hdr_t *hp;
	unsigned char cmnd[SG_MAX_CDB_SIZE];
	int timeout;
	unsigned long ul_timeout;

	if (count < SZ_SG_IO_HDR)
		return -EINVAL;

	sfp->cmd_q = 1;	/* when sg_io_hdr seen, set command queuing on */
	if (!(srp = sg_add_request(sfp))) {
		SCSI_LOG_TIMEOUT(1, sg_printk(KERN_INFO, sfp->parentdp,
					      "sg_new_write: queue full\n"));
		return -EDOM;
	}
	srp->sg_io_owned = sg_io_owned;
	hp = &srp->header;
	if (get_sg_io_hdr(hp, buf)) {
		sg_remove_request(sfp, srp);
		return -EFAULT;
	}
	hp->duration = jiffies_to_msecs(jiffies);

	if (hp->interface_id != 'S') {
		sg_remove_request(sfp, srp);
		return -ENOSYS;
	}
	if (hp->flags & SG_FLAG_MMAP_IO) {
		if (hp->dxfer_len > sfp->reserve.bufflen) {
			sg_remove_request(sfp, srp);
			return -ENOMEM;	/* MMAP_IO size must fit in reserve buffer */
		}
		if (hp->flags & SG_FLAG_DIRECT_IO) {
			sg_remove_request(sfp, srp);
			return -EINVAL;	/* either MMAP_IO or DIRECT_IO (not both) */
		}
		if (sfp->res_in_use) {
			sg_remove_request(sfp, srp);
			return -EBUSY;	/* reserve buffer already being used */
		}
	}
	ul_timeout = msecs_to_jiffies(srp->header.timeout);
	timeout = (ul_timeout < INT_MAX) ? ul_timeout : INT_MAX;
	if ((!hp->cmdp) || (hp->cmd_len < 6) || (hp->cmd_len > sizeof (cmnd))) {
		sg_remove_request(sfp, srp);
		return -EMSGSIZE;
	}
	if (copy_from_user(cmnd, hp->cmdp, hp->cmd_len)) {
		sg_remove_request(sfp, srp);
		return -EFAULT;
	}
	if (read_only && sg_allow_access(file, cmnd)) {
		sg_remove_request(sfp, srp);
		return -EPERM;
	}
	k = sg_common_write(sfp, srp, cmnd, timeout, blocking);
	if (k < 0)
		return k;
	if (o_srp)
		*o_srp = srp;
	return count;
}
/* …（同文件无关代码省略）… */
static int
sg_common_write(Sg_fd * sfp, Sg_request * srp,
		unsigned char *cmnd, int timeout, int blocking)
{
	int k, at_head;
	Sg_device *sdp = sfp->parentdp;
	sg_io_hdr_t *hp = &srp->header;

	srp->data.cmd_opcode = cmnd[0];	/* hold opcode of command */
	hp->status = 0;
	hp->masked_status = 0;
	hp->msg_status = 0;
	hp->info = 0;
	hp->host_status = 0;
	hp->driver_status = 0;
	hp->resid = 0;
	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, sfp->parentdp,
			"sg_common_write:  scsi opcode=0x%02x, cmd_size=%d\n",
			(int) cmnd[0], (int) hp->cmd_len));

	if (hp->dxfer_len >= SZ_256M) {
		sg_remove_request(sfp, srp);
		return -EINVAL;
	}

	k = sg_start_req(srp, cmnd);
	if (k) {
		SCSI_LOG_TIMEOUT(1, sg_printk(KERN_INFO, sfp->parentdp,
			"sg_common_write: start_req err=%d\n", k));
		sg_finish_rem_req(srp);
		sg_remove_request(sfp, srp);
		return k;	/* probably out of space --> ENOMEM */
	}
	if (atomic_read(&sdp->detaching)) {
		if (srp->bio) {
			blk_mq_free_request(srp->rq);
			srp->rq = NULL;
		}

		sg_finish_rem_req(srp);
		sg_remove_request(sfp, srp);
		return -ENODEV;
	}

	if (hp->interface_id != '\0' &&	/* v3 (or later) interface */
	    (SG_FLAG_Q_AT_TAIL & hp->flags))
		at_head = 0;
	else
		at_head = 1;

	srp->rq->timeout = timeout;
	kref_get(&sfp->f_ref); /* sg_rq_end_io() does kref_put(). */
	srp->rq->end_io = sg_rq_end_io;
	blk_execute_rq_nowait(srp->rq, at_head);
	return 0;
}
/* …（同文件无关代码省略）… */
static int srp_done(Sg_fd *sfp, Sg_request *srp)
{
	unsigned long flags;
	int ret;

	read_lock_irqsave(&sfp->rq_list_lock, flags);
	ret = srp->done;
	read_unlock_irqrestore(&sfp->rq_list_lock, flags);
	return ret;
}
/* …（同文件无关代码省略）… */
static int max_sectors_bytes(struct request_queue *q)
{
	unsigned int max_sectors = queue_max_sectors(q);

	max_sectors = min_t(unsigned int, max_sectors, INT_MAX >> 9);

	return max_sectors << 9;
}
/* …（同文件无关代码省略）… */
static void
sg_fill_request_table(Sg_fd *sfp, sg_req_info_t *rinfo)
{
	Sg_request *srp;
	int val;
	unsigned int ms;

	val = 0;
	list_for_each_entry(srp, &sfp->rq_list, entry) {
		if (val >= SG_MAX_QUEUE)
			break;
		rinfo[val].req_state = srp->done + 1;
		rinfo[val].problem = srp->header.masked_status ||
					     srp->header.host_status ||
					     srp->header.driver_status;
		if (srp->done)
			rinfo[val].duration =
				srp->header.duration;
		else {
			ms = jiffies_to_msecs(jiffies);
			rinfo[val].duration =
				(ms > srp->header.duration) ?
				(ms - srp->header.duration) : 0;
		}
		rinfo[val].orphan = srp->orphan;
		rinfo[val].sg_io_owned = srp->sg_io_owned;
		rinfo[val].pack_id = srp->header.pack_id;
		rinfo[val].usr_ptr = srp->header.usr_ptr;
		val++;
	}
}
/* …（同文件无关代码省略）… */
struct compat_sg_req_info { /* used by SG_GET_REQUEST_TABLE ioctl() */
	char req_state;
	char orphan;
	char sg_io_owned;
	char problem;
	int pack_id;
	compat_uptr_t usr_ptr;
	unsigned int duration;
	int unused;
};
/* …（同文件无关代码省略）… */
static int put_compat_request_table(struct compat_sg_req_info __user *o,
				    struct sg_req_info *rinfo)
{
	int i;
	for (i = 0; i < SG_MAX_QUEUE; i++) {
		if (copy_to_user(o + i, rinfo + i, offsetof(sg_req_info_t, usr_ptr)) ||
		    put_user((uintptr_t)rinfo[i].usr_ptr, &o[i].usr_ptr) ||
		    put_user(rinfo[i].duration, &o[i].duration) ||
		    put_user(rinfo[i].unused, &o[i].unused))
			return -EFAULT;
	}
	return 0;
}
/* …（同文件无关代码省略）… */
static long
sg_ioctl_common(struct file *filp, Sg_device *sdp, Sg_fd *sfp,
		unsigned int cmd_in, void __user *p)
{
	int __user *ip = p;
	int result, val, read_only;
	Sg_request *srp;
	unsigned long iflags;

	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sdp,
				   "sg_ioctl: cmd=0x%x\n", (int) cmd_in));
	read_only = (O_RDWR != (filp->f_flags & O_ACCMODE));

	switch (cmd_in) {
	case SG_IO:
		if (atomic_read(&sdp->detaching))
			return -ENODEV;
		if (!scsi_block_when_processing_errors(sdp->device))
			return -ENXIO;
		result = sg_new_write(sfp, filp, p, SZ_SG_IO_HDR,
				 1, read_only, 1, &srp);
		if (result < 0)
			return result;
		result = wait_event_interruptible(sfp->read_wait,
			srp_done(sfp, srp));
		write_lock_irq(&sfp->rq_list_lock);
		if (srp->done) {
			srp->done = 2;
			write_unlock_irq(&sfp->rq_list_lock);
			result = sg_new_read(sfp, p, SZ_SG_IO_HDR, srp);
			return (result < 0) ? result : 0;
		}
		srp->orphan = 1;
		write_unlock_irq(&sfp->rq_list_lock);
		return result;	/* -ERESTARTSYS because signal hit process */
	case SG_SET_TIMEOUT:
		result = get_user(val, ip);
		if (result)
			return result;
		if (val < 0)
			return -EIO;
		if (val >= mult_frac((s64)INT_MAX, USER_HZ, HZ))
			val = min_t(s64, mult_frac((s64)INT_MAX, USER_HZ, HZ),
				    INT_MAX);
		sfp->timeout_user = val;
		sfp->timeout = mult_frac(val, HZ, USER_HZ);

		return 0;
	case SG_GET_TIMEOUT:	/* N.B. User receives timeout as return value */
				/* strange ..., for backward compatibility */
		return sfp->timeout_user;
	case SG_SET_FORCE_LOW_DMA:
		/*
		 * N.B. This ioctl never worked properly, but failed to
		 * return an error value. So returning '0' to keep compability
		 * with legacy applications.
		 */
		return 0;
	case SG_GET_LOW_DMA:
		return put_user(0, ip);
	case SG_GET_SCSI_ID:
		{
			sg_scsi_id_t v;

			if (atomic_read(&sdp->detaching))
				return -ENODEV;
			memset(&v, 0, sizeof(v));
			v.host_no = sdp->device->host->host_no;
			v.channel = sdp->device->channel;
			v.scsi_id = sdp->device->id;
			v.lun = sdp->device->lun;
			v.scsi_type = sdp->device->type;
			v.h_cmd_per_lun = sdp->device->host->cmd_per_lun;
			v.d_queue_depth = sdp->device->queue_depth;
			if (copy_to_user(p, &v, sizeof(sg_scsi_id_t)))
				return -EFAULT;
			return 0;
		}
	case SG_SET_FORCE_PACK_ID:
		result = get_user(val, ip);
		if (result)
			return result;
		sfp->force_packid = val ? 1 : 0;
		return 0;
	case SG_GET_PACK_ID:
		read_lock_irqsave(&sfp->rq_list_lock, iflags);
		list_for_each_entry(srp, &sfp->rq_list, entry) {
			if ((1 == srp->done) && (!srp->sg_io_owned)) {
				read_unlock_irqrestore(&sfp->rq_list_lock,
						       iflags);
				return put_user(srp->header.pack_id, ip);
			}
		}
		read_unlock_irqrestore(&sfp->rq_list_lock, iflags);
		return put_user(-1, ip);
	case SG_GET_NUM_WAITING:
		read_lock_irqsave(&sfp->rq_list_lock, iflags);
		val = 0;
		list_for_each_entry(srp, &sfp->rq_list, entry) {
			if ((1 == srp->done) && (!srp->sg_io_owned))
				++val;
		}
		read_unlock_irqrestore(&sfp->rq_list_lock, iflags);
		return put_user(val, ip);
	case SG_GET_SG_TABLESIZE:
		return put_user(sdp->sg_tablesize, ip);
	case SG_SET_RESERVED_SIZE:
		result = get_user(val, ip);
		if (result)
			return result;
                if (val < 0)
                        return -EINVAL;
		val = min_t(int, val,
			    max_sectors_bytes(sdp->device->request_queue));
		mutex_lock(&sfp->f_mutex);
		if (val != sfp->reserve.bufflen) {
			if (sfp->mmap_called ||
			    sfp->res_in_use) {
				mutex_unlock(&sfp->f_mutex);
				return -EBUSY;
			}

			sg_remove_scat(sfp, &sfp->reserve);
			sg_build_reserve(sfp, val);
		}
		mutex_unlock(&sfp->f_mutex);
		return 0;
	case SG_GET_RESERVED_SIZE:
		val = min_t(int, sfp->reserve.bufflen,
			    max_sectors_bytes(sdp->device->request_queue));
		return put_user(val, ip);
	case SG_SET_COMMAND_Q:
		result = get_user(val, ip);
		if (result)
			return result;
		sfp->cmd_q = val ? 1 : 0;
		return 0;
	case SG_GET_COMMAND_Q:
		return put_user((int) sfp->cmd_q, ip);
	case SG_SET_KEEP_ORPHAN:
		result = get_user(val, ip);
		if (result)
			return result;
		sfp->keep_orphan = val;
		return 0;
	case SG_GET_KEEP_ORPHAN:
		return put_user((int) sfp->keep_orphan, ip);
	case SG_NEXT_CMD_LEN:
		result = get_user(val, ip);
		if (result)
			return result;
		if (val > SG_MAX_CDB_SIZE)
			return -ENOMEM;
		sfp->next_cmd_len = (val > 0) ? val : 0;
		return 0;
	case SG_GET_VERSION_NUM:
		return put_user(sg_version_num, ip);
	case SG_GET_ACCESS_COUNT:
		/* faked - we don't have a real access count anymore */
		val = (sdp->device ? 1 : 0);
		return put_user(val, ip);
	case SG_GET_REQUEST_TABLE:
		{
			sg_req_info_t *rinfo;

			rinfo = kcalloc(SG_MAX_QUEUE, SZ_SG_REQ_INFO,
					GFP_KERNEL);
			if (!rinfo)
				return -ENOMEM;
			read_lock_irqsave(&sfp->rq_list_lock, iflags);
			sg_fill_request_table(sfp, rinfo);
			read_unlock_irqrestore(&sfp->rq_list_lock, iflags);
	#ifdef CONFIG_COMPAT
			if (in_compat_syscall())
				result = put_compat_request_table(p, rinfo);
			else
	#endif
				result = copy_to_user(p, rinfo,
						      SZ_SG_REQ_INFO * SG_MAX_QUEUE);
			result = result ? -EFAULT : 0;
			kfree(rinfo);
			return result;
		}
	case SG_EMULATED_HOST:
		if (atomic_read(&sdp->detaching))
			return -ENODEV;
		return put_user(sdp->device->host->hostt->emulated, ip);
	case SCSI_IOCTL_SEND_COMMAND:
		if (atomic_read(&sdp->detaching))
			return -ENODEV;
		return scsi_ioctl(sdp->device, filp->f_mode & FMODE_WRITE,
				  cmd_in, p);
	case SG_SET_DEBUG:
		result = get_user(val, ip);
		if (result)
			return result;
		sdp->sgdebug = (char) val;
		return 0;
	case BLKSECTGET:
		return put_user(max_sectors_bytes(sdp->device->request_queue),
				ip);
	case BLKTRACESETUP:
		return blk_trace_setup(sdp->device->request_queue, sdp->name,
				       MKDEV(SCSI_GENERIC_MAJOR, sdp->index),
				       NULL, p);
	case BLKTRACESTART:
		return blk_trace_startstop(sdp->device->request_queue, 1);
	case BLKTRACESTOP:
		return blk_trace_startstop(sdp->device->request_queue, 0);
	case BLKTRACETEARDOWN:
		return blk_trace_remove(sdp->device->request_queue);
	case SCSI_IOCTL_GET_IDLUN:
	case SCSI_IOCTL_GET_BUS_NUMBER:
	case SCSI_IOCTL_PROBE_HOST:
	case SG_GET_TRANSFORM:
	case SG_SCSI_RESET:
		if (atomic_read(&sdp->detaching))
			return -ENODEV;
		break;
	default:
		if (read_only)
			return -EPERM;	/* don't know so take safe approach */
		break;
	}

	result = scsi_ioctl_block_when_processing_errors(sdp->device,
			cmd_in, filp->f_flags & O_NDELAY);
	if (result)
		return result;

	return -ENOIOCTLCMD;
}
/* …（同文件无关代码省略）… */
static long
sg_ioctl(struct file *filp, unsigned int cmd_in, unsigned long arg)
{
	void __user *p = (void __user *)arg;
	Sg_device *sdp;
	Sg_fd *sfp;
	int ret;

	if ((!(sfp = (Sg_fd *) filp->private_data)) || (!(sdp = sfp->parentdp)))
		return -ENXIO;

	ret = sg_ioctl_common(filp, sdp, sfp, cmd_in, p);
	if (ret != -ENOIOCTLCMD)
		return ret;
	return scsi_ioctl(sdp->device, filp->f_mode & FMODE_WRITE, cmd_in, p);
}
/* …（同文件无关代码省略）… */
static int
sg_fasync(int fd, struct file *filp, int mode)
{
	Sg_device *sdp;
	Sg_fd *sfp;

	if ((!(sfp = (Sg_fd *) filp->private_data)) || (!(sdp = sfp->parentdp)))
		return -ENXIO;
	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sdp,
				      "sg_fasync: mode=%d\n", mode));

	return fasync_helper(fd, filp, mode, &sfp->async_qp);
}

static vm_fault_t
sg_vma_fault(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	Sg_fd *sfp;
	unsigned long offset, len, sa;
	Sg_scatter_hold *rsv_schp;
	int k, length;

	if ((NULL == vma) || (!(sfp = (Sg_fd *) vma->vm_private_data)))
		return VM_FAULT_SIGBUS;
	rsv_schp = &sfp->reserve;
	offset = vmf->pgoff << PAGE_SHIFT;
	if (offset >= rsv_schp->bufflen)
		return VM_FAULT_SIGBUS;
	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sfp->parentdp,
				      "sg_vma_fault: offset=%lu, scatg=%d\n",
				      offset, rsv_schp->k_use_sg));
	sa = vma->vm_start;
	length = 1 << (PAGE_SHIFT + rsv_schp->page_order);
	for (k = 0; k < rsv_schp->k_use_sg && sa < vma->vm_end; k++) {
		len = vma->vm_end - sa;
		len = (len < length) ? len : length;
		if (offset < len) {
			struct page *page = rsv_schp->pages[k] + (offset >> PAGE_SHIFT);
			get_page(page);	/* increment page count */
			vmf->page = page;
			return 0; /* success */
		}
		sa += len;
		offset -= len;
	}

	return VM_FAULT_SIGBUS;
}

static const struct vm_operations_struct sg_mmap_vm_ops = {
	.fault = sg_vma_fault,
};

static int
sg_mmap(struct file *filp, struct vm_area_struct *vma)
{
	Sg_fd *sfp;
	unsigned long req_sz, len, sa;
	Sg_scatter_hold *rsv_schp;
	int k, length;
	int ret = 0;

	if ((!filp) || (!vma) || (!(sfp = (Sg_fd *) filp->private_data)))
		return -ENXIO;
	req_sz = vma->vm_end - vma->vm_start;
	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sfp->parentdp,
				      "sg_mmap starting, vm_start=%p, len=%d\n",
				      (void *) vma->vm_start, (int) req_sz));
	if (vma->vm_pgoff)
		return -EINVAL;	/* want no offset */
	rsv_schp = &sfp->reserve;
	mutex_lock(&sfp->f_mutex);
	if (req_sz > rsv_schp->bufflen) {
		ret = -ENOMEM;	/* cannot map more than reserved buffer */
		goto out;
	}

	sa = vma->vm_start;
	length = 1 << (PAGE_SHIFT + rsv_schp->page_order);
	for (k = 0; k < rsv_schp->k_use_sg && sa < vma->vm_end; k++) {
		len = vma->vm_end - sa;
		len = (len < length) ? len : length;
		sa += len;
	}

	sfp->mmap_called = 1;
	vm_flags_set(vma, VM_IO | VM_DONTEXPAND | VM_DONTDUMP);
	vma->vm_private_data = sfp;
	vma->vm_ops = &sg_mmap_vm_ops;
out:
	mutex_unlock(&sfp->f_mutex);
	return ret;
}

static void
/* …（同文件无关代码省略）… */
static enum rq_end_io_ret
sg_rq_end_io(struct request *rq, blk_status_t status,
	     const struct io_comp_batch *iob)
{
	struct scsi_cmnd *scmd = blk_mq_rq_to_pdu(rq);
	struct sg_request *srp = rq->end_io_data;
	Sg_device *sdp;
	Sg_fd *sfp;
	unsigned long iflags;
	unsigned int ms;
	char *sense;
	int result, resid, done = 1;

	if (WARN_ON(srp->done != 0))
		return RQ_END_IO_NONE;

	sfp = srp->parentfp;
	if (WARN_ON(sfp == NULL))
		return RQ_END_IO_NONE;

	sdp = sfp->parentdp;
	if (unlikely(atomic_read(&sdp->detaching)))
		pr_info("%s: device detaching\n", __func__);

	sense = scmd->sense_buffer;
	result = scmd->result;
	resid = scmd->resid_len;

	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, sdp,
				      "sg_cmd_done: pack_id=%d, res=0x%x\n",
				      srp->header.pack_id, result));
	srp->header.resid = resid;
	if (0 != result) {
		struct scsi_sense_hdr sshdr;

		srp->header.status = 0xff & result;
		srp->header.masked_status = sg_status_byte(result);
		srp->header.msg_status = COMMAND_COMPLETE;
		srp->header.host_status = host_byte(result);
		srp->header.driver_status = driver_byte(result);
		if ((sdp->sgdebug > 0) &&
		    ((CHECK_CONDITION == srp->header.masked_status) ||
		     (COMMAND_TERMINATED == srp->header.masked_status)))
			__scsi_print_sense(sdp->device, __func__, sense,
					   SCSI_SENSE_BUFFERSIZE);

		/* Following if statement is a patch supplied by Eric Youngdale */
		if (driver_byte(result) != 0
		    && scsi_normalize_sense(sense, SCSI_SENSE_BUFFERSIZE, &sshdr)
		    && !scsi_sense_is_deferred(&sshdr)
		    && sshdr.sense_key == UNIT_ATTENTION
		    && sdp->device->removable) {
			/* Detected possible disc change. Set the bit - this */
			/* may be used if there are filesystems using this device */
			sdp->device->changed = 1;
		}
	}

	if (scmd->sense_len)
		memcpy(srp->sense_b, scmd->sense_buffer, SCSI_SENSE_BUFFERSIZE);

	/* Rely on write phase to clean out srp status values, so no "else" */

	/*
	 * Free the request as soon as it is complete so that its resources
	 * can be reused without waiting for userspace to read() the
	 * result.  But keep the associated bio (if any) around until
	 * blk_rq_unmap_user() can be called from user context.
	 */
	srp->rq = NULL;
	blk_mq_free_request(rq);

	write_lock_irqsave(&sfp->rq_list_lock, iflags);
	if (unlikely(srp->orphan)) {
		if (sfp->keep_orphan)
			srp->sg_io_owned = 0;
		else
			done = 0;
	}
	srp->done = done;
	ms = jiffies_to_msecs(jiffies);
	srp->header.duration = (ms > srp->header.duration) ?
				(ms - srp->header.duration) : 0;
	write_unlock_irqrestore(&sfp->rq_list_lock, iflags);

	if (likely(done)) {
		/* Now wake up any sg_read() that is waiting for this
		 * packet.
		 */
		wake_up_interruptible(&sfp->read_wait);
		kill_fasync(&sfp->async_qp, SIGPOLL, POLL_IN);
		kref_put(&sfp->f_ref, sg_remove_sfp);
	} else {
		INIT_WORK(&srp->ew.work, sg_rq_end_io_usercontext);
		schedule_work(&srp->ew.work);
	}
	return RQ_END_IO_NONE;
}
/* …（同文件无关代码省略）… */
	.unlocked_ioctl = sg_ioctl,
	.compat_ioctl = compat_ptr_ioctl,
	.open = sg_open,
	.mmap = sg_mmap,
	.release = sg_release,
	.fasync = sg_fasync,
};
/* …（同文件无关代码省略）… */
static void
sg_device_destroy(struct kref *kref)
{
	struct sg_device *sdp = container_of(kref, struct sg_device, d_ref);
	struct request_queue *q = sdp->device->request_queue;
	unsigned long flags;

	/* CAUTION!  Note that the device can still be found via idr_find()
	 * even though the refcount is 0.  Therefore, do idr_remove() BEFORE
	 * any other cleanup.
	 */

	blk_trace_remove(q);
	blk_put_queue(q);

	write_lock_irqsave(&sg_index_lock, flags);
	idr_remove(&sg_index_idr, sdp->index);
	write_unlock_irqrestore(&sg_index_lock, flags);

	SCSI_LOG_TIMEOUT(3,
		sg_printk(KERN_INFO, sdp, "sg_device_destroy\n"));

	kfree(sdp);
}
/* …（同文件无关代码省略）… */
static int
sg_start_req(Sg_request *srp, unsigned char *cmd)
{
	int res;
	struct request *rq;
	Sg_fd *sfp = srp->parentfp;
	sg_io_hdr_t *hp = &srp->header;
	int dxfer_len = (int) hp->dxfer_len;
	int dxfer_dir = hp->dxfer_direction;
	unsigned int iov_count = hp->iovec_count;
	Sg_scatter_hold *req_schp = &srp->data;
	Sg_scatter_hold *rsv_schp = &sfp->reserve;
	struct request_queue *q = sfp->parentdp->device->request_queue;
	struct rq_map_data *md, map_data;
	int rw = hp->dxfer_direction == SG_DXFER_TO_DEV ? ITER_SOURCE : ITER_DEST;
	struct scsi_cmnd *scmd;

	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, sfp->parentdp,
				      "sg_start_req: dxfer_len=%d\n",
				      dxfer_len));

	/*
	 * NOTE
	 *
	 * With scsi-mq enabled, there are a fixed number of preallocated
	 * requests equal in number to shost->can_queue.  If all of the
	 * preallocated requests are already in use, then scsi_alloc_request()
	 * will sleep until an active command completes, freeing up a request.
	 * Although waiting in an asynchronous interface is less than ideal, we
	 * do not want to use BLK_MQ_REQ_NOWAIT here because userspace might
	 * not expect an EWOULDBLOCK from this condition.
	 */
	rq = scsi_alloc_request(q, hp->dxfer_direction == SG_DXFER_TO_DEV ?
			REQ_OP_DRV_OUT : REQ_OP_DRV_IN, 0);
	if (IS_ERR(rq))
		return PTR_ERR(rq);
	scmd = blk_mq_rq_to_pdu(rq);

	if (hp->cmd_len > sizeof(scmd->cmnd)) {
		blk_mq_free_request(rq);
		return -EINVAL;
	}

	memcpy(scmd->cmnd, cmd, hp->cmd_len);
	scmd->cmd_len = hp->cmd_len;

	srp->rq = rq;
	rq->end_io_data = srp;
	scmd->allowed = SG_DEFAULT_RETRIES;

	if ((dxfer_len <= 0) || (dxfer_dir == SG_DXFER_NONE))
		return 0;

	if (sg_allow_dio && hp->flags & SG_FLAG_DIRECT_IO &&
	    dxfer_dir != SG_DXFER_UNKNOWN && !iov_count &&
	    blk_rq_aligned(q, (unsigned long)hp->dxferp, dxfer_len))
		md = NULL;
	else
		md = &map_data;

	if (md) {
		mutex_lock(&sfp->f_mutex);
		if (dxfer_len <= rsv_schp->bufflen &&
		    !sfp->res_in_use) {
			sfp->res_in_use = 1;
			sg_link_reserve(sfp, srp, dxfer_len);
		} else if (hp->flags & SG_FLAG_MMAP_IO) {
			res = -EBUSY; /* sfp->res_in_use == 1 */
			if (dxfer_len > rsv_schp->bufflen)
				res = -ENOMEM;
			mutex_unlock(&sfp->f_mutex);
			return res;
		} else {
			res = sg_build_indirect(req_schp, sfp, dxfer_len);
			if (res) {
				mutex_unlock(&sfp->f_mutex);
				return res;
			}
		}
		mutex_unlock(&sfp->f_mutex);

		md->pages = req_schp->pages;
		md->page_order = req_schp->page_order;
		md->nr_entries = req_schp->k_use_sg;
		md->offset = 0;
		md->null_mapped = hp->dxferp ? 0 : 1;
		if (dxfer_dir == SG_DXFER_TO_FROM_DEV)
			md->from_user = 1;
		else
			md->from_user = 0;
	}

	res = blk_rq_map_user_io(rq, md, hp->dxferp, hp->dxfer_len,
			GFP_KERNEL, iov_count, iov_count, 1, rw);
	if (!res) {
		srp->bio = rq->bio;

		if (!md) {
			req_schp->dio_in_use = 1;
			hp->info |= SG_INFO_DIRECT_IO;
		}
	}
	return res;
}
/* …（同文件无关代码省略）… */
static int
sg_finish_rem_req(Sg_request *srp)
{
	int ret = 0;

	Sg_fd *sfp = srp->parentfp;
	Sg_scatter_hold *req_schp = &srp->data;

	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, sfp->parentdp,
				      "sg_finish_rem_req: res_used=%d\n",
				      (int) srp->res_used));
	if (srp->bio)
		ret = blk_rq_unmap_user(srp->bio);

	if (srp->rq)
		blk_mq_free_request(srp->rq);

	if (srp->res_used)
		sg_unlink_reserve(sfp, srp);
	else
		sg_remove_scat(sfp, req_schp);

	return ret;
}
/* …（同文件无关代码省略）… */
static int
sg_build_sgat(Sg_scatter_hold * schp, const Sg_fd * sfp, int tablesize)
{
	int sg_bufflen = tablesize * sizeof(struct page *);
	gfp_t gfp_flags = GFP_ATOMIC | __GFP_NOWARN;

	schp->pages = kzalloc(sg_bufflen, gfp_flags);
	if (!schp->pages)
		return -ENOMEM;
	schp->sglist_len = sg_bufflen;
	return tablesize;	/* number of scat_gath elements allocated */
}
/* …（同文件无关代码省略）… */
static int
sg_build_indirect(Sg_scatter_hold * schp, Sg_fd * sfp, int buff_size)
{
	int ret_sz = 0, i, k, rem_sz, num, mx_sc_elems;
	int sg_tablesize = sfp->parentdp->sg_tablesize;
	int blk_size = buff_size, order;
	gfp_t gfp_mask = GFP_ATOMIC | __GFP_COMP | __GFP_NOWARN | __GFP_ZERO;

	if (blk_size < 0)
		return -EFAULT;
	if (0 == blk_size)
		++blk_size;	/* don't know why */
	/* round request up to next highest SG_SECTOR_SZ byte boundary */
	blk_size = ALIGN(blk_size, SG_SECTOR_SZ);
	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, sfp->parentdp,
		"sg_build_indirect: buff_size=%d, blk_size=%d\n",
		buff_size, blk_size));

	/* N.B. ret_sz carried into this block ... */
	mx_sc_elems = sg_build_sgat(schp, sfp, sg_tablesize);
	if (mx_sc_elems < 0)
		return mx_sc_elems;	/* most likely -ENOMEM */

	num = scatter_elem_sz;
	if (unlikely(num != scatter_elem_sz_prev)) {
		if (num < PAGE_SIZE) {
			scatter_elem_sz = PAGE_SIZE;
			scatter_elem_sz_prev = PAGE_SIZE;
		} else
			scatter_elem_sz_prev = num;
	}

	order = get_order(num);
retry:
	ret_sz = 1 << (PAGE_SHIFT + order);

	for (k = 0, rem_sz = blk_size; rem_sz > 0 && k < mx_sc_elems;
	     k++, rem_sz -= ret_sz) {

		num = (rem_sz > scatter_elem_sz_prev) ?
			scatter_elem_sz_prev : rem_sz;

		schp->pages[k] = alloc_pages(gfp_mask, order);
		if (!schp->pages[k])
			goto out;

		if (num == scatter_elem_sz_prev) {
			if (unlikely(ret_sz > scatter_elem_sz_prev)) {
				scatter_elem_sz = ret_sz;
				scatter_elem_sz_prev = ret_sz;
			}
		}

		SCSI_LOG_TIMEOUT(5, sg_printk(KERN_INFO, sfp->parentdp,
				 "sg_build_indirect: k=%d, num=%d, ret_sz=%d\n",
				 k, num, ret_sz));
	}		/* end of for loop */

	schp->page_order = order;
	schp->k_use_sg = k;
	SCSI_LOG_TIMEOUT(5, sg_printk(KERN_INFO, sfp->parentdp,
			 "sg_build_indirect: k_use_sg=%d, rem_sz=%d\n",
			 k, rem_sz));

	schp->bufflen = blk_size;
	if (rem_sz > 0)	/* must have failed */
		return -ENOMEM;
	return 0;
out:
	for (i = 0; i < k; i++)
		__free_pages(schp->pages[i], order);

	if (--order >= 0)
		goto retry;

	return -ENOMEM;
}
/* …（同文件无关代码省略）… */
static void
sg_remove_scat(Sg_fd * sfp, Sg_scatter_hold * schp)
{
	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, sfp->parentdp,
			 "sg_remove_scat: k_use_sg=%d\n", schp->k_use_sg));
	if (schp->pages && schp->sglist_len > 0) {
		if (!schp->dio_in_use) {
			int k;

			for (k = 0; k < schp->k_use_sg && schp->pages[k]; k++) {
				SCSI_LOG_TIMEOUT(5,
					sg_printk(KERN_INFO, sfp->parentdp,
					"sg_remove_scat: k=%d, pg=0x%p\n",
					k, schp->pages[k]));
				__free_pages(schp->pages[k], schp->page_order);
			}

			kfree(schp->pages);
		}
	}
	memset(schp, 0, sizeof (*schp));
}
/* …（同文件无关代码省略）… */
static void
sg_build_reserve(Sg_fd * sfp, int req_size)
{
	Sg_scatter_hold *schp = &sfp->reserve;

	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, sfp->parentdp,
			 "sg_build_reserve: req_size=%d\n", req_size));
	do {
		if (req_size < PAGE_SIZE)
			req_size = PAGE_SIZE;
		if (0 == sg_build_indirect(schp, sfp, req_size))
			return;
		else
			sg_remove_scat(sfp, schp);
		req_size >>= 1;	/* divide by 2 */
	} while (req_size > (PAGE_SIZE / 2));
}
/* …（同文件无关代码省略）… */
static void
sg_unlink_reserve(Sg_fd * sfp, Sg_request * srp)
{
	Sg_scatter_hold *req_schp = &srp->data;

	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, srp->parentfp->parentdp,
				      "sg_unlink_reserve: req->k_use_sg=%d\n",
				      (int) req_schp->k_use_sg));
	req_schp->k_use_sg = 0;
	req_schp->bufflen = 0;
	req_schp->pages = NULL;
	req_schp->page_order = 0;
	req_schp->sglist_len = 0;
	srp->res_used = 0;
	/* Called without mutex lock to avoid deadlock */
	sfp->res_in_use = 0;
}
/* …（同文件无关代码省略）… */
static Sg_request *
sg_add_request(Sg_fd * sfp)
{
	int k;
	unsigned long iflags;
	Sg_request *rp = sfp->req_arr;

	write_lock_irqsave(&sfp->rq_list_lock, iflags);
	if (!list_empty(&sfp->rq_list)) {
		if (!sfp->cmd_q)
			goto out_unlock;

		for (k = 0; k < SG_MAX_QUEUE; ++k, ++rp) {
			if (!rp->parentfp)
				break;
		}
		if (k >= SG_MAX_QUEUE)
			goto out_unlock;
	}
	memset(rp, 0, sizeof (Sg_request));
	rp->parentfp = sfp;
	rp->header.duration = jiffies_to_msecs(jiffies);
	list_add_tail(&rp->entry, &sfp->rq_list);
	write_unlock_irqrestore(&sfp->rq_list_lock, iflags);
	return rp;
out_unlock:
	write_unlock_irqrestore(&sfp->rq_list_lock, iflags);
	return NULL;
}
/* …（同文件无关代码省略）… */
static int
sg_remove_request(Sg_fd * sfp, Sg_request * srp)
{
	unsigned long iflags;
	int res = 0;

	if (!sfp || !srp || list_empty(&sfp->rq_list))
		return res;
	write_lock_irqsave(&sfp->rq_list_lock, iflags);
	if (!list_empty(&srp->entry)) {
		list_del(&srp->entry);
		srp->parentfp = NULL;
		res = 1;
	}
	write_unlock_irqrestore(&sfp->rq_list_lock, iflags);

	/*
	 * If the device is detaching, wakeup any readers in case we just
	 * removed the last response, which would leave nothing for them to
	 * return other than -ENODEV.
	 */
	if (unlikely(atomic_read(&sfp->parentdp->detaching)))
		wake_up_interruptible_all(&sfp->read_wait);

	return res;
}
/* …（同文件无关代码省略）… */
static Sg_fd *
sg_add_sfp(Sg_device * sdp)
{
	Sg_fd *sfp;
	unsigned long iflags;
	int bufflen;

	sfp = kzalloc_obj(*sfp, GFP_ATOMIC | __GFP_NOWARN);
	if (!sfp)
		return ERR_PTR(-ENOMEM);

	init_waitqueue_head(&sfp->read_wait);
	rwlock_init(&sfp->rq_list_lock);
	INIT_LIST_HEAD(&sfp->rq_list);
	kref_init(&sfp->f_ref);
	mutex_init(&sfp->f_mutex);
	sfp->timeout = SG_DEFAULT_TIMEOUT;
	sfp->timeout_user = SG_DEFAULT_TIMEOUT_USER;
	sfp->force_packid = SG_DEF_FORCE_PACK_ID;
	sfp->cmd_q = SG_DEF_COMMAND_Q;
	sfp->keep_orphan = SG_DEF_KEEP_ORPHAN;
	sfp->parentdp = sdp;
	write_lock_irqsave(&sdp->sfd_lock, iflags);
	if (atomic_read(&sdp->detaching)) {
		write_unlock_irqrestore(&sdp->sfd_lock, iflags);
		kfree(sfp);
		return ERR_PTR(-ENODEV);
	}
	list_add_tail(&sfp->sfd_siblings, &sdp->sfds);
	write_unlock_irqrestore(&sdp->sfd_lock, iflags);
	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sdp,
				      "sg_add_sfp: sfp=0x%p\n", sfp));

	bufflen = min_t(int, def_reserved_size,
			max_sectors_bytes(sdp->device->request_queue));
	sg_build_reserve(sfp, bufflen);
	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sdp,
				      "sg_add_sfp: bufflen=%d, k_use_sg=%d\n",
				      sfp->reserve.bufflen,
				      sfp->reserve.k_use_sg));

	kref_get(&sdp->d_ref);
	__module_get(THIS_MODULE);
	return sfp;
}
/* …（同文件无关代码省略）… */
static void
sg_remove_sfp_usercontext(struct work_struct *work)
{
	struct sg_fd *sfp = container_of(work, struct sg_fd, ew.work);
	struct sg_device *sdp = sfp->parentdp;
	struct scsi_device *device = sdp->device;
	Sg_request *srp;
	unsigned long iflags;

	/* Cleanup any responses which were never read(). */
	write_lock_irqsave(&sfp->rq_list_lock, iflags);
	while (!list_empty(&sfp->rq_list)) {
		srp = list_first_entry(&sfp->rq_list, Sg_request, entry);
		list_del(&srp->entry);
		write_unlock_irqrestore(&sfp->rq_list_lock, iflags);

		sg_finish_rem_req(srp);
		/*
		 * sg_rq_end_io() uses srp->parentfp. Hence, only clear
		 * srp->parentfp after blk_mq_free_request() has been called.
		 */
		srp->parentfp = NULL;

		write_lock_irqsave(&sfp->rq_list_lock, iflags);
	}
	write_unlock_irqrestore(&sfp->rq_list_lock, iflags);

	if (sfp->reserve.bufflen > 0) {
		SCSI_LOG_TIMEOUT(6, sg_printk(KERN_INFO, sdp,
				"sg_remove_sfp:    bufflen=%d, k_use_sg=%d\n",
				(int) sfp->reserve.bufflen,
				(int) sfp->reserve.k_use_sg));
		sg_remove_scat(sfp, &sfp->reserve);
	}

	SCSI_LOG_TIMEOUT(6, sg_printk(KERN_INFO, sdp,
			"sg_remove_sfp: sfp=0x%p\n", sfp));
	kfree(sfp);

	kref_put(&sdp->d_ref, sg_device_destroy);
	scsi_device_put(device);
	module_put(THIS_MODULE);
}
/* …（同文件无关代码省略）… */
static void
sg_remove_sfp(struct kref *kref)
{
	struct sg_fd *sfp = container_of(kref, struct sg_fd, f_ref);
	struct sg_device *sdp = sfp->parentdp;
	unsigned long iflags;

	write_lock_irqsave(&sdp->sfd_lock, iflags);
	list_del(&sfp->sfd_siblings);
	write_unlock_irqrestore(&sdp->sfd_lock, iflags);

	INIT_WORK(&sfp->ew.work, sg_remove_sfp_usercontext);
	schedule_work(&sfp->ew.work);
}
/* …（同文件无关代码省略）… */
static Sg_device *sg_lookup_dev(int dev)
{
	return idr_find(&sg_index_idr, dev);
}
/* …（同文件无关代码省略）… */
static Sg_device *
sg_get_dev(int dev)
{
	struct sg_device *sdp;
	unsigned long flags;

	read_lock_irqsave(&sg_index_lock, flags);
	sdp = sg_lookup_dev(dev);
	if (!sdp)
		sdp = ERR_PTR(-ENXIO);
	else if (atomic_read(&sdp->detaching)) {
		/* If sdp->detaching, then the refcount may already be 0, in
		 * which case it would be a bug to do kref_get().
		 */
		sdp = ERR_PTR(-ENODEV);
	} else
		kref_get(&sdp->d_ref);
	read_unlock_irqrestore(&sg_index_lock, flags);

	return sdp;
}
