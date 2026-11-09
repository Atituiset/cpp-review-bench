// AUTO-DRAFT from torvalds/linux PR #e6f836ef7865fb454641febe080cf88a3c677186
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
static void ata_qc_complete_internal(struct ata_queued_cmd *qc)
{
	struct completion *waiting = qc->private_data;

	complete(waiting);
}
/* …（同文件无关代码省略）… */
unsigned int ata_exec_internal(struct ata_device *dev, struct ata_taskfile *tf,
			       const u8 *cdb, enum dma_data_direction dma_dir,
			       void *buf, unsigned int buflen,
			       unsigned int timeout)
{
	struct ata_link *link = dev->link;
	struct ata_port *ap = link->ap;
	const bool owns_eh_mutex = ap->host->eh_owner == current;
	u8 command = tf->command;
	struct ata_queued_cmd *qc;
	struct scatterlist sgl;
	unsigned int preempted_tag;
	u32 preempted_sactive;
	u64 preempted_qc_active;
	int preempted_nr_active_links;
	bool auto_timeout = false;
	DECLARE_COMPLETION_ONSTACK(wait);
	unsigned long flags;
	unsigned int err_mask;
	int rc;

	if (WARN_ON(dma_dir != DMA_NONE && !buf))
		return AC_ERR_INVALID;

	spin_lock_irqsave(ap->lock, flags);

	/* No internal command while frozen */
	if (ata_port_is_frozen(ap)) {
		spin_unlock_irqrestore(ap->lock, flags);
		return AC_ERR_SYSTEM;
	}

	/* Initialize internal qc */
	qc = __ata_qc_from_tag(ap, ATA_TAG_INTERNAL);

	qc->tag = ATA_TAG_INTERNAL;
	qc->hw_tag = 0;
	qc->scsicmd = NULL;
	qc->ap = ap;
	qc->dev = dev;
	ata_qc_reinit(qc);

	preempted_tag = link->active_tag;
	preempted_sactive = link->sactive;
	preempted_qc_active = ap->qc_active;
	preempted_nr_active_links = ap->nr_active_links;
	link->active_tag = ATA_TAG_POISON;
	link->sactive = 0;
	ap->qc_active = 0;
	ap->nr_active_links = 0;

	/* Prepare and issue qc */
	qc->tf = *tf;
	if (cdb)
		memcpy(qc->cdb, cdb, ATAPI_CDB_LEN);

	/* Some SATA bridges need us to indicate data xfer direction */
	if (tf->protocol == ATAPI_PROT_DMA && (dev->flags & ATA_DFLAG_DMADIR) &&
	    dma_dir == DMA_FROM_DEVICE)
		qc->tf.feature |= ATAPI_DMADIR;

	qc->flags |= ATA_QCFLAG_RESULT_TF;
	qc->dma_dir = dma_dir;
	if (dma_dir != DMA_NONE) {
		sg_init_one(&sgl, buf, buflen);
		ata_sg_init(qc, &sgl, 1);
		qc->nbytes = buflen;
	}

	qc->private_data = &wait;
	qc->complete_fn = ata_qc_complete_internal;

	ata_qc_issue(ap, qc);

	spin_unlock_irqrestore(ap->lock, flags);

	if (!timeout) {
		if (ata_probe_timeout) {
			timeout = ata_probe_timeout * 1000;
		} else {
			timeout = ata_internal_cmd_timeout(dev, command);
			auto_timeout = true;
		}
	}

	if (owns_eh_mutex) {
		/*
		 * To prevent that the compiler complains about the
		 * ata_eh_release() call below.
		 */
		__acquire(&ap->host->eh_mutex);
		ata_eh_release(ap);
	}

	rc = wait_for_completion_timeout(&wait, msecs_to_jiffies(timeout));

	if (owns_eh_mutex) {
		ata_eh_acquire(ap);
		/*
		 * To prevent that the compiler complains about the above
		 * ata_eh_acquire() call.
		 */
		__release(&ap->host->eh_mutex);
	}

	ata_sff_flush_pio_task(ap);

	if (!rc) {
		/*
		 * We are racing with irq here. If we lose, the following test
		 * prevents us from completing the qc twice. If we win, the port
		 * is frozen and will be cleaned up by ->post_internal_cmd().
		 */
		spin_lock_irqsave(ap->lock, flags);
		if (qc->flags & ATA_QCFLAG_ACTIVE) {
			qc->err_mask |= AC_ERR_TIMEOUT;
			ata_port_freeze(ap);
			ata_dev_warn(dev, "qc timeout after %u msecs (cmd 0x%x)\n",
				     timeout, command);
		}
		spin_unlock_irqrestore(ap->lock, flags);
	}

	if (ap->ops->post_internal_cmd)
		ap->ops->post_internal_cmd(qc);

	/* Perform minimal error analysis */
	if (qc->flags & ATA_QCFLAG_EH) {
		if (qc->result_tf.status & (ATA_ERR | ATA_DF))
			qc->err_mask |= AC_ERR_DEV;

		if (!qc->err_mask)
			qc->err_mask |= AC_ERR_OTHER;

		if (qc->err_mask & ~AC_ERR_OTHER)
			qc->err_mask &= ~AC_ERR_OTHER;
	} else if (qc->tf.command == ATA_CMD_REQ_SENSE_DATA) {
		qc->result_tf.status |= ATA_SENSE;
	}

	/* Finish up */
	spin_lock_irqsave(ap->lock, flags);

	*tf = qc->result_tf;
	err_mask = qc->err_mask;

	ata_qc_free(qc);
	link->active_tag = preempted_tag;
	link->sactive = preempted_sactive;
	ap->qc_active = preempted_qc_active;
	ap->nr_active_links = preempted_nr_active_links;

	spin_unlock_irqrestore(ap->lock, flags);

	if ((err_mask & AC_ERR_TIMEOUT) && auto_timeout)
		ata_internal_cmd_timed_out(dev, command);

	return err_mask;
}
/* …（同文件无关代码省略）… */
bool ata_dev_power_init_tf(struct ata_device *dev, struct ata_taskfile *tf,
			   bool set_active)
{
	/* Only applies to ATA and ZAC devices */
	if (dev->class != ATA_DEV_ATA && dev->class != ATA_DEV_ZAC)
		return false;

	ata_tf_init(dev, tf);
	tf->flags |= ATA_TFLAG_DEVICE | ATA_TFLAG_ISADDR;
	tf->protocol = ATA_PROT_NODATA;

	if (set_active) {
		/* VERIFY for 1 sector at lba=0 */
		tf->command = ATA_CMD_VERIFY;
		tf->nsect = 1;
		if (dev->flags & ATA_DFLAG_LBA) {
			tf->flags |= ATA_TFLAG_LBA;
			tf->device |= ATA_LBA;
		} else {
			/* CHS */
			tf->lbal = 0x1; /* sect */
		}
	} else {
		tf->command = ATA_CMD_STANDBYNOW1;
	}

	return true;
}
/* …（同文件无关代码省略）… */
static bool ata_dev_power_is_active(struct ata_device *dev)
{
	struct ata_taskfile tf;
	unsigned int err_mask;

	ata_tf_init(dev, &tf);
	tf.flags |= ATA_TFLAG_DEVICE | ATA_TFLAG_ISADDR;
	tf.protocol = ATA_PROT_NODATA;
	tf.command = ATA_CMD_CHK_POWER;

	err_mask = ata_exec_internal(dev, &tf, NULL, DMA_NONE, NULL, 0, 0);
	if (err_mask) {
		ata_dev_err(dev, "Check power mode failed (err_mask=0x%x)\n",
			    err_mask);
		/*
		 * Assume we are in standby mode so that we always force a
		 * spinup in ata_dev_power_set_active().
		 */
		return false;
	}

	ata_dev_dbg(dev, "Power mode: 0x%02x\n", tf.nsect);

	/* Active or idle */
	return tf.nsect == 0xff;
}
/* …（同文件无关代码省略）… */
	struct ata_taskfile tf;
	unsigned int err_mask;

	/* If the device is already sleeping or in standby, do nothing. */
	if ((dev->flags & ATA_DFLAG_SLEEPING) ||
	    !ata_dev_power_is_active(dev))
/* …（同文件无关代码省略）… */
	    system_entering_hibernation())
		return;

	/* Issue STANDBY IMMEDIATE command only if supported by the device */
	if (!ata_dev_power_init_tf(dev, &tf, false))
		return;

	ata_dev_notice(dev, "Entering standby power mode\n");

	err_mask = ata_exec_internal(dev, &tf, NULL, DMA_NONE, NULL, 0, 0);
/* …（同文件无关代码省略）… */
void ata_sg_init(struct ata_queued_cmd *qc, struct scatterlist *sg,
		 unsigned int n_elem)
{
	qc->sg = sg;
	qc->n_elem = n_elem;
	qc->cursg = qc->sg;
}
/* …（同文件无关代码省略）… */
void ata_qc_free(struct ata_queued_cmd *qc)
{
	qc->flags = 0;
	if (ata_tag_valid(qc->tag))
		qc->tag = ATA_TAG_POISON;
}
