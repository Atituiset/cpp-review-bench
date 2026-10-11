// AUTO-DRAFT from torvalds/linux PR #a5ebb76233b79db01e82e061173cb73c6d2b5c6b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>
  // <<< BUG ANCHOR
static inline void error_address_to_page_and_offset(u64 error_address,
						    struct err_info *err)
{
	err->page = (u32) (error_address >> PAGE_SHIFT);
	err->offset = ((u32) error_address) & ~PAGE_MASK;
}
/* …（同文件无关代码省略）… */
static int fixup_node_id(int node_id, struct mce *m)
{
	/* MCA_IPID[InstanceIdHi] give the AMD Node ID for the bank. */
	u8 nid = (m->ipid >> 44) & 0xF;

	if (smca_get_bank_type(m->extcpu, m->bank) != SMCA_UMC_V2)
		return node_id;

	/* Nodes below the GPU base node are CPU nodes and don't need a fixup. */
	if (nid < gpu_node_map.base_node_id)
		return node_id;

	/* Convert the hardware-provided AMD Node ID to a Linux logical one. */
	return nid - gpu_node_map.base_node_id + 1;
}
/* …（同文件无关代码省略）… */
static void __log_ecc_error(struct mem_ctl_info *mci, struct err_info *err,
			    u8 ecc_type)
{
	enum hw_event_mc_err_type err_type;
	const char *string;

	if (ecc_type == 2)
		err_type = HW_EVENT_ERR_CORRECTED;
	else if (ecc_type == 1)
		err_type = HW_EVENT_ERR_UNCORRECTED;
	else if (ecc_type == 3)
		err_type = HW_EVENT_ERR_DEFERRED;
	else {
		WARN(1, "Something is rotten in the state of Denmark.\n");
		return;
	}

	switch (err->err_code) {
	case DECODE_OK:
		string = "";
		break;
	case ERR_NODE:
		string = "Failed to map error addr to a node";
		break;
	case ERR_CSROW:
		string = "Failed to map error addr to a csrow";
		break;
	case ERR_CHANNEL:
		string = "Unknown syndrome - possible error reporting race";
		break;
	case ERR_SYND:
		string = "MCA_SYND not valid - unknown syndrome and csrow";
		break;
	case ERR_NORM_ADDR:
		string = "Cannot decode normalized address";
		break;
	default:
		string = "WTF error";
		break;
	}

	edac_mc_handle_error(err_type, mci, 1,
			     err->page, err->offset, err->syndrome,
			     err->csrow, err->channel, -1,
			     string, "");
}
/* …（同文件无关代码省略）… */
 * the instance_id. For example, instance_id=0xYXXXXX where Y is the channel
 * number.
 *
 * For DRAM ECC errors, the Chip Select number is given in bits [2:0] of
 * the MCA_SYND[ErrorInformation] field.
 */
static void umc_get_err_info(struct mce *m, struct err_info *err)
{
	err->channel = (m->ipid & GENMASK(31, 0)) >> 20;
	err->csrow = m->synd & 0x7;
}

static void decode_umc_error(int node_id, struct mce *m)
{
	u8 ecc_type = (m->status >> 45) & 0x3;
	struct mem_ctl_info *mci;
	unsigned long sys_addr;
	struct amd64_pvt *pvt;
	struct atl_err a_err;
	struct err_info err;

	node_id = fixup_node_id(node_id, m);

	mci = edac_mc_find(node_id);
	if (!mci)
		return;

	pvt = mci->pvt_info;

	memset(&err, 0, sizeof(err));

	if (m->status & MCI_STATUS_DEFERRED)
		ecc_type = 3;

	if (!(m->status & MCI_STATUS_SYNDV)) {
		err.err_code = ERR_SYND;
		goto log_error;
	}

	if (ecc_type == 2) {
		u8 length = (m->synd >> 18) & 0x3f;

		if (length)
			err.syndrome = (m->synd >> 32) & GENMASK(length - 1, 0);
		else
			err.err_code = ERR_CHANNEL;
	}

	pvt->ops->get_err_info(m, &err);

	a_err.addr = m->addr;
	a_err.ipid = m->ipid;
	a_err.cpu  = m->extcpu;

	sys_addr = amd_convert_umc_mca_addr_to_sys_addr(&a_err);
	if (IS_ERR_VALUE(sys_addr)) {
		err.err_code = ERR_NORM_ADDR;
		goto log_error;
	}

	error_address_to_page_and_offset(sys_addr, &err);

log_error:
	__log_ecc_error(mci, &err, ecc_type);
}
