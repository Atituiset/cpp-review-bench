// AUTO-DRAFT from torvalds/linux PR #69f80fef3153299d9c72c53d1d71eef6354b6926
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
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
		if (qc->cdb[0] == ALLOW_MEDIUM_REMOVAL && qc->dev->sdev)
			qc->dev->sdev->locked = 0;

		if (cmd->result)
			ata_scsi_qc_done(qc, false, 0);
		else
			ata_scsi_qc_done(qc, true, SAM_STAT_CHECK_CONDITION);
		goto schedule_deferred;
	}
