// AUTO-DRAFT from torvalds/linux PR #e6f836ef7865fb454641febe080cf88a3c677186
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#define CACHE_MPAGE			0x8
#define CACHE_MPAGE_LEN			20
#define CONTROL_MPAGE			0xa
#define CONTROL_MPAGE_LEN		12
/* …（同文件无关代码省略）… */
#define ALL_SUB_MPAGES			0xff
/* …（同文件无关代码省略）… */
#define ATA_FEATURE_SUB_MPAGE		0xf2
#define ATA_FEATURE_SUB_MPAGE_LEN	16
/* …（同文件无关代码省略）… */
void ata_scsi_set_sense(struct ata_device *dev, struct scsi_cmnd *cmd,
			u8 sk, u16 code)
{
	bool d_sense = (dev->flags & ATA_DFLAG_D_SENSE);

	scsi_set_sense(cmd, d_sense, sk, code);
}
/* …（同文件无关代码省略）… */
struct ata_err_sense {
	u8	err_mask;
	u8	sense_key;
	u16	sense_code;
};
/* …（同文件无关代码省略）… */
static void ata_to_sense_error(u8 drv_stat, u8 drv_err, u8 *sk, u16 *scode)
{
	int i;

	/* Based on the 3ware driver translation table */
	static const struct ata_err_sense sense_table[] = {
		{
			/* BBD|ECC|ID|MAR - Device busy */
			0xd1,
			ABORTED_COMMAND,
			NO_ADDITIONAL_SENSE_INFORMATION
		},
		{
			/* BBD|ECC|ID - Device busy */
			0xd0,
			ABORTED_COMMAND,
			NO_ADDITIONAL_SENSE_INFORMATION
		},
		{
			/* ECC|MC|MARK - Device fault */
			0x61,
			HARDWARE_ERROR,
			NO_ADDITIONAL_SENSE_INFORMATION
		},
		{
			/* ICRC|ABRT - Data CRC error */
			/* NB: ICRC & !ABRT is BBD */
			0x84,
			ABORTED_COMMAND,
			SCSI_PARITY_ERROR
		},
		{
			/* MC|ID|ABRT|TRK0|MARK - Unit offline */
			0x37,
			NOT_READY,
			LU_NOT_READY
		},
		{
			/* MCR|MARK - Unrecovered disk error */
			0x09,
			NOT_READY,
			LU_NOT_READY
		},
		{
			/* Bad address mark */
			0x01,
			MEDIUM_ERROR,
			ADDRESS_MARK_NOT_FOUND_FOR_DATA_FIELD
		},
		{
			/* TRK0 - Track 0 not found */
			0x02,
			HARDWARE_ERROR,
			NO_ADDITIONAL_SENSE_INFORMATION
		},
		{
			/* Abort: 0x04 is not translated here, see below */
			/* Media change request */
			0x08,
			NOT_READY,
			LU_NOT_READY
		},
		{
			/* SRV/IDNF - ID not found */
			0x10,
			ILLEGAL_REQUEST,
			LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE
		},
		{
			/* MC - Media Changed */
			0x20,
			UNIT_ATTENTION,
			NOT_READY_TO_READY_CHANGE_MEDIUM_MAY_HAVE_CHANGED
		},
		{
			/* ECC - Uncorrectable ECC error */
			0x40,
			MEDIUM_ERROR,
			UNRECOVERED_READ_ERROR_AUTO_REALLOCATE_FAILED
		},
		{
			/* BBD - block marked bad */
			0x80,
			MEDIUM_ERROR,
			UNRECOVERED_READ_ERROR_AUTO_REALLOCATE_FAILED
		},
		{
			/* END mark */
			0xFF, 0xFF, 0xFFFF
		},
	};
	static const struct ata_err_sense stat_table[] = {
		{
			/*
			 * Busy: must be first because BUSY means no other bits
			 * valid.
			 */
			ATA_BUSY,
			ABORTED_COMMAND,
			NO_ADDITIONAL_SENSE_INFORMATION
		},
		{
			/* Device fault */
			ATA_DF,
			HARDWARE_ERROR,
			INTERNAL_TARGET_FAILURE
		},
		{
			/* Corrected data error */
			ATA_CORR,
			RECOVERED_ERROR,
			NO_ADDITIONAL_SENSE_INFORMATION
		},
		{
			/* END mark */
			0xFF, 0xFF, 0xFFFF
		},
	};

	/*
	 *	Is this an error we can process/parse
	 */
	if (drv_stat & ATA_BUSY) {
		drv_err = 0;	/* Ignore the err bits, they're invalid */
	}

	if (drv_err) {
		/* Look for drv_err */
		for (i = 0; sense_table[i].err_mask != 0xFF; i++) {
			/* Look for best matches first */
			if ((sense_table[i].err_mask & drv_err) ==
			    sense_table[i].err_mask) {
				*sk = sense_table[i].sense_key;
				*scode = sense_table[i].sense_code;
				return;
			}
		}
	}

	/*
	 * Fall back to interpreting status bits.  Note that if the drv_err
	 * has only the ABRT bit set, we decode drv_stat.  ABRT by itself
	 * is not descriptive enough.
	 */
	for (i = 0; stat_table[i].err_mask != 0xFF; i++) {
		if (stat_table[i].err_mask & drv_stat) {
			*sk = stat_table[i].sense_key;
			*scode = stat_table[i].sense_code;
			return;
		}
	}

	/*
	 * We need a sensible error return here, which is tricky, and one
	 * that won't cause people to do things like return a disk wrongly.
	 */
	*sk = ABORTED_COMMAND;
	*scode = NO_ADDITIONAL_SENSE_INFORMATION;
}
/* …（同文件无关代码省略）… */
static void ata_gen_passthru_sense(struct ata_queued_cmd *qc)
{
	struct ata_device *dev = qc->dev;
	struct scsi_cmnd *cmd = qc->scsicmd;
	struct ata_taskfile *tf = &qc->result_tf;
	u16 sense_code;
	u8 sense_key;

	if (!(qc->flags & ATA_QCFLAG_RTF_FILLED)) {
		ata_dev_dbg(dev,
			    "missing result TF: can't generate ATA PT sense data\n");
		if (qc->err_mask)
			ata_scsi_set_sense(dev, cmd, ABORTED_COMMAND,
					   NO_ADDITIONAL_SENSE_INFORMATION);
		return;
	}

	/*
	 * Use ata_to_sense_error() to map status register bits onto sense key
	 * and sense code.
	 */
	if (qc->err_mask ||
	    tf->status & (ATA_BUSY | ATA_DF | ATA_ERR | ATA_DRQ)) {
		ata_to_sense_error(tf->status, tf->error,
				   &sense_key, &sense_code);
		ata_scsi_set_sense(qc->dev, cmd, sense_key, sense_code);
		return;
	}

	/*
	 * Note: we are supposed to call ata_scsi_set_sense(), which respects
	 * the D_SENSE bit, instead of unconditionally generating the sense data
	 * in descriptor format. However, because hdparm, hddtemp, and udisks
	 * incorrectly assume sense data in descriptor format, without even
	 * looking at the RESPONSE CODE field in the returned sense data (to see
	 * which format the returned sense data is in), we are stuck with being
	 * bug compatible with older kernels.
	 */
	scsi_set_sense(cmd, 1, RECOVERED_ERROR,
		       ATA_PASS_THROUGH_INFORMATION_AVAILABLE);
}
/* …（同文件无关代码省略）… */
static void ata_scsi_qc_done(struct ata_queued_cmd *qc, bool set_result,
			     u32 scmd_result)
{
	struct scsi_cmnd *cmd = qc->scsicmd;
	void (*done)(struct scsi_cmnd *) = qc->scsidone;

	ata_qc_free(qc);

	if (set_result)
		cmd->result = scmd_result;
	done(cmd);
}
/* …（同文件无关代码省略）… */
enum scsi_timeout_action ata_scsi_requeue_deferred_qc(struct ata_port *ap,
					struct scsi_cmnd *timedout_scmd)
{
	enum scsi_timeout_action action = SCSI_EH_NOT_HANDLED;
	struct ata_queued_cmd *qc;
	struct ata_link *link;
	u32 host_byte;

	lockdep_assert_held(ap->lock);

	/*
	 * If we have deferred QCs when a reset, a timeout or an NCQ command
	 * fails, do not try to be smart about what to do with the deferred
	 * commands and simply terminate them and let the SCSI layer decide
	 * what to do.
	 */
	ata_for_each_link(link, ap, PMP_FIRST) {
		qc = link->deferred_qc;
		if (!qc)
			continue;

		/*
		 * Clear the deferred QC so that the deferred work does not try
		 * to issue it.
		 */
		link->deferred_qc = NULL;
		cancel_work(&link->deferred_qc_work);

		/*
		 * We are going to complete some scsi command, either with
		 * DID_TIME_OUT if the command timed out while waiting for being
		 * issued, or with DID_REQUEUE if another command timed out or
		 * we had a failed command. However, the block layer may re-issue
		 * these commands immediately, keeping the scsi host busy and
		 * thus preventing the SCSI EH task from running.
		 * So schedule EH on the port to prevent accepting new commands
		 * until everything is sorted out with the error or timeout that
		 * got us here in the first place. Note that we set EH pending
		 * on the port before calling ata_port_schedule_eh() so that we
		 * do not reenter this function from ata_eh_set_pending() with
		 * timedout_scmd being NULL and erroneously retry deferred QCs
		 * that have timed out on other links.
		 */
		if (!ata_port_eh_scheduled(ap)) {
			ap->pflags |= ATA_PFLAG_EH_PENDING;
			ata_port_schedule_eh(ap);
		}

		/*
		 * If we are being called from scsi_timeout(), then we have a
		 * non-NULL timedout_scmd. If the timed out command is for a
		 * deferred QC, terminate that deferred QC with DID_TIME_OUT and
		 * requeue all other deferred QCs. In this case we need to
		 * return SCSI_EH_DONE, because the timed out command was
		 * handled.
		 * If the timed out command is not for a deferred QC, we need to
		 * requeue all deferred QCs, and return SCSI_EH_NOT_HANDLED so
		 * that the timed out command gets added to the EH work queue
		 * with scsi_eh_scmd_add(), for later handling with libata EH
		 * ata_scsi_cmd_error_handler().
		 * If timedout_scmd is NULL, we simply need to requeue all
		 * deferred QCs and the return value does not matter as we were
		 * not called from scsi_timeout().
		 */
		if (timedout_scmd && qc->scsicmd == timedout_scmd) {
			host_byte = DID_TIME_OUT;
			action = SCSI_EH_DONE;
		} else {
			host_byte = DID_REQUEUE;
		}
		ata_scsi_qc_done(qc, true, host_byte << 16);
	}

	return action;
}
/* …（同文件无关代码省略）… */
static void ata_scsi_schedule_deferred_qc(struct ata_link *link)
{
	struct ata_queued_cmd *qc = link->deferred_qc;
	struct ata_port *ap = link->ap;

	lockdep_assert_held(ap->lock);

	/*
	 * If we have a deferred qc, then qc_defer() is defined and we can use
	 * this callback to determine if this qc is good to go, unless EH has
	 * been scheduled.
	 */
	if (!qc)
		return;

	if (ata_port_eh_scheduled(ap)) {
		ata_scsi_requeue_deferred_qc(ap, NULL);
		return;
	}
	if (!ap->ops->qc_defer(qc))
		queue_work(system_highpri_wq, &link->deferred_qc_work);
}
/* …（同文件无关代码省略）… */
	if (xlat_func(qc))
		goto done;

	return ata_scsi_qc_issue(ap, qc);

done:
/* …（同文件无关代码省略）… */
	unsigned long flags;
	unsigned int len;

	spin_lock_irqsave(&ata_scsi_rbuf_lock, flags);

	memset(ata_scsi_rbuf, 0, ATA_SCSI_RBUF_SIZE);
/* …（同文件无关代码省略）… */
static void modecpy(u8 *dest, const u8 *src, int n, bool changeable)
{
	if (changeable) {
		memcpy(dest, src, 2);
		memset(dest + 2, 0, n - 2);
	} else {
		memcpy(dest, src, n);
	}
}
/* …（同文件无关代码省略）… */
static unsigned int ata_msense_caching(u16 *id, u8 *buf, bool changeable)
{
	modecpy(buf, def_cache_mpage, sizeof(def_cache_mpage), changeable);
	if (changeable) {
		buf[2] |= (1 << 2);	/* ata_mselect_caching() */
	} else {
		buf[2] |= (ata_id_wcache_enabled(id) << 2);	/* write cache enable */
		buf[12] |= (!ata_id_rahead_enabled(id) << 5);	/* disable read ahead */
	}
	return sizeof(def_cache_mpage);
}
/* …（同文件无关代码省略）… */
static unsigned int ata_msense_control_spg0(struct ata_device *dev, u8 *buf,
					    bool changeable)
{
	modecpy(buf, def_control_mpage,
		sizeof(def_control_mpage), changeable);
	if (changeable) {
		/* ata_mselect_control() */
		buf[2] |= (1 << 2);
	} else {
		bool d_sense = (dev->flags & ATA_DFLAG_D_SENSE);

		/* descriptor format sense data */
		buf[2] |= (d_sense << 2);
	}

	return sizeof(def_control_mpage);
}
/* …（同文件无关代码省略）… */
static void atapi_fixup_inquiry(struct scsi_cmnd *cmd)
{
	u8 buf[4];

	sg_copy_to_buffer(scsi_sglist(cmd), scsi_sg_count(cmd), buf, 4);
	if (buf[2] == 0) {
		buf[2] = 0x5;
		buf[3] = 0x32;
	}
	sg_copy_from_buffer(scsi_sglist(cmd), scsi_sg_count(cmd), buf, 4);
}
/* …（同文件无关代码省略）… */
static void atapi_qc_complete(struct ata_queued_cmd *qc)
{
	struct ata_link *link = qc->dev->link;
	struct scsi_cmnd *cmd = qc->scsicmd;
	unsigned int err_mask = qc->err_mask;

	/* handle completion from EH */
	if (unlikely(err_mask || qc->flags & ATA_QCFLAG_SENSE_VALID)) {

		if (!(qc->flags & ATA_QCFLAG_SENSE_VALID))
			ata_gen_passthru_sense(qc);

		/* SCSI EH automatically locks door if sdev->locked is
		 * set.  Sometimes door lock request continues to
		 * fail, for example, when no media is present.  This
		 * creates a loop - SCSI EH issues door lock which
		 * fails and gets invoked again to acquire sense data
		 * for the failed command.
		 *
		 * If door lock fails, always clear sdev->locked to
		 * avoid this infinite loop.
		 *
		 * This may happen before SCSI scan is complete.  Make
		 * sure qc->dev->sdev isn't NULL before dereferencing.
		 */
		if (qc->cdb[0] == ALLOW_MEDIUM_REMOVAL && qc->dev->sdev)
			qc->dev->sdev->locked = 0;

		/*
		 * Report CHECK CONDITION unless the command was terminated
		 * with a host byte set. Only set the status byte, so that the
		 * SCSI midlayer internal byte, which scsi_check_sense() may
		 * have set from the sense data, is preserved.
		 */
		if (get_host_byte(cmd) == DID_OK)
			set_status_byte(cmd, SAM_STAT_CHECK_CONDITION);
		ata_scsi_qc_done(qc, false, 0);
		goto schedule_deferred;
	}

	if (cmd->result) {
		ata_scsi_qc_done(qc, false, 0);
		goto schedule_deferred;
	}

	/* successful completion path */
	if (cmd->cmnd[0] == INQUIRY && (cmd->cmnd[1] & 0x03) == 0)
		atapi_fixup_inquiry(cmd);

	ata_scsi_qc_done(qc, true, SAM_STAT_GOOD);

schedule_deferred:
	ata_scsi_schedule_deferred_qc(link);
}
/* …（同文件无关代码省略）… */

	qc->complete_fn = atapi_qc_complete;

	qc->tf.flags |= ATA_TFLAG_ISADDR | ATA_TFLAG_DEVICE;
	if (scmd->sc_data_direction == DMA_TO_DEVICE) {
		qc->tf.flags |= ATA_TFLAG_WRITE;
/* …（同文件无关代码省略）… */
/**
 *	ata_mselect_caching - Simulate MODE SELECT for caching info page
 *	@qc: Storage for translated ATA taskfile
 *	@buf: input buffer
 *	@len: number of valid bytes in the input buffer
 *	@fp: out parameter for the failed field on error
 *
 *	Prepare a taskfile to modify caching information for the device.
 *
 *	LOCKING:
 *	None.
 */
static int ata_mselect_caching(struct ata_queued_cmd *qc,
			       const u8 *buf, int len, u16 *fp)
{
	struct ata_taskfile *tf = &qc->tf;
	struct ata_device *dev = qc->dev;
	u8 mpage[CACHE_MPAGE_LEN];
	u8 wce;
	int i;

	/*
	 * The first two bytes of def_cache_mpage are a header, so offsets
	 * in mpage are off by 2 compared to buf.  Same for len.
	 */

	if (len != CACHE_MPAGE_LEN - 2) {
		*fp = min(len, CACHE_MPAGE_LEN - 2);
		return -EINVAL;
	}

	wce = buf[0] & (1 << 2);

	/*
	 * Check that read-only bits are not modified.
	 */
	ata_msense_caching(dev->id, mpage, false);
	for (i = 0; i < CACHE_MPAGE_LEN - 2; i++) {
		if (i == 0)
			continue;
		if (mpage[i + 2] != buf[i]) {
			*fp = i;
			return -EINVAL;
		}
	}

	tf->flags |= ATA_TFLAG_DEVICE | ATA_TFLAG_ISADDR;
	tf->protocol = ATA_PROT_NODATA;
	tf->nsect = 0;
	tf->command = ATA_CMD_SET_FEATURES;
	tf->feature = wce ? SETFEATURES_WC_ON : SETFEATURES_WC_OFF;
	return 0;
}

/*
 * Simulate MODE SELECT control mode page, sub-page 0.
 */
static int ata_mselect_control_spg0(struct ata_queued_cmd *qc,
				    const u8 *buf, int len, u16 *fp)
{
	struct ata_device *dev = qc->dev;
	u8 mpage[CONTROL_MPAGE_LEN];
	u8 d_sense;
	int i;

	/*
	 * The first two bytes of def_control_mpage are a header, so offsets
	 * in mpage are off by 2 compared to buf.  Same for len.
	 */

	if (len != CONTROL_MPAGE_LEN - 2) {
		*fp = min(len, CONTROL_MPAGE_LEN - 2);
		return -EINVAL;
	}

	d_sense = buf[0] & (1 << 2);

	/*
	 * Check that read-only bits are not modified.
	 */
	ata_msense_control_spg0(dev, mpage, false);
	for (i = 0; i < CONTROL_MPAGE_LEN - 2; i++) {
		if (i == 0)
			continue;
		if (mpage[2 + i] != buf[i]) {
			*fp = i;
			return -EINVAL;
		}
	}
	if (d_sense & (1 << 2))
		dev->flags |= ATA_DFLAG_D_SENSE;
	else
		dev->flags &= ~ATA_DFLAG_D_SENSE;
	return 0;
}

/*
 * Translate MODE SELECT control mode page, sub-page f2h (ATA feature mode
 * page) into a SET FEATURES command.
 */
static int ata_mselect_control_ata_feature(struct ata_queued_cmd *qc,
					   const u8 *buf, int len, u16 *fp)
{
	struct ata_device *dev = qc->dev;
	struct ata_taskfile *tf = &qc->tf;
	u8 cdl_action;

	/*
	 * The first four bytes of ATA Feature Control mode page are a header,
	 * so offsets in mpage are off by 4 compared to buf.  Same for len.
	 */
	if (len != ATA_FEATURE_SUB_MPAGE_LEN - 4) {
		*fp = min(len, ATA_FEATURE_SUB_MPAGE_LEN - 4);
		return -EINVAL;
	}

	/* Check cdl_ctrl */
	switch (buf[0] & 0x03) {
	case 0:
		/* Disable CDL */
		ata_dev_dbg(dev, "Disabling CDL\n");
		cdl_action = 0;
		dev->flags &= ~ATA_DFLAG_CDL_ENABLED;
		break;
	case 0x02:
		/*
		 * Enable CDL. Since CDL is mutually exclusive with NCQ
		 * priority, allow this only if NCQ priority is disabled.
		 */
		if (dev->flags & ATA_DFLAG_NCQ_PRIO_ENABLED) {
			ata_dev_err(dev,
				"NCQ priority must be disabled to enable CDL\n");
			return -EINVAL;
		}
		ata_dev_dbg(dev, "Enabling CDL\n");
		cdl_action = 1;
		dev->flags |= ATA_DFLAG_CDL_ENABLED;
		break;
	default:
		*fp = 0;
		return -EINVAL;
	}

	tf->flags |= ATA_TFLAG_DEVICE | ATA_TFLAG_ISADDR;
	tf->protocol = ATA_PROT_NODATA;
	tf->command = ATA_CMD_SET_FEATURES;
	tf->feature = SETFEATURES_CDL;
	tf->nsect = cdl_action;

	return 1;
}
/* …（同文件无关代码省略）… */
/**
 *	ata_mselect_control - Simulate MODE SELECT for control page
 *	@qc: Storage for translated ATA taskfile
 *	@spg: target sub-page of the control page
 *	@buf: input buffer
 *	@len: number of valid bytes in the input buffer
 *	@fp: out parameter for the failed field on error
 *
 *	Prepare a taskfile to modify caching information for the device.
 *
 *	LOCKING:
 *	None.
 */
static int ata_mselect_control(struct ata_queued_cmd *qc, u8 spg,
			       const u8 *buf, int len, u16 *fp)
{
	switch (spg) {
	case 0:
		return ata_mselect_control_spg0(qc, buf, len, fp);
	case ATA_FEATURE_SUB_MPAGE:
		return ata_mselect_control_ata_feature(qc, buf, len, fp);
	default:
		return -EINVAL;
	}
}
/* …（同文件无关代码省略）… */
		goto invalid_param_len;
	if (bd_len != 0 && bd_len != 8) {
		fp = (six_byte) ? 3 : 6;
		fp += bd_len + hdr_len;
		goto invalid_param;
	}

/* …（同文件无关代码省略）… */
		if (len < 4)
			goto invalid_param_len;

		spg = p[1];
		pg_len = get_unaligned_be16(&p[2]);
		p += 4;
		len -= 4;
	} else {
		if (len < 2)
			goto invalid_param_len;

		spg = 0;
		pg_len = p[1];
		p += 2;
		len -= 2;
	}

	/*
	 * Supported subpages: all subpages and ATA feature sub-page f2h of
	 * the control page.
	 */
	if (spg) {
		switch (spg) {
		case ALL_SUB_MPAGES:
			/* All subpages is not supported for the control page */
			if (pg == CONTROL_MPAGE) {
				fp = (p[0] & 0x40) ? 1 : 0;
				fp += hdr_len + bd_len;
				goto invalid_param;
			}
			break;
		case ATA_FEATURE_SUB_MPAGE:
			if (qc->dev->flags & ATA_DFLAG_CDL &&
			    pg == CONTROL_MPAGE)
				break;
			fallthrough;
		default:
			fp = (p[0] & 0x40) ? 1 : 0;
			fp += hdr_len + bd_len;
			goto invalid_param;
		}
	}
	if (pg_len > len)
		goto invalid_param_len;

	switch (pg) {
	case CACHE_MPAGE:
		if (ata_mselect_caching(qc, p, pg_len, &fp) < 0) {
/* …（同文件无关代码省略）… */
		goto invalid_param;
	}

	/*
	 * Only one page has changeable data, so we only support setting one
	 * page at a time.
	 */
	if (len > pg_len)
		goto invalid_param;

	return 0;

 invalid_fld:
