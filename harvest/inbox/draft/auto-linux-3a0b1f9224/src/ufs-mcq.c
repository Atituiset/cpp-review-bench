// AUTO-DRAFT from torvalds/linux PR #d5b6dc381db93ff2d1888f7d99c06e0bd85f4083
#define CQE_UCD_BA GENMASK_ULL(63, 7)
/* …（同文件无关代码省略）… */
static int ufshcd_mcq_get_tag(struct ufs_hba *hba, struct cq_entry *cqe)
{  // <<< BUG ANCHOR
	u64 addr;

	if (hba->ufs_version >= ufshci_version(4, 1))
		return cqe->task_tag;

	/* Both UCD types must have a size that is a multiple of 128 bytes */
	BUILD_BUG_ON(sizeof(struct utp_transfer_cmd_desc) & GENMASK(6, 0));
	BUILD_BUG_ON(sizeof(struct utp_devman_cmd_desc) & GENMASK(6, 0));

	/* Bits 63:7 UCD base address, 6:5 are reserved, 4:0 is SQ ID */
	addr = le64_to_cpu(cqe->command_desc_base_addr) & CQE_UCD_BA;

	/* The devman UCD is outside the pool; return its reserved tag. */
	if (unlikely(addr == hba->devman_ucd_dma_addr))
		return hba->dev_cmd.tag;

	/* Pool entries follow the reserved tags. */
	return div_u64(addr - hba->ucdl_dma_addr, ufshcd_get_ucd_size(hba)) +
		UFSHCD_NUM_RESERVED;
}

static void ufshcd_mcq_process_cqe(struct ufs_hba *hba,
				   struct ufs_hw_queue *hwq)
{
	struct cq_entry *cqe = ufshcd_mcq_cur_cqe(hwq);

	if (cqe->command_desc_base_addr) {
		int tag = ufshcd_mcq_get_tag(hba, cqe);

		ufshcd_compl_one_cqe(hba, tag, cqe);
		/* After processed the cqe, mark it empty (invalid) entry */
		cqe->command_desc_base_addr = 0;
	} else {
		dev_err(hba->dev, "Abnormal CQ entry!\n");
	}
}

/*
 * This function is called from the UFS error handler with the UFS host
 * controller disabled (HCE = 0). Reading host controller registers, e.g. the
 * CQ tail pointer (CQTPy), may not be safe with the host controller disabled.
 * Hence, iterate over all completion queue entries. This won't result in
 * double completions because ufshcd_mcq_process_cqe() clears a CQE after it
 * has been processed.
 */
void ufshcd_mcq_compl_all_cqes_lock(struct ufs_hba *hba,
				    struct ufs_hw_queue *hwq)
{
	unsigned long flags;
	u32 entries = hwq->max_entries;

	spin_lock_irqsave(&hwq->cq_lock, flags);
	while (entries > 0) {
		ufshcd_mcq_process_cqe(hba, hwq);
		ufshcd_mcq_inc_cq_head_slot(hwq);
		entries--;
	}

	ufshcd_mcq_update_cq_tail_slot(hwq);
	hwq->cq_head_slot = hwq->cq_tail_slot;
	spin_unlock_irqrestore(&hwq->cq_lock, flags);
}
