// AUTO-DRAFT from torvalds/linux PR #a41e364995bbcfa627418a70fe913c35b7510728
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define FAILURE (-1)
/* …（同文件无关代码省略）… */
  Lock should already have been acquired by the caller.
*/

static struct blogic_ccb *blogic_alloc_ccb(struct blogic_adapter *adapter)
{
	static unsigned long serial;
	struct blogic_ccb *ccb;
	ccb = adapter->free_ccbs;
/* …（同文件无关代码省略）… */
  caller.
*/

static void blogic_dealloc_ccb(struct blogic_ccb *ccb, int dma_unmap)
{
	struct blogic_adapter *adapter = ccb->adapter;

/* …（同文件无关代码省略）… */
  should already have been acquired by the caller.
*/

static void blogic_process_ccbs(struct blogic_adapter *adapter)
{
	if (adapter->processing_ccbs)
		return;
	adapter->processing_ccbs = true;
/* …（同文件无关代码省略）… */
			/*
			   Place CCB back on the Host Adapter's free list.
			 */
			blogic_dealloc_ccb(ccb, 1);
#if 0			/* this needs to be redone different for new EH */
			/*
			   Bus Device Reset CCBs have the command field
/* …（同文件无关代码省略）… */
				if (ccb->status == BLOGIC_CCB_RESET &&
						ccb->tgt_id == tgt_id) {
					command = ccb->command;
					blogic_dealloc_ccb(ccb, 1);
					adapter->active_cmds[tgt_id]--;
					command->result = DID_RESET << 16;
					scsi_done(command);
/* …（同文件无关代码省略）… */
			/*
			   Place CCB back on the Host Adapter's free list.
			 */
			blogic_dealloc_ccb(ccb, 1);
			/*
			   Call the SCSI Command Completion Routine.
			 */
/* …（同文件无关代码省略）… */
static irqreturn_t blogic_inthandler(int irq_ch, void *devid)
{
	struct blogic_adapter *adapter = (struct blogic_adapter *) devid;
	unsigned long processor_flag;
	/*
	   Acquire exclusive access to Host Adapter.
	 */
	spin_lock_irqsave(&adapter->scsi_host->host_lock, processor_flag);
	/*
	   Handle Interrupts appropriately for each Host Adapter type.
	 */
/* …（同文件无关代码省略）… */
	   Process any completed CCBs.
	 */
	if (adapter->firstccb != NULL)
		blogic_process_ccbs(adapter);
	/*
	   Reset the Host Adapter if requested.
	 */
	if (adapter->adapter_extreset) {
		blogic_warn("Resetting %s due to External SCSI Bus Reset\n", adapter, adapter->full_model);
		blogic_inc_count(&adapter->ext_resets);
		blogic_resetadapter(adapter, false);
		adapter->adapter_extreset = false;
	} else if (adapter->adapter_intern_err) {
		blogic_warn("Resetting %s due to Host Adapter Internal Error\n", adapter, adapter->full_model);
		blogic_inc_count(&adapter->adapter_intern_errors);
		blogic_resetadapter(adapter, true);
		adapter->adapter_intern_err = false;
	}
	/*
	   Release exclusive access to Host Adapter.
	 */
	spin_unlock_irqrestore(&adapter->scsi_host->host_lock, processor_flag);
	return IRQ_HANDLED;
}

/* …（同文件无关代码省略）… */
  already have been acquired by the caller.
*/

static bool blogic_write_outbox(struct blogic_adapter *adapter,
		enum blogic_action action, struct blogic_ccb *ccb)
{
	struct blogic_outbox *next_outbox;

	next_outbox = adapter->next_outbox;
/* …（同文件无关代码省略）… */
{
	struct blogic_adapter *adapter =
		(struct blogic_adapter *) SCpnt->device->host->hostdata;

	unsigned int id = SCpnt->device->id;
	struct blogic_tgt_stats *stats = &adapter->tgt_stats[id];
	int rc;

	spin_lock_irq(&SCpnt->device->host->host_lock);

	blogic_inc_count(&stats->adapter_reset_req);

	rc = blogic_resetadapter(adapter, false);
	spin_unlock_irq(&SCpnt->device->host->host_lock);
	return rc;
}

/* …（同文件无关代码省略）… */
  Outgoing Mailbox for execution by the associated Host Adapter.
*/

static enum scsi_qc_status blogic_qcmd_lck(struct scsi_cmnd *command)
{
	void (*comp_cb)(struct scsi_cmnd *) = scsi_done;
	struct blogic_adapter *adapter =
		(struct blogic_adapter *) command->device->host->hostdata;
	struct blogic_tgt_flags *tgt_flags =
		&adapter->tgt_flags[command->device->id];
	struct blogic_tgt_stats *tgt_stats = adapter->tgt_stats;
/* …（同文件无关代码省略）… */
	   probably hung so signal an error as a Host Adapter Hard Reset
	   should be initiated soon.
	 */
	ccb = blogic_alloc_ccb(adapter);
	if (ccb == NULL) {
		spin_unlock_irq(&adapter->scsi_host->host_lock);
		blogic_delay(1);
		spin_lock_irq(&adapter->scsi_host->host_lock);
		ccb = blogic_alloc_ccb(adapter);
		if (ccb == NULL) {
			command->result = DID_ERROR << 16;
			comp_cb(command);
/* …（同文件无关代码省略）… */
	if (dma_mapping_error(&adapter->pci_device->dev, sense_buf)) {
		blogic_err("DMA mapping for sense data buffer failed\n",
				adapter);
		blogic_dealloc_ccb(ccb, 0);
		return SCSI_MLQUEUE_HOST_BUSY;
	}
	ccb->sensedata = sense_buf;
/* …（同文件无关代码省略）… */
		   so signal an error as a Host Adapter Hard Reset should
		   be initiated soon.
		 */
		if (!blogic_write_outbox(adapter, BLOGIC_MBOX_START, ccb)) {
			spin_unlock_irq(&adapter->scsi_host->host_lock);
			blogic_warn("Unable to write Outgoing Mailbox - Pausing for 1 second\n", adapter);
			blogic_delay(1);
			spin_lock_irq(&adapter->scsi_host->host_lock);
			if (!blogic_write_outbox(adapter, BLOGIC_MBOX_START,
						ccb)) {
				blogic_warn("Still unable to write Outgoing Mailbox - Host Adapter Dead?\n", adapter);
				blogic_dealloc_ccb(ccb, 1);
				command->result = DID_ERROR << 16;
				scsi_done(command);
			}
/* …（同文件无关代码省略）… */
		   pending.
		 */
		if (ccb->status == BLOGIC_CCB_COMPLETE)
			blogic_process_ccbs(adapter);
	}
	return 0;
}

static DEF_SCSI_QCMD(blogic_qcmd)

#if 0
/*
/* …（同文件无关代码省略）… */

static int blogic_abort(struct scsi_cmnd *command)
{
	struct blogic_adapter *adapter =
		(struct blogic_adapter *) command->device->host->hostdata;

	int tgt_id = command->device->id;
	struct blogic_ccb *ccb;
	blogic_inc_count(&adapter->tgt_stats[tgt_id].aborts_request);

	/*
	   Attempt to find an Active CCB for this Command. If no Active
	   CCB for this Command is found, then no Abort is necessary.
	 */
	for (ccb = adapter->all_ccbs; ccb != NULL; ccb = ccb->next_all)
		if (ccb->command == command)
			break;
	if (ccb == NULL) {
		blogic_warn("Unable to Abort Command to Target %d - No CCB Found\n", adapter, tgt_id);
		return SUCCESS;
	} else if (ccb->status == BLOGIC_CCB_COMPLETE) {
		blogic_warn("Unable to Abort Command to Target %d - 
