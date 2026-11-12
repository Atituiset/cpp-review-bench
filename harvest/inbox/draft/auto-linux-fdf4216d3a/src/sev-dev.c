// AUTO-DRAFT from torvalds/linux PR #0eaee8fc9305b4ae8e75e68c30e7e93b6b367234
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#define CMD_BUF_FW_WRITABLE_MAX 2
/* …（同文件无关代码省略）… */
#define CMD_BUF_DESC_MAX (CMD_BUF_FW_WRITABLE_MAX + 1)
/* …（同文件无关代码省略）… */
enum snp_hv_fixed_pages_state {
	ALLOCATED,
	HV_FIXED,
};
/* …（同文件无关代码省略）… */
struct snp_hv_fixed_pages_entry {
	struct list_head list;
	struct page *page;
	unsigned int order;
	bool free;
	enum snp_hv_fixed_pages_state page_state;
};
/* …（同文件无关代码省略）… */
#define SEV_TMR_SIZE		(1024 * 1024)
/* …（同文件无关代码省略）… */
#define NV_LENGTH (32 * 1024)
/* …（同文件无关代码省略）… */
static int sev_wait_cmd_ioc(struct sev_device *sev,
			    unsigned int *reg, unsigned int timeout)
{
	int ret;

	/*
	 * If invoked during panic handling, local interrupts are disabled,
	 * so the PSP command completion interrupt can't be used. Poll for
	 * PSP command completion instead.
	 */
	if (irqs_disabled()) {
		unsigned long timeout_usecs = (timeout * USEC_PER_SEC) / 10;

		/* Poll for SEV command completion: */
		while (timeout_usecs--) {
			*reg = ioread32(sev->io_regs + sev->vdata->cmdresp_reg);
			if (*reg & PSP_CMDRESP_RESP)
				return 0;

			udelay(10);
		}
		return -ETIMEDOUT;
	}

	ret = wait_event_timeout(sev->int_queue,
			sev->int_rcvd, timeout * HZ);
	if (!ret)
		return -ETIMEDOUT;

	*reg = ioread32(sev->io_regs + sev->vdata->cmdresp_reg);

	return 0;
}
/* …（同文件无关代码省略）… */
static int sev_cmd_buffer_len(int cmd)
{
	switch (cmd) {
	case SEV_CMD_INIT:			return sizeof(struct sev_data_init);
	case SEV_CMD_INIT_EX:                   return sizeof(struct sev_data_init_ex);
	case SEV_CMD_SNP_SHUTDOWN_EX:		return sizeof(struct sev_data_snp_shutdown_ex);
	case SEV_CMD_SNP_INIT_EX:		return sizeof(struct sev_data_snp_init_ex);
	case SEV_CMD_PLATFORM_STATUS:		return sizeof(struct sev_user_data_status);
	case SEV_CMD_PEK_CSR:			return sizeof(struct sev_data_pek_csr);
	case SEV_CMD_PEK_CERT_IMPORT:		return sizeof(struct sev_data_pek_cert_import);
	case SEV_CMD_PDH_CERT_EXPORT:		return sizeof(struct sev_data_pdh_cert_export);
	case SEV_CMD_LAUNCH_START:		return sizeof(struct sev_data_launch_start);
	case SEV_CMD_LAUNCH_UPDATE_DATA:	return sizeof(struct sev_data_launch_update_data);
	case SEV_CMD_LAUNCH_UPDATE_VMSA:	return sizeof(struct sev_data_launch_update_vmsa);
	case SEV_CMD_LAUNCH_FINISH:		return sizeof(struct sev_data_launch_finish);
	case SEV_CMD_LAUNCH_MEASURE:		return sizeof(struct sev_data_launch_measure);
	case SEV_CMD_ACTIVATE:			return sizeof(struct sev_data_activate);
	case SEV_CMD_DEACTIVATE:		return sizeof(struct sev_data_deactivate);
	case SEV_CMD_DECOMMISSION:		return sizeof(struct sev_data_decommission);
	case SEV_CMD_GUEST_STATUS:		return sizeof(struct sev_data_guest_status);
	case SEV_CMD_DBG_DECRYPT:		return sizeof(struct sev_data_dbg);
	case SEV_CMD_DBG_ENCRYPT:		return sizeof(struct sev_data_dbg);
	case SEV_CMD_SEND_START:		return sizeof(struct sev_data_send_start);
	case SEV_CMD_SEND_UPDATE_DATA:		return sizeof(struct sev_data_send_update_data);
	case SEV_CMD_SEND_UPDATE_VMSA:		return sizeof(struct sev_data_send_update_vmsa);
	case SEV_CMD_SEND_FINISH:		return sizeof(struct sev_data_send_finish);
	case SEV_CMD_RECEIVE_START:		return sizeof(struct sev_data_receive_start);
	case SEV_CMD_RECEIVE_FINISH:		return sizeof(struct sev_data_receive_finish);
	case SEV_CMD_RECEIVE_UPDATE_DATA:	return sizeof(struct sev_data_receive_update_data);
	case SEV_CMD_RECEIVE_UPDATE_VMSA:	return sizeof(struct sev_data_receive_update_vmsa);
	case SEV_CMD_LAUNCH_UPDATE_SECRET:	return sizeof(struct sev_data_launch_secret);
	case SEV_CMD_DOWNLOAD_FIRMWARE:		return sizeof(struct sev_data_download_firmware);
	case SEV_CMD_GET_ID:			return sizeof(struct sev_data_get_id);
	case SEV_CMD_ATTESTATION_REPORT:	return sizeof(struct sev_data_attestation_report);
	case SEV_CMD_SEND_CANCEL:		return sizeof(struct sev_data_send_cancel);
	case SEV_CMD_SNP_GCTX_CREATE:		return sizeof(struct sev_data_snp_addr);
	case SEV_CMD_SNP_LAUNCH_START:		return sizeof(struct sev_data_snp_launch_start);
	case SEV_CMD_SNP_LAUNCH_UPDATE:		return sizeof(struct sev_data_snp_launch_update);
	case SEV_CMD_SNP_ACTIVATE:		return sizeof(struct sev_data_snp_activate);
	case SEV_CMD_SNP_DECOMMISSION:		return sizeof(struct sev_data_snp_addr);
	case SEV_CMD_SNP_PAGE_RECLAIM:		return sizeof(struct sev_data_snp_page_reclaim);
	case SEV_CMD_SNP_GUEST_STATUS:		return sizeof(struct sev_data_snp_guest_status);
	case SEV_CMD_SNP_LAUNCH_FINISH:		return sizeof(struct sev_data_snp_launch_finish);
	case SEV_CMD_SNP_DBG_DECRYPT:		return sizeof(struct sev_data_snp_dbg);
	case SEV_CMD_SNP_DBG_ENCRYPT:		return sizeof(struct sev_data_snp_dbg);
	case SEV_CMD_SNP_VERIFY_MITIGATION:	return sizeof(struct sev_data_snp_verify_mitigation);
	case SEV_CMD_SNP_PAGE_UNSMASH:		return sizeof(struct sev_data_snp_page_unsmash);
	case SEV_CMD_SNP_PLATFORM_STATUS:	return sizeof(struct sev_data_snp_addr);
	case SEV_CMD_SNP_GUEST_REQUEST:		return sizeof(struct sev_data_snp_guest_request);
	case SEV_CMD_SNP_CONFIG:		return sizeof(struct sev_user_data_snp_config);
	case SEV_CMD_SNP_COMMIT:		return sizeof(struct sev_data_snp_commit);
	case SEV_CMD_SNP_FEATURE_INFO:		return sizeof(struct sev_data_snp_feature_info);
	case SEV_CMD_SNP_VLEK_LOAD:		return sizeof(struct sev_user_data_snp_vlek_load);
	default:				return sev_tio_cmd_buffer_len(cmd);
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static struct file *open_file_as_root(const char *filename, int flags, umode_t mode)
{
	CLASS(prepare_creds, cred)();
	if (!cred)
		return ERR_PTR(-ENOMEM);

	cred->fsuid = GLOBAL_ROOT_UID;

	scoped_with_init_fs() {
		scoped_with_creds(cred)
			return filp_open(filename, flags, mode);
	}
}
/* …（同文件无关代码省略）… */
static int sev_read_init_ex_file(void)
{
	struct sev_device *sev = psp_master->sev_data;
	struct file *fp;
	ssize_t nread;

	lockdep_assert_held(&sev_cmd_mutex);

	if (!sev_init_ex_buffer)
		return -EOPNOTSUPP;

	fp = open_file_as_root(init_ex_path, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		int ret = PTR_ERR(fp);

		if (ret == -ENOENT) {
			dev_info(sev->dev,
				"SEV: %s does not exist and will be created later.\n",
				init_ex_path);
			ret = 0;
		} else {
			dev_err(sev->dev,
				"SEV: could not open %s for read, error %d\n",
				init_ex_path, ret);
		}
		return ret;
	}

	nread = kernel_read(fp, sev_init_ex_buffer, NV_LENGTH, NULL);
	if (nread != NV_LENGTH) {
		dev_info(sev->dev,
			"SEV: could not read %u bytes to non volatile memory area, ret %ld\n",
			NV_LENGTH, nread);
	}

	dev_dbg(sev->dev, "SEV: read %ld bytes from NV file\n", nread);
	filp_close(fp, NULL);

	return 0;
}
/* …（同文件无关代码省略）… */
static int sev_write_init_ex_file(void)
{
	struct sev_device *sev = psp_master->sev_data;
	struct file *fp;
	loff_t offset = 0;
	ssize_t nwrite;

	lockdep_assert_held(&sev_cmd_mutex);

	if (!sev_init_ex_buffer)
		return 0;

	fp = open_file_as_root(init_ex_path, O_CREAT | O_WRONLY, 0600);
	if (IS_ERR(fp)) {
		int ret = PTR_ERR(fp);

		dev_err(sev->dev,
			"SEV: could not open file for write, error %d\n",
			ret);
		return ret;
	}

	nwrite = kernel_write(fp, sev_init_ex_buffer, NV_LENGTH, &offset);
	vfs_fsync(fp, 0);
	filp_close(fp, NULL);

	if (nwrite != NV_LENGTH) {
		dev_err(sev->dev,
			"SEV: failed to write %u bytes to non volatile memory area, ret %ld\n",
			NV_LENGTH, nwrite);
		return -EIO;
	}

	dev_dbg(sev->dev, "SEV: write successful to NV file\n");

	return 0;
}
/* …（同文件无关代码省略）… */
static int sev_write_init_ex_file_if_required(int cmd_id)
{
	lockdep_assert_held(&sev_cmd_mutex);

	if (!sev_init_ex_buffer)
		return 0;

	/*
	 * Only a few platform commands modify the SPI/NV area, but none of the
	 * non-platform commands do. Only INIT(_EX), PLATFORM_RESET, PEK_GEN,
	 * PEK_CERT_IMPORT, and PDH_GEN do.
	 */
	switch (cmd_id) {
	case SEV_CMD_FACTORY_RESET:
	case SEV_CMD_INIT_EX:
	case SEV_CMD_PDH_GEN:
	case SEV_CMD_PEK_CERT_IMPORT:
	case SEV_CMD_PEK_GEN:
		break;
	default:
		return 0;
	}

	return sev_write_init_ex_file();
}
/* …（同文件无关代码省略）… */
int snp_reclaim_pages(unsigned long paddr, unsigned int npages, bool locked)
{
	int ret, err, i;

	paddr = __sme_clr(ALIGN_DOWN(paddr, PAGE_SIZE));

	for (i = 0; i < npages; i++, paddr += PAGE_SIZE) {
		struct sev_data_snp_page_reclaim data = {0};

		data.paddr = paddr;

		if (locked)
			ret = __sev_do_cmd_locked(SEV_CMD_SNP_PAGE_RECLAIM, &data, &err);
		else
			ret = sev_do_cmd(SEV_CMD_SNP_PAGE_RECLAIM, &data, &err);

		if (ret)
			goto cleanup;

		ret = rmp_make_shared(__phys_to_pfn(paddr), PG_LEVEL_4K);
		if (ret)
			goto cleanup;
	}

	return 0;

cleanup:
	/*
	 * If there was a failure reclaiming the page then it is no longer safe
	 * to release it back to the system; leak it instead.
	 */
	snp_leak_pages(__phys_to_pfn(paddr), npages - i);
	return ret;
}
/* …（同文件无关代码省略）… */
static int rmp_mark_pages_firmware(unsigned long paddr, unsigned int npages, bool locked)
{
	unsigned long pfn = __sme_clr(paddr) >> PAGE_SHIFT;
	int rc, i;

	for (i = 0; i < npages; i++, pfn++) {
		rc = rmp_make_private(pfn, 0, PG_LEVEL_4K, 0, true);
		if (rc)
			goto cleanup;
	}

	return 0;

cleanup:
	/*
	 * Try unrolling the firmware state changes by
	 * reclaiming the pages which were already changed to the
	 * firmware state.
	 */
	snp_reclaim_pages(paddr, i, locked);

	return rc;
}
/* …（同文件无关代码省略）… */
static struct page *__snp_alloc_firmware_pages(gfp_t gfp_mask, int order, bool locked)
{
	unsigned long npages = 1ul << order, paddr;
	struct sev_device *sev;
	struct page *page;

	if (!psp_master || !psp_master->sev_data)
		return NULL;

	page = alloc_pages(gfp_mask, order);
	if (!page)
		return NULL;

	/* If SEV-SNP is initialized then add the page in RMP table. */
	sev = psp_master->sev_data;
	if (!sev->snp_initialized)
		return page;

	paddr = __pa((unsigned long)page_address(page));
	if (rmp_mark_pages_firmware(paddr, npages, locked))
		return NULL;

	return page;
}
/* …（同文件无关代码省略）… */
static void __snp_free_firmware_pages(struct page *page, int order, bool locked)
{
	struct sev_device *sev = psp_master->sev_data;
	unsigned long paddr, npages = 1ul << order;

	if (!page)
		return;

	paddr = __pa((unsigned long)page_address(page));
	if (sev->snp_initialized &&
	    snp_reclaim_pages(paddr, npages, locked))
		return;

	__free_pages(page, order);
}
/* …（同文件无关代码省略）… */
static void *sev_fw_alloc(unsigned long len)
{
	struct page *page;

	page = __snp_alloc_firmware_pages(GFP_KERNEL, get_order(len), true);
	if (!page)
		return NULL;

	return page_address(page);
}
/* …（同文件无关代码省略）… */
struct cmd_buf_desc {
	u64 *paddr_ptr;
	u64 paddr_orig;
	u32 len;
	bool guest_owned;
};
/* …（同文件无关代码省略）… */
static void snp_populate_cmd_buf_desc_list(int cmd, void *cmd_buf,
					   struct cmd_buf_desc *desc_list)
{
	switch (cmd) {
	case SEV_CMD_PDH_CERT_EXPORT: {
		struct sev_data_pdh_cert_export *data = cmd_buf;

		desc_list[0].paddr_ptr = &data->pdh_cert_address;
		desc_list[0].len = data->pdh_cert_len;
		desc_list[1].paddr_ptr = &data->cert_chain_address;
		desc_list[1].len = data->cert_chain_len;
		break;
	}
	case SEV_CMD_GET_ID: {
		struct sev_data_get_id *data = cmd_buf;

		desc_list[0].paddr_ptr = &data->address;
		desc_list[0].len = data->len;
		break;
	}
	case SEV_CMD_PEK_CSR: {
		struct sev_data_pek_csr *data = cmd_buf;

		desc_list[0].paddr_ptr = &data->address;
		desc_list[0].len = data->len;
		break;
	}
	case SEV_CMD_LAUNCH_UPDATE_DATA: {
		struct sev_data_launch_update_data *data = cmd_buf;

		desc_list[0].paddr_ptr = &data->address;
		desc_list[0].len = data->len;
		desc_list[0].guest_owned = true;
		break;
	}
	case SEV_CMD_LAUNCH_UPDATE_VMSA: {
		struct sev_data_launch_update_vmsa *data = cmd_buf;

		desc_list[0].paddr_ptr = &data->address;
		desc_list[0].len = data->len;
		desc_list[0].guest_owned = true;
		break;
	}
	case SEV_CMD_LAUNCH_MEASURE: {
		struct sev_data_launch_measure *data = cmd_buf;

		desc_list[0].paddr_ptr = &data->address;
		desc_list[0].len = data->len;
		break;
	}
	case SEV_CMD_LAUNCH_UPDATE_SECRET: {
		struct sev_data_launch_secret *data = cmd_buf;

		desc_list[0].paddr_ptr = &data->guest_address;
		desc_list[0].len = data->guest_len;
		desc_list[0].guest_owned = true;
		break;
	}
	case SEV_CMD_DBG_DECRYPT: {
		struct sev_data_dbg *data = cmd_buf;

		desc_list[0].paddr_ptr = &data->dst_addr;
		desc_list[0].len = data->len;
		desc_list[0].guest_owned = true;
		break;
	}
	case SEV_CMD_DBG_ENCRYPT: {
		struct sev_data_dbg *data = cmd_buf;

		desc_list[0].paddr_ptr = &data->dst_addr;
		desc_list[0].len = data->len;
		desc_list[0].guest_owned = true;
		break;
	}
	case SEV_CMD_ATTESTATION_REPORT: {
		struct sev_data_attestation_report *data = cmd_buf;

		desc_list[0].paddr_ptr = &data->address;
		desc_list[0].len = data->len;
		break;
	}
	case SEV_CMD_SEND_START: {
		struct sev_data_send_start *data = cmd_buf;

		desc_list[0].paddr_ptr = &data->session_address;
		desc_list[0].len = data->session_len;
		break;
	}
	case SEV_CMD_SEND_UPDATE_DATA: {
		struct sev_data_send_update_data *data = cmd_buf;

		desc_list[0].paddr_ptr = &data->hdr_address;
		desc_list[0].len = data->hdr_len;
		desc_list[1].paddr_ptr = &data->trans_address;
		desc_list[1].len = data->trans_len;
		break;
	}
	case SEV_CMD_SEND_UPDATE_VMSA: {
		struct sev_data_send_update_vmsa *data = cmd_buf;

		desc_list[0].paddr_ptr = &data->hdr_address;
		desc_list[0].len = data->hdr_len;
		desc_list[1].paddr_ptr = &data->trans_address;
		desc_list[1].len = data->trans_len;
		break;
	}
	case SEV_CMD_RECEIVE_UPDATE_DATA: {
		struct sev_data_receive_update_data *data = cmd_buf;

		desc_list[0].paddr_ptr = &data->guest_address;
		desc_list[0].len = data->guest_len;
		desc_list[0].guest_owned = true;
		break;
	}
	case SEV_CMD_RECEIVE_UPDATE_VMSA: {
		struct sev_data_receive_update_vmsa *data = cmd_buf;

		desc_list[0].paddr_ptr = &data->guest_address;
		desc_list[0].len = data->guest_len;
		desc_list[0].guest_owned = true;
		break;
	}
	default:
		break;
	}
}
/* …（同文件无关代码省略）… */
static int snp_map_cmd_buf_desc(struct cmd_buf_desc *desc)
{
	unsigned int npages;

	if (!desc->len)
		return 0;

	/* Allocate a bounce buffer if this isn't a guest owned page. */
	if (!desc->guest_owned) {
		struct page *page;

		page = alloc_pages(GFP_KERNEL_ACCOUNT, get_order(desc->len));
		if (!page) {
			pr_warn("Failed to allocate bounce buffer for SEV legacy command.\n");
			return -ENOMEM;
		}

		desc->paddr_orig = *desc->paddr_ptr;
		*desc->paddr_ptr = __psp_pa(page_to_virt(page));
	}

	npages = PAGE_ALIGN(desc->len) >> PAGE_SHIFT;

	/* Transition the buffer to firmware-owned. */
	if (rmp_mark_pages_firmware(*desc->paddr_ptr, npages, true)) {
		pr_warn("Error moving pages to firmware-owned state for SEV legacy command.\n");
		return -EFAULT;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static int snp_unmap_cmd_buf_desc(struct cmd_buf_desc *desc)
{
	unsigned int npages;

	if (!desc->len)
		return 0;

	npages = PAGE_ALIGN(desc->len) >> PAGE_SHIFT;

	/* Transition the buffers back to hypervisor-owned. */
	if (snp_reclaim_pages(*desc->paddr_ptr, npages, true)) {
		pr_warn("Failed to reclaim firmware-owned pages while issuing SEV legacy command.\n");
		return -EFAULT;
	}

	/* Copy data from bounce buffer and then free it. */
	if (!desc->guest_owned) {
		void *bounce_buf = __va(__sme_clr(*desc->paddr_ptr));
		void *dst_buf = __va(__sme_clr(desc->paddr_orig));

		memcpy(dst_buf, bounce_buf, desc->len);
		__free_pages(virt_to_page(bounce_buf), get_order(desc->len));

		/* Restore the original address in the command buffer. */
		*desc->paddr_ptr = desc->paddr_orig;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static int snp_map_cmd_buf_desc_list(int cmd, void *cmd_buf, struct cmd_buf_desc *desc_list)
{
	int i;

	snp_populate_cmd_buf_desc_list(cmd, cmd_buf, desc_list);

	for (i = 0; i < CMD_BUF_DESC_MAX; i++) {
		struct cmd_buf_desc *desc = &desc_list[i];

		if (!desc->paddr_ptr)
			break;

		if (snp_map_cmd_buf_desc(desc))
			goto err_unmap;
	}

	return 0;

err_unmap:
	for (i--; i >= 0; i--)
		snp_unmap_cmd_buf_desc(&desc_list[i]);

	return -EFAULT;
}
/* …（同文件无关代码省略）… */
static int snp_unmap_cmd_buf_desc_list(struct cmd_buf_desc *desc_list)
{
	int i, ret = 0;

	for (i = 0; i < CMD_BUF_DESC_MAX; i++) {
		struct cmd_buf_desc *desc = &desc_list[i];

		if (!desc->paddr_ptr)
			break;

		if (snp_unmap_cmd_buf_desc(&desc_list[i]))
			ret = -EFAULT;
	}

	return ret;
}
/* …（同文件无关代码省略）… */
static bool sev_cmd_buf_writable(int cmd)
{
	switch (cmd) {
	case SEV_CMD_PLATFORM_STATUS:
	case SEV_CMD_GUEST_STATUS:
	case SEV_CMD_LAUNCH_START:
	case SEV_CMD_RECEIVE_START:
	case SEV_CMD_LAUNCH_MEASURE:
	case SEV_CMD_SEND_START:
	case SEV_CMD_SEND_UPDATE_DATA:
	case SEV_CMD_SEND_UPDATE_VMSA:
	case SEV_CMD_PEK_CSR:
	case SEV_CMD_PDH_CERT_EXPORT:
	case SEV_CMD_GET_ID:
	case SEV_CMD_ATTESTATION_REPORT:
		return true;
	default:
		return false;
	}
}
/* …（同文件无关代码省略）… */
static bool snp_legacy_handling_needed(int cmd)
{
	struct sev_device *sev = psp_master->sev_data;

	return cmd < SEV_CMD_SNP_INIT && sev->snp_initialized;
}
/* …（同文件无关代码省略）… */
static int snp_prep_cmd_buf(int cmd, void *cmd_buf, struct cmd_buf_desc *desc_list)
{
	if (!snp_legacy_handling_needed(cmd))
		return 0;

	if (snp_map_cmd_buf_desc_list(cmd, cmd_buf, desc_list))
		return -EFAULT;

	/*
	 * Before command execution, the command buffer needs to be put into
	 * the firmware-owned state.
	 */
	if (sev_cmd_buf_writable(cmd)) {
		if (rmp_mark_pages_firmware(__pa(cmd_buf), 1, true))
			return -EFAULT;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static int snp_reclaim_cmd_buf(int cmd, void *cmd_buf)
{
	if (!snp_legacy_handling_needed(cmd))
		return 0;

	/*
	 * After command completion, the command buffer needs to be put back
	 * into the hypervisor-owned state.
	 */
	if (sev_cmd_buf_writable(cmd))
		if (snp_reclaim_pages(__pa(cmd_buf), 1, true))
			return -EFAULT;

	return 0;
}
/* …（同文件无关代码省略）… */
int __sev_do_cmd_locked(int cmd, void *data, int *psp_ret)
{
	struct cmd_buf_desc desc_list[CMD_BUF_DESC_MAX] = {0};
	struct psp_device *psp = psp_master;
	struct sev_device *sev;
	unsigned int cmdbuff_hi, cmdbuff_lo;
	unsigned int phys_lsb, phys_msb;
	unsigned int reg;
	void *cmd_buf;
	int buf_len;
	int ret = 0;

	if (!psp || !psp->sev_data)
		return -ENODEV;

	if (psp_dead)
		return -EBUSY;

	sev = psp->sev_data;

	buf_len = sev_cmd_buffer_len(cmd);
	if (WARN_ON_ONCE(!data != !buf_len))
		return -EINVAL;

	/*
	 * Copy the incoming data to driver's scratch buffer as __pa() will not
	 * work for some memory, e.g. vmalloc'd addresses, and @data may not be
	 * physically contiguous.
	 */
	if (data) {
		/*
		 * Commands are generally issued one at a time and require the
		 * sev_cmd_mutex, but there could be recursive firmware requests
		 * due to SEV_CMD_SNP_PAGE_RECLAIM needing to be issued while
		 * preparing buffers for another command. This is the only known
		 * case of nesting in the current code, so exactly one
		 * additional command buffer is available for that purpose.
		 */
		if (!sev->cmd_buf_active) {
			cmd_buf = sev->cmd_buf;
			sev->cmd_buf_active = true;
		} else if (!sev->cmd_buf_backup_active) {
			cmd_buf = sev->cmd_buf_backup;
			sev->cmd_buf_backup_active = true;
		} else {
			dev_err(sev->dev,
				"SEV: too many firmware commands in progress, no command buffers available.\n");
			return -EBUSY;
		}

		memcpy(cmd_buf, data, buf_len);

		/*
		 * The behavior of the SEV-legacy commands is altered when the
		 * SNP firmware is in the INIT state.
		 */
		ret = snp_prep_cmd_buf(cmd, cmd_buf, desc_list);
		if (ret) {
			dev_err(sev->dev,
				"SEV: failed to prepare buffer for legacy command 0x%x. Error: %d\n",
				cmd, ret);
			return ret;
		}
	} else {
		cmd_buf = sev->cmd_buf;
	}

	/* Get the physical address of the command buffer */
	phys_lsb = data ? lower_32_bits(__psp_pa(cmd_buf)) : 0;
	phys_msb = data ? upper_32_bits(__psp_pa(cmd_buf)) : 0;

	dev_dbg(sev->dev, "sev command id %#x buffer 0x%08x%08x timeout %us\n",
		cmd, phys_msb, phys_lsb, psp_timeout);

	print_hex_dump_debug("(in):  ", DUMP_PREFIX_OFFSET, 16, 2, data,
			     buf_len, false);

	iowrite32(phys_lsb, sev->io_regs + sev->vdata->cmdbuff_addr_lo_reg);
	iowrite32(phys_msb, sev->io_regs + sev->vdata->cmdbuff_addr_hi_reg);

	sev->int_rcvd = 0;

	reg = FIELD_PREP(SEV_CMDRESP_CMD, cmd);

	/*
	 * If invoked during panic handling, local interrupts are disabled so
	 * the PSP command completion interrupt can't be used.
	 * sev_wait_cmd_ioc() already checks for interrupts disabled and
	 * polls for PSP command completion.  Ensure we do not request an
	 * interrupt from the PSP if irqs disabled.
	 */
	if (!irqs_disabled())
		reg |= SEV_CMDRESP_IOC;

	iowrite32(reg, sev->io_regs + sev->vdata->cmdresp_reg);

	/* wait for command completion */
	ret = sev_wait_cmd_ioc(sev, &reg, psp_timeout);
	if (ret) {
		if (psp_ret)
			*psp_ret = 0;

		dev_err(sev->dev, "sev command %#x timed out, disabling PSP\n", cmd);
		psp_dead = true;

		return ret;
	}

	psp_timeout = psp_cmd_timeout;

	if (psp_ret)
		*psp_ret = FIELD_GET(PSP_CMDRESP_STS, reg);

	if (FIELD_GET(PSP_CMDRESP_STS, reg)) {
		dev_dbg(sev->dev, "sev command %#x failed (%#010lx)\n",
			cmd, FIELD_GET(PSP_CMDRESP_STS, reg));

		/*
		 * PSP firmware may report additional error information in the
		 * command buffer registers on error. Print contents of command
		 * buffer registers if they changed.
		 */
		cmdbuff_hi = ioread32(sev->io_regs + sev->vdata->cmdbuff_addr_hi_reg);
		cmdbuff_lo = ioread32(sev->io_regs + sev->vdata->cmdbuff_addr_lo_reg);
		if (cmdbuff_hi != phys_msb || cmdbuff_lo != phys_lsb) {
			dev_dbg(sev->dev, "Additional error information reported in cmdbuff:");
			dev_dbg(sev->dev, "  cmdbuff hi: %#010x\n", cmdbuff_hi);
			dev_dbg(sev->dev, "  cmdbuff lo: %#010x\n", cmdbuff_lo);
		}
		ret = -EIO;
	} else {
		ret = sev_write_init_ex_file_if_required(cmd);
	}

	/*
	 * Copy potential output from the PSP back to data.  Do this even on
	 * failure in case the caller wants to glean something from the error.
	 */
	if (data) {
		int ret_reclaim;
		/*
		 * Restore the page state after the command completes.
		 */
		ret_reclaim = snp_reclaim_cmd_buf(cmd, cmd_buf);
		if (ret_reclaim) {
			dev_err(sev->dev,
				"SEV: failed to reclaim buffer for legacy command %#x. Error: %d\n",
				cmd, ret_reclaim);
			return ret_reclaim;
		}

		memcpy(data, cmd_buf, buf_len);

		if (sev->cmd_buf_backup_active)
			sev->cmd_buf_backup_active = false;
		else
			sev->cmd_buf_active = false;

		if (snp_unmap_cmd_buf_desc_list(desc_list))
			return -EFAULT;
	}

	print_hex_dump_debug("(out): ", DUMP_PREFIX_OFFSET, 16, 2, data,
			     buf_len, false);

	return ret;
}
/* …（同文件无关代码省略）… */
int sev_do_cmd(int cmd, void *data, int *psp_ret)
{
	int rc;

	mutex_lock(&sev_cmd_mutex);
	rc = __sev_do_cmd_locked(cmd, data, psp_ret);
	mutex_unlock(&sev_cmd_mutex);

	return rc;
}
/* …（同文件无关代码省略）… */
static int __sev_init_locked(int *error)
{
	struct sev_data_init data;

	memset(&data, 0, sizeof(data));
	if (sev_es_tmr) {
		/*
		 * Do not include the encryption mask on the physical
		 * address of the TMR (firmware should clear it anyway).
		 */
		data.tmr_address = __pa(sev_es_tmr);

		data.flags |= SEV_INIT_FLAGS_SEV_ES;
		data.tmr_len = sev_es_tmr_size;
	}

	return __sev_do_cmd_locked(SEV_CMD_INIT, &data, error);
}
/* …（同文件无关代码省略）… */
static int __sev_init_ex_locked(int *error)
{
	struct sev_data_init_ex data;

	memset(&data, 0, sizeof(data));
	data.length = sizeof(data);
	data.nv_address = __psp_pa(sev_init_ex_buffer);
	data.nv_len = NV_LENGTH;

	if (sev_es_tmr) {
		/*
		 * Do not include the encryption mask on the physical
		 * address of the TMR (firmware should clear it anyway).
		 */
		data.tmr_address = __pa(sev_es_tmr);

		data.flags |= SEV_INIT_FLAGS_SEV_ES;
		data.tmr_len = sev_es_tmr_size;
	}

	return __sev_do_cmd_locked(SEV_CMD_INIT_EX, &data, error);
}
/* …（同文件无关代码省略）… */
static inline int __sev_do_init_locked(int *psp_ret)
{
	if (sev_init_ex_buffer)
		return __sev_init_ex_locked(psp_ret);
	else
		return __sev_init_locked(psp_ret);
}
/* …（同文件无关代码省略）… */
static void snp_hv_fixed_pages_state_update(struct sev_device *sev,
					    enum snp_hv_fixed_pages_state page_state)
{
	struct snp_hv_fixed_pages_entry *entry;

	/* List is protected by sev_cmd_mutex */
	lockdep_assert_held(&sev_cmd_mutex);

	if (list_empty(&snp_hv_fixed_pages))
		return;

	list_for_each_entry(entry, &snp_hv_fixed_pages, list)
		entry->page_state = page_state;
}
/* …（同文件无关代码省略）… */
	list_for_each_entry(entry, &snp_hv_fixed_pages, list) {
		/* Hypervisor fixed page allocator implements exact fit policy */
		if (entry->order == order && entry->free) {
			entry->free = false;
			memset(page_address(entry->page), 0,
			       (1 << entry->order) * PAGE_SIZE);
			return entry->page;
		}
	}
/* …（同文件无关代码省略）… */
static void snp_leak_hv_fixed_pages(void)
{
	struct snp_hv_fixed_pages_entry *entry, *nentry;

	/* List is protected by sev_cmd_mutex */
	lockdep_assert_held(&sev_cmd_mutex);

	if (list_empty(&snp_hv_fixed_pages))
		return;

	list_for_each_entry_safe(entry, nentry, &snp_hv_fixed_pages, list) {
		if (entry->free && entry->page_state != HV_FIXED)
			__free_pages(entry->page, entry->order);
		else
			__snp_leak_pages(page_to_pfn(entry->page),
					 1 << entry->order, false);

		list_del(&entry->list);
		kfree(entry);
	}
}
/* …（同文件无关代码省略）… */
static void __sev_platform_init_handle_tmr(struct sev_device *sev)
{
	if (sev_es_tmr)
		return;

	/* Obtain the TMR memory area for SEV-ES use */
	sev_es_tmr = sev_fw_alloc(sev_es_tmr_size);
	if (sev_es_tmr) {
		/* Must flush the cache before giving it to the firmware */
		if (!sev->snp_initialized)
			clflush_cache_range(sev_es_tmr, sev_es_tmr_size);
	} else {
			dev_warn(sev->dev, "SEV: TMR allocation failed, SEV-ES support unavailable\n");
	}
}
/* …（同文件无关代码省略）… */
static int __sev_platform_init_handle_init_ex_path(struct sev_device *sev)
{
	struct page *page;
	int rc;

	if (!init_ex_path)
		return 0;

	if (sev_init_ex_buffer)
		return 0;

	page = alloc_pages(GFP_KERNEL | __GFP_ZERO, get_order(NV_LENGTH));
	if (!page) {
		dev_err(sev->dev, "SEV: INIT_EX NV memory allocation failed\n");
		return -ENOMEM;
	}

	sev_init_ex_buffer = page_address(page);

	rc = sev_read_init_ex_file();
	if (rc)
		goto err_free;

	/* If SEV-SNP is initialized, transition to firmware page. */
	if (sev->snp_initialized) {
		unsigned long npages;

		npages = 1UL << get_order(NV_LENGTH);
		if (rmp_mark_pages_firmware(__pa(sev_init_ex_buffer), npages, true)) {
			dev_err(sev->dev, "SEV: INIT_EX NV memory page state change failed.\n");
			rc = -ENOMEM;
			/*
			 * Pages can be in an inconsistent state, don't release them back to the
			 * system.
			 */
			goto err_reset;
		}
	}

	return 0;

err_free:
	__free_pages(page, get_order(NV_LENGTH));
err_reset:
	sev_init_ex_buffer = NULL;
	return rc;
}
/* …（同文件无关代码省略）… */
static int __sev_platform_init_locked(int *error)
{
	int rc, psp_ret, dfflush_error;
	struct sev_device *sev;

	psp_ret = dfflush_error = SEV_RET_NO_FW_CALL;

	if (!psp_master || !psp_master->sev_data)
		return -ENODEV;

	sev = psp_master->sev_data;

	if (sev->sev_plat_status.state == SEV_STATE_INIT)
		return 0;

	__sev_platform_init_handle_tmr(sev);

	rc = __sev_platform_init_handle_init_ex_path(sev);
	if (rc)
		return rc;

	rc = __sev_do_init_locked(&psp_ret);
	if (rc && psp_ret == SEV_RET_SECURE_DATA_INVALID) {
		/*
		 * Initialization command returned an integrity check failure
		 * status code, meaning that firmware load and validation of SEV
		 * related persistent data has failed. Retrying the
		 * initialization function should succeed by replacing the state
		 * with a reset state.
		 */
		dev_err(sev->dev,
"SEV: retrying INIT command because of SECURE_DATA_INVALID error. Retrying once to reset PSP SEV state.");
		rc = __sev_do_init_locked(&psp_ret);
	}

	if (error)
		*error = psp_ret;

	if (rc) {
		dev_err(sev->dev, "SEV: %s failed %#x, rc %d\n",
			sev_init_ex_buffer ? "INIT_EX" : "INIT", psp_ret, rc);
		return rc;
	}

	sev->sev_plat_status.state = SEV_STATE_INIT;

	/* Prepare for first SEV guest launch after INIT */
	wbinvd_on_all_cpus();
	rc = __sev_do_cmd_locked(SEV_CMD_DF_FLUSH, NULL, &dfflush_error);
	if (rc) {
		dev_err(sev->dev, "SEV: DF_FLUSH failed %#x, rc %d\n",
			dfflush_error, rc);
		return rc;
	}

	dev_dbg(sev->dev, "SEV firmware initialized\n");

	dev_info(sev->dev, "SEV API:%d.%d build:%d\n", sev->api_major,
		 sev->api_minor, sev->build);

	return 0;
}
/* …（同文件无关代码省略）… */
static int __sev_platform_shutdown_locked(int *error)
{
	struct psp_device *psp = psp_master;
	struct sev_device *sev;
	int ret;

	if (!psp || !psp->sev_data)
		return 0;

	sev = psp->sev_data;

	if (sev->sev_plat_status.state == SEV_STATE_UNINIT)
		return 0;

	ret = __sev_do_cmd_locked(SEV_CMD_SHUTDOWN, NULL, error);
	if (ret) {
		dev_err(sev->dev, "SEV: failed to SHUTDOWN error %#x, rc %d\n",
			*error, ret);
		return ret;
	}

	sev->sev_plat_status.state = SEV_STATE_UNINIT;
	dev_dbg(sev->dev, "SEV firmware shutdown\n");

	return ret;
}
/* …（同文件无关代码省略）… */
static int sev_get_platform_state(int *state, int *error)
{
	struct sev_user_data_status data;
	int rc;

	rc = __sev_do_cmd_locked(SEV_CMD_PLATFORM_STATUS, &data, error);
	if (rc)
		return rc;

	*state = data.state;
	return rc;
}
/* …（同文件无关代码省略）… */
static int sev_move_to_init_state(struct sev_issue_cmd *argp, bool *shutdown_required)
{
	int rc;

	rc = __sev_platform_init_locked(&argp->error);
	if (rc)
		return rc;

	*shutdown_required = true;

	return 0;
}
/* …（同文件无关代码省略）… */
	if (!writable)
		return -EPERM;

	/*
	 * The SEV spec requires that FACTORY_RESET must be issued in
	 * UNINIT state. Before we go further lets check if any guest is
/* …（同文件无关代码省略）… */
	 */
	rc = sev_get_platform_state(&state, &argp->error);
	if (rc)
		return rc;

	if (state == SEV_STATE_WORKING)
		return -EBUSY;

	if (state == SEV_STATE_INIT) {
		rc = __sev_platform_shutdown_locked(&argp->error);
		if (rc)
			return rc;
	}

	return __sev_do_cmd_locked(SEV_CMD_FACTORY_RESET, NULL, &argp->error);
}

static int sev_ioctl_do_platform_status(struct sev_issue_cmd *argp)
{
	struct sev_user_data_status data;
	int ret;

	memset(&data, 0, sizeof(data));

	ret = __sev_do_cmd_locked(SEV_CMD_PLATFORM_STATUS, &data, &argp->error);
	if (ret)
		return ret;

	if (copy_to_user((void __user *)argp->data, &data, sizeof(data)))
		ret = -EFAULT;

	return ret;
}
/* …（同文件无关代码省略）… */
	if (!writable)
		return -EPERM;

	if (sev->sev_plat_status.state == SEV_STATE_UNINIT) {
		rc = sev_move_to_init_state(argp, &shutdown_required);
		if (rc)
			return rc;
	}

	rc = __sev_do_cmd_locked(cmd, NULL, &argp->error);

	if (shutdown_required)
		__sev_firmware_shutdown(sev, false);

	return rc;
}

/* …（同文件无关代码省略）… */
	data.len = input.length;

cmd:
	if (sev->sev_plat_status.state == SEV_STATE_UNINIT) {
		ret = sev_move_to_init_state(argp, &shutdown_required);
		if (ret)
			goto e_free_blob;
	}

	ret = __sev_do_cmd_locked(SEV_CMD_PEK_CSR, &data, &argp->error);

	/*
	 * Firmware will returns the length of the CSR blob (either the minimum
	 * required length or the actual length written), return it to the user.
/* …（同文件无关代码省略）… */
	}

e_free_blob:
	if (shutdown_required)
		__sev_firmware_shutdown(sev, false);

	kfree(blob);
	return ret;
}
/* …（同文件无关代码省略）… */
void *psp_copy_user_blob(u64 uaddr, u32 len)
{
	if (!uaddr || !len)
		return ERR_PTR(-EINVAL);

	/* verify that blob length does not exceed our limit */
	if (len > SEV_FW_BLOB_MAX_SIZE)
		return ERR_PTR(-EINVAL);

	return memdup_user((void __user *)uaddr, len);
}
/* …（同文件无关代码省略）… */
static int __sev_snp_shutdown_locked(int *error, bool panic)
{
	struct psp_device *psp = psp_master;
	struct sev_device *sev;
	struct sev_data_snp_shutdown_ex data;
	int ret;

	if (!psp || !psp->sev_data)
		return 0;

	sev = psp->sev_data;

	if (!sev->snp_initialized)
		return 0;

	memset(&data, 0, sizeof(data));
	data.len = sizeof(data);
	data.iommu_snp_shutdown = 1;
	if (sev->snp_feat_info_0.ecx & SNP_X86_SHUTDOWN_SUPPORTED)
		data.x86_snp_shutdown = 1;

	/*
	 * If invoked during panic handling, local interrupts are disabled
	 * and all CPUs are stopped, so wbinvd_on_all_cpus() can't be called.
	 * In that case, a wbinvd() is done on remote CPUs via the NMI
	 * callback, so only a local wbinvd() is needed here.
	 */
	if (!panic)
		wbinvd_on_all_cpus();
	else
		wbinvd();

	ret = __sev_do_cmd_locked(SEV_CMD_SNP_SHUTDOWN_EX, &data, error);
	/* SHUTDOWN may require DF_FLUSH */
	if (*error == SEV_RET_DFFLUSH_REQUIRED) {
		int dfflush_error = SEV_RET_NO_FW_CALL;

		ret = __sev_do_cmd_locked(SEV_CMD_SNP_DF_FLUSH, NULL, &dfflush_error);
		if (ret) {
			dev_err(sev->dev, "SEV-SNP DF_FLUSH failed, ret = %d, error = %#x\n",
				ret, dfflush_error);
			return ret;
		}
		/* reissue the shutdown command */
		ret = __sev_do_cmd_locked(SEV_CMD_SNP_SHUTDOWN_EX, &data,
					  error);
	}
	if (ret) {
		dev_err(sev->dev, "SEV-SNP firmware shutdown failed, rc %d, error %#x\n",
			ret, *error);
		return ret;
	}

	if (data.x86_snp_shutdown) {
		if (!panic)
			snp_shutdown();
		snp_hv_fixed_pages_state_update(sev, ALLOCATED);
	} else {
		/*
		 * SNP_SHUTDOWN_EX with IOMMU_SNP_SHUTDOWN set to 1 disables SNP
		 * enforcement by the IOMMU and also transitions all pages
		 * associated with the IOMMU to the Reclaim state.
		 * Firmware was transitioning the IOMMU pages to Hypervisor state
		 * before version 1.53. But, accounting for the number of assigned
		 * 4kB pages in a 2M page was done incorrectly by not transitioning
		 * to the Reclaim state. This resulted in RMP #PF when later accessing
		 * the 2M page containing those pages during kexec boot. Hence, the
		 * firmware now transitions these pages to Reclaim state and hypervisor
		 * needs to transition these pages to shared state. SNP Firmware
		 * version 1.53 and above are needed for kexec boot.
		 */
		ret = amd_iommu_snp_disable();
		if (ret) {
			dev_err(sev->dev, "SNP IOMMU shutdown failed\n");
			return ret;
		}
	}

	snp_leak_hv_fixed_pages();
	sev->snp_initialized = false;
	dev_dbg(sev->dev, "SEV-SNP firmware shutdown\n");

	/*
	 * __sev_snp_shutdown_locked() deadlocks when it tries to unregister
	 * itself during panic as the panic notifier is called with RCU read
	 * lock held and notifier unregistration does RCU synchronization.
	 */
	if (!panic)
		atomic_notifier_chain_unregister(&panic_notifier_list,
						 &snp_panic_notifier);

	/* Reset TMR size back to default */
	sev_es_tmr_size = SEV_TMR_SIZE;

	return ret;
}
/* …（同文件无关代码省略）… */
	data.oca_cert_address = __psp_pa(oca_blob);
	data.oca_cert_len = input.oca_cert_len;

	/* If platform is not in INIT state then transition it to INIT */
	if (sev->sev_plat_status.state != SEV_STATE_INIT) {
		ret = sev_move_to_init_state(argp, &shutdown_required);
		if (ret)
			goto e_free_oca;
	}

	ret = __sev_do_cmd_locked(SEV_CMD_PEK_CERT_IMPORT, &data, &argp->error);

e_free_oca:
	if (shutdown_required)
		__sev_firmware_shutdown(sev, false);

	kfree(oca_blob);
e_free_pek:
	kfree(pek_blob);
/* …（同文件无关代码省略）… */
		data.len = 0;
	}

	ret = __sev_do_cmd_locked(SEV_CMD_GET_ID, &data, &argp->error);

	/*
	 * Firmware will return the length of the ID value (either the minimum
/* …（同文件无关代码省略）… */
	data->address = __psp_pa(id_blob);
	data->len = user_size;

	ret = __sev_do_cmd_locked(SEV_CMD_GET_ID, data, &argp->error);
	if (!ret) {
		if (copy_to_user((void __user *)argp->data, id_blob, data->len))
			ret = -EFAULT;
/* …（同文件无关代码省略）… */
	data.cert_chain_len = input.cert_chain_len;

cmd:
	/* If platform is not in INIT state then transition it to INIT. */
	if (sev->sev_plat_status.state != SEV_STATE_INIT) {
		if (!writable) {
			ret = -EPERM;
			goto e_free_cert;
		}
		ret = sev_move_to_init_state(argp, &shutdown_required);
		if (ret)
			goto e_free_cert;
	}

	ret = __sev_do_cmd_locked(SEV_CMD_PDH_CERT_EXPORT, &data, &argp->error);

	/*
	 * Firmware will return the length of the blobs (either the minimum
	 * required length or the actual length written), return 'em to the user.
/* …（同文件无关代码省略）… */
	}

e_free_cert:
	if (shutdown_required)
		__sev_firmware_shutdown(sev, false);

	kfree(cert_blob);
e_free_pdh:
	kfree(pdh_blob);
/* …（同文件无关代码省略）… */
static int __sev_do_snp_platform_status(struct sev_user_data_snp_status *status,
					int *error)
{
	struct sev_device *sev = psp_master->sev_data;
	struct sev_data_snp_addr buf;
	struct page *status_page;
	void *data;
	int ret;

	status_page = alloc_page(GFP_KERNEL_ACCOUNT);
	if (!status_page)
		return -ENOMEM;

	data = page_address(status_page);

	/*
	 * SNP_PLATFORM_STATUS can be executed in any SNP state. But if executed
	 * when SNP has been initialized, the status page must be firmware-owned.
	 */
	if (sev->snp_initialized) {
		/*
		 * Firmware expects the status page to be in Firmware state,
		 * otherwise it will report an error INVALID_PAGE_STATE.
		 */
		if (rmp_mark_pages_firmware(__pa(data), 1, true)) {
			ret = -EFAULT;
			goto cleanup;
		}
	}

	buf.address = __psp_pa(data);
	ret = __sev_do_cmd_locked(SEV_CMD_SNP_PLATFORM_STATUS, &buf, error);

	if (sev->snp_initialized) {
		/*
		 * The status page will be in Reclaim state on success, or left
		 * in Firmware state on failure. Use snp_reclaim_pages() to
		 * transition either case back to Hypervisor-owned state.
		 */
		if (snp_reclaim_pages(__pa(data), 1, true))
			return -EFAULT;
	}

	if (ret)
		goto cleanup;

	memcpy(status, data, sizeof(*status));

cleanup:
	__free_pages(status_page, 0);
	return ret;
}
/* …（同文件无关代码省略）… */

	buf.len = sizeof(buf);

	ret = __sev_do_cmd_locked(SEV_CMD_SNP_COMMIT, &buf, &argp->error);

	return ret;
}
/* …（同文件无关代码省略）… */
{
	struct sev_device *sev = psp_master->sev_data;
	struct sev_user_data_snp_config config;

	if (!argp->data)
		return -EINVAL;
/* …（同文件无关代码省略）… */
	if (copy_from_user(&config, (void __user *)argp->data, sizeof(config)))
		return -EFAULT;

	return __sev_do_cmd_locked(SEV_CMD_SNP_CONFIG, &config, &argp->error);
}

static int sev_ioctl_do_snp_vlek_load(struct sev_issue_cmd *argp, bool writable)
{
	struct sev_device *sev = psp_master->sev_data;
	struct sev_user_data_snp_vlek_load input;
	void *blob;
	int ret;

	if (!argp->data)
		return -EINVAL;

	if (!writable)
		return -EPERM;

	if (!sev->snp_initialized)
		return -ENODEV;

	if (copy_from_user(&input, u64_to_user_ptr(argp->data), sizeof(input)))
		return -EFAULT;

	if (input.len != sizeof(input) || input.vlek_wrapped_version != 0)
		return -EINVAL;

	blob = psp_copy_user_blob(input.vlek_wrapped_address,
				  sizeof(struct sev_user_data_snp_wrapped_vlek_hashstick));
	if (IS_ERR(blob))
		return PTR_ERR(blob);

	input.vlek_wrapped_address = __psp_pa(blob);

	ret = __sev_do_cmd_locked(SEV_CMD_SNP_VLEK_LOAD, &input, &argp->error);
	kfree(blob);

	return ret;
}
/* …（同文件无关代码省略）… */
	if (input.cmd > SEV_MAX)
		return -EINVAL;

	mutex_lock(&sev_cmd_mutex);

	switch (input.cmd) {

	case SEV_FACTORY_RESET:
/* …（同文件无关代码省略）… */
	if (copy_to_user(argp, &input, sizeof(struct sev_issue_cmd)))
		ret = -EFAULT;
out:
	mutex_unlock(&sev_cmd_mutex);

	return ret;
}
/* …（同文件无关代码省略）… */
static void __sev_firmware_shutdown(struct sev_device *sev, bool panic)
{
	int error;

	__sev_platform_shutdown_locked(&error);

	if (sev_es_tmr) {
		/*
		 * The TMR area was encrypted, flush it from the cache.
		 *
		 * If invoked during panic handling, local interrupts are
		 * disabled and all CPUs are stopped, so wbinvd_on_all_cpus()
		 * can't be used. In that case, wbinvd() is done on remote CPUs
		 * via the NMI callback, and done for this CPU later during
		 * SNP shutdown, so wbinvd_on_all_cpus() can be skipped.
		 */
		if (!panic)
			wbinvd_on_all_cpus();

		__snp_free_firmware_pages(virt_to_page(sev_es_tmr),
					  get_order(sev_es_tmr_size),
					  true);
		sev_es_tmr = NULL;
	}

	if (sev_init_ex_buffer) {
		__snp_free_firmware_pages(virt_to_page(sev_init_ex_buffer),
					  get_order(NV_LENGTH),
					  true);
		sev_init_ex_buffer = NULL;
	}

	__sev_snp_shutdown_locked(&error, panic);
}
/* …（同文件无关代码省略）… */
	struct sev_user_data_snp_status status;
	int ret, error = 0;

	mutex_lock(&sev_cmd_mutex);
	ret = __sev_do_snp_platform_status(&status, &error);
	mutex_unlock(&sev_cmd_mutex);
	if (ret < 0)
		return ret;

/* …（同文件无关代码省略）… */
	struct sev_snp_tcb_version_turin *tcb;
	int ret, error = 0;

	mutex_lock(&sev_cmd_mutex);
	ret = __sev_do_snp_platform_status(&status, &error);
	mutex_unlock(&sev_cmd_mutex);
	if (ret < 0)
		return ret;
