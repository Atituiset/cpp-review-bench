// AUTO-DRAFT from torvalds/linux PR #e6f836ef7865fb454641febe080cf88a3c677186
static void ata_eh_scsidone(struct scsi_cmnd *scmd)
{
	/* nada */
}
/* …（同文件无关代码省略）… */
static void __ata_eh_qc_complete(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;
	struct scsi_cmnd *scmd = qc->scsicmd;
	unsigned long flags;
  // <<< BUG ANCHOR
	spin_lock_irqsave(ap->lock, flags);
	qc->scsidone = ata_eh_scsidone;
	__ata_qc_complete(qc);
	WARN_ON(ata_tag_valid(qc->tag));
	spin_unlock_irqrestore(ap->lock, flags);

	scsi_eh_finish_cmd(scmd, &ap->eh_done_q);
}
/* …（同文件无关代码省略）… */
static void ata_eh_qc_complete(struct ata_queued_cmd *qc)
{
	struct scsi_cmnd *scmd = qc->scsicmd;

	scmd->retries = scmd->allowed;
	__ata_eh_qc_complete(qc);
}
/* …（同文件无关代码省略）… */
				ata_eh_qc_complete(qc);
			}
		} else {
			if (qc->flags & ATA_QCFLAG_SENSE_VALID ||
			    qc->flags & ATA_QCFLAG_EH_SUCCESS_CMD) {
				ata_eh_qc_complete(qc);
			} else {
				/* feed zero TF to sense generation */
