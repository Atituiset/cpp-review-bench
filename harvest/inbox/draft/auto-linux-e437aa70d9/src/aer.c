// AUTO-DRAFT from torvalds/linux PR #3857c2fe5449541e24afc5efdb0f81a8a8f9a3a0
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define pr_fmt(fmt) "AER: " fmt
#define dev_fmt pr_fmt
/* …（同文件无关代码省略）… */
#define aer_printk(level, pdev, fmt, arg...) \
	dev_printk(level, &(pdev)->dev, fmt, ##arg)
/* …（同文件无关代码省略）… */
#define AER_MAX_TYPEOF_COR_ERRS		16	/* as per PCI_ERR_COR_STATUS */
#define AER_MAX_TYPEOF_UNCOR_ERRS	32	/* as per PCI_ERR_UNCOR_STATUS*/
/* …（同文件无关代码省略）… */
struct aer_info {

	/*
	 * Fields for all AER capable devices. They indicate the errors
	 * "as seen by this device". Note that this may mean that if an
	 * Endpoint is causing problems, the AER counters may increment
	 * at its link partner (e.g. Root Port) because the errors will be
	 * "seen" by the link partner and not the problematic Endpoint
	 * itself (which may report all counters as 0 as it never saw any
	 * problems).
	 */
	/* Counters for different type of correctable errors */
	u64 dev_cor_errs[AER_MAX_TYPEOF_COR_ERRS];
	/* Counters for different type of fatal uncorrectable errors */
	u64 dev_fatal_errs[AER_MAX_TYPEOF_UNCOR_ERRS];
	/* Counters for different type of nonfatal uncorrectable errors */
	u64 dev_nonfatal_errs[AER_MAX_TYPEOF_UNCOR_ERRS];
	/* Total number of ERR_COR sent by this device */
	u64 dev_total_cor_errs;
	/* Total number of ERR_FATAL sent by this device */
	u64 dev_total_fatal_errs;
	/* Total number of ERR_NONFATAL sent by this device */
	u64 dev_total_nonfatal_errs;

	/*
	 * Fields for Root Ports & Root Complex Event Collectors only; these
	 * indicate the total number of ERR_COR, ERR_FATAL, and ERR_NONFATAL
	 * messages received by the Root Port / Event Collector, INCLUDING the
	 * ones that are generated internally (by the Root Port itself)
	 */
	u64 rootport_total_cor_errs;
	u64 rootport_total_fatal_errs;
	u64 rootport_total_nonfatal_errs;

	/* Ratelimits for errors */
	struct ratelimit_state correctable_ratelimit;
	struct ratelimit_state nonfatal_ratelimit;
};
/* …（同文件无关代码省略）… */
#define AER_ANFE			(PCI_ERR_UNC_UNX_COMP |		\
					PCI_ERR_UNC_ATOMEG |		\
					PCI_ERR_UNC_DMWR_BLK)
/* …（同文件无关代码省略）… */
#define AER_POSSIBLE_ANFE		(PCI_ERR_UNC_POISON_TLP |	\
					PCI_ERR_UNC_POISON_BLK |	\
					PCI_ERR_UNC_ECRC |		\
					PCI_ERR_UNC_UNSUP |		\
					PCI_ERR_UNC_COMP_TIME |		\
					PCI_ERR_UNC_COMP_ABORT |	\
					PCI_ERR_UNC_ACSV |		\
					PCI_ERR_UNC_TLPPRE |		\
					PCI_ERR_UNC_PCRC_CHECK)
/* …（同文件无关代码省略）… */
#define AER_LOG_TLP_MASKS		(PCI_ERR_UNC_POISON_TLP|	\
					PCI_ERR_UNC_POISON_BLK |	\
					PCI_ERR_UNC_ECRC|		\
					PCI_ERR_UNC_UNSUP|		\
					PCI_ERR_UNC_COMP_ABORT|		\
					PCI_ERR_UNC_UNX_COMP|		\
					PCI_ERR_UNC_ACSV |		\
					PCI_ERR_UNC_MCBTLP |		\
					PCI_ERR_UNC_ATOMEG |		\
					PCI_ERR_UNC_DMWR_BLK |		\
					PCI_ERR_UNC_XLAT_BLK |		\
					PCI_ERR_UNC_TLPPRE |		\
					PCI_ERR_UNC_MALF_TLP |		\
					PCI_ERR_UNC_IDE_CHECK |		\
					PCI_ERR_UNC_MISR_IDE |		\
					PCI_ERR_UNC_PCRC_CHECK)
/* …（同文件无关代码省略）… */
#define ROOT_PORT_INTR_ON_MESG_MASK	(PCI_ERR_ROOT_CMD_COR_EN|	\
					PCI_ERR_ROOT_CMD_NONFATAL_EN|	\
					PCI_ERR_ROOT_CMD_FATAL_EN)
/* …（同文件无关代码省略）… */
#define AER_AGENT_RECEIVER		0
#define AER_AGENT_REQUESTER		1
#define AER_AGENT_COMPLETER		2
#define AER_AGENT_TRANSMITTER		3
#define AER_AGENT_COMPONENT		4
#define AER_AGENT_UNDEF			5
/* …（同文件无关代码省略）… */
#define AER_AGENT_REQUESTER_MASK(t)	((t == AER_CORRECTABLE) ?	\
	0 : PCI_ERR_UNC_COMP_TIME)
#define AER_AGENT_COMPLETER_MASK(t)	((t == AER_CORRECTABLE) ?	\
	0 : PCI_ERR_UNC_COMP_ABORT)
#define AER_AGENT_TRANSMITTER_MASK(t)	((t == AER_CORRECTABLE) ?	\
	(PCI_ERR_COR_REP_ROLL|PCI_ERR_COR_REP_TIMER) :			\
	(PCI_ERR_UNC_POISON_BLK|PCI_ERR_UNC_ATOMEG|			\
	 PCI_ERR_UNC_DMWR_BLK|PCI_ERR_UNC_XLAT_BLK|			\
	 PCI_ERR_UNC_TLPPRE))
#define AER_AGENT_COMPONENT_MASK(t)	((t == AER_CORRECTABLE) ?	\
	(PCI_ERR_COR_INTERNAL|PCI_ERR_COR_LOG_OVER) :			\
	(PCI_ERR_UNC_INTN|PCI_ERR_UNC_SURPDN))
#define AER_AGENT_UNDEF_MASK(t)		((t == AER_CORRECTABLE) ?	\
	PCI_ERR_COR_ADV_NFAT : 0)
/* …（同文件无关代码省略）… */
#define AER_GET_AGENT(t, e)						\
	((e & AER_AGENT_COMPLETER_MASK(t)) ? AER_AGENT_COMPLETER :	\
	(e & AER_AGENT_REQUESTER_MASK(t)) ? AER_AGENT_REQUESTER :	\
	(e & AER_AGENT_TRANSMITTER_MASK(t)) ? AER_AGENT_TRANSMITTER :	\
	(e & AER_AGENT_COMPONENT_MASK(t)) ? AER_AGENT_COMPONENT :	\
	(e & AER_AGENT_UNDEF_MASK(t)) ? AER_AGENT_UNDEF :		\
	AER_AGENT_RECEIVER)
/* …（同文件无关代码省略）… */
#define AER_PHYSICAL_LAYER_ERROR	0
#define AER_DATA_LINK_LAYER_ERROR	1
#define AER_TRANSACTION_LAYER_ERROR	2
#define AER_GENERAL_ERROR		3
#define AER_UNDEF_ERROR			4
/* …（同文件无关代码省略）… */
#define AER_PHYSICAL_LAYER_ERROR_MASK(t) ((t == AER_CORRECTABLE) ?	\
	PCI_ERR_COR_RCVR : 0)
#define AER_DATA_LINK_LAYER_ERROR_MASK(t) ((t == AER_CORRECTABLE) ?	\
	(PCI_ERR_COR_BAD_TLP|						\
	PCI_ERR_COR_BAD_DLLP|						\
	PCI_ERR_COR_REP_ROLL|						\
	PCI_ERR_COR_REP_TIMER) : (PCI_ERR_UNC_DLP|PCI_ERR_UNC_SURPDN))
#define AER_GENERAL_ERROR_MASK(t)  ((t == AER_CORRECTABLE) ?		\
	(PCI_ERR_COR_INTERNAL|PCI_ERR_COR_LOG_OVER) : PCI_ERR_UNC_INTN)
#define AER_UNDEF_ERROR_MASK(t)  ((t == AER_CORRECTABLE) ?		\
	PCI_ERR_COR_ADV_NFAT : 0)
/* …（同文件无关代码省略）… */
#define AER_GET_LAYER_ERROR(t, e)					\
	((e & AER_PHYSICAL_LAYER_ERROR_MASK(t)) ? AER_PHYSICAL_LAYER_ERROR : \
	(e & AER_DATA_LINK_LAYER_ERROR_MASK(t)) ? AER_DATA_LINK_LAYER_ERROR : \
	(e & AER_GENERAL_ERROR_MASK(t)) ? AER_GENERAL_ERROR :		\
	(e & AER_UNDEF_ERROR_MASK(t)) ? AER_UNDEF_ERROR :		\
	AER_TRANSACTION_LAYER_ERROR)
/* …（同文件无关代码省略）… */
static void pci_dev_aer_stats_incr(struct pci_dev *pdev,
				   struct aer_err_info *info)
{
	unsigned long status = info->status & ~info->mask;
	int i, max = -1;
	u64 *counter = NULL;
	struct aer_info *aer_info = pdev->aer_info;

	if (!aer_info)
		return;

	switch (info->severity) {
	case AER_CORRECTABLE:
		aer_info->dev_total_cor_errs++;
		counter = &aer_info->dev_cor_errs[0];
		max = AER_MAX_TYPEOF_COR_ERRS;
		break;
	case AER_NONFATAL:
		aer_info->dev_total_nonfatal_errs++;
		hwerr_log_error_type(HWERR_RECOV_PCI);
		counter = &aer_info->dev_nonfatal_errs[0];
		max = AER_MAX_TYPEOF_UNCOR_ERRS;
		break;
	case AER_FATAL:
		aer_info->dev_total_fatal_errs++;
		counter = &aer_info->dev_fatal_errs[0];
		max = AER_MAX_TYPEOF_UNCOR_ERRS;
		break;
	}

	for_each_set_bit(i, &status, max)
		counter[i]++;
}
/* …（同文件无关代码省略）… */
static int aer_ratelimit(struct pci_dev *dev, unsigned int severity)
{
	if (!dev->aer_info)
		return 1;

	switch (severity) {
	case AER_NONFATAL:
		return __ratelimit(&dev->aer_info->nonfatal_ratelimit);
	case AER_CORRECTABLE:
		return __ratelimit(&dev->aer_info->correctable_ratelimit);
	default:
		return 1;	/* Don't ratelimit fatal errors */
	}
}
/* …（同文件无关代码省略）… */
static u32 aer_compute_anfe_status(u16 devsta, u32 uncor_status,
				   u32 uncor_mask, u32 uncor_severity)
{
	u32 anfe_status;

	/*
	 * Uncorrectable Errors must be unmasked and have Non-Fatal severity
	 * to qualify as Advisory Non-Fatal Errors (PCIe r7.0 sec 6.2.4.3).
	 */
	uncor_status &= ~uncor_mask & ~uncor_severity;

	/* Some Non-Fatal Errors are always Advisory (PCIe r7.0 sec 6.2.7). */
	anfe_status = uncor_status & AER_ANFE;

	/*
	 * Others may be Advisory at the discretion of the detecting agent.
	 * That's impossible to discern if the agent signaled ERR_NONFATAL
	 * in addition to ERR_COR.  Assume none are Advisory in that case
	 * to ensure that the Uncorrectable Error code path is taken.
	 */
	if (!(devsta & PCI_EXP_DEVSTA_NFED))
		anfe_status |= uncor_status & AER_POSSIBLE_ANFE;

	return anfe_status;
}
/* …（同文件无关代码省略）… */
static bool tlp_header_logged(u32 status, u32 capctl)
{
	/* Errors for which a header is always logged (PCIe r7.0 sec 6.2.7) */
	if (status & AER_LOG_TLP_MASKS)
		return true;

	/* Completion Timeout header is only logged on capable devices */
	if (status & PCI_ERR_UNC_COMP_TIME &&
	    capctl & PCI_ERR_CAP_COMP_TIME_LOG)
		return true;

	return false;
}
/* …（同文件无关代码省略）… */
static void __aer_print_error(struct pci_dev *dev, struct aer_err_info *info)
{
	const char **strings;
	unsigned long status = info->status & ~info->mask;
	const char *errmsg, *agent, *layer;
	const char *level = info->level;
	int i;

	if (info->severity == AER_CORRECTABLE)
		strings = aer_correctable_error_string;
	else
		strings = aer_uncorrectable_error_string;

	for_each_set_bit(i, &status, 32) {
		errmsg = strings[i];
		agent = aer_agent_string[AER_GET_AGENT(info->severity, BIT(i))];
		layer = aer_error_layer[AER_GET_LAYER_ERROR(info->severity,
								       BIT(i))];
		if (!errmsg) {
			errmsg = "Unknown Error Bit";
			agent = "";
			layer = "";
		}

		aer_printk(level, dev, "  [%2d] %-17s | %-11s | %-17s%s\n",
			   i, errmsg, agent, layer,
			   info->severity != AER_CORRECTABLE &&
				info->first_error == i ? " (First)" : "");
	}
}
/* …（同文件无关代码省略）… */
static void __pci_print_aer(struct pci_dev *dev, int aer_severity,
			    struct aer_capability_regs *aer,
			    bool ratelimit_print, const char *level)
{
	const char *bus_type, *sev;
	int tlp_header_valid = 0;
	u32 status, mask;
	struct aer_err_info info = {
		.level = level,
		.severity = aer_severity,
		.first_error = PCI_ERR_CAP_FEP(aer->cap_control),
	};

	if (aer_severity == AER_CORRECTABLE) {
		status = aer->cor_status;
		mask = aer->cor_mask;
		sev = "cor";
	} else {
		status = aer->uncor_status;
		mask = aer->uncor_mask;
		sev = "uncor";
		tlp_header_valid = tlp_header_logged(status & ~mask,
						     aer->cap_control);
	}

	info.status = status;
	info.mask = mask;
	info.is_cxl = pcie_is_cxl(dev);

	bus_type = aer_err_bus(&info);

	pci_dev_aer_stats_incr(dev, &info);
	trace_aer_event(pci_name(dev), (status & ~mask), aer_severity,
			tlp_header_valid, &aer->header_log, bus_type);

	/*
	 * For Advisory Non-Fatal Errors, record statistics and tracing
	 * even if ratelimited
	 */
	if (!ratelimit_print)
		goto anfe;

	aer_printk(info.level, dev,
		   "aer_%s_status: 0x%08x, aer_%s_mask: 0x%08x\n",
		   sev, status, sev, mask);
	__aer_print_error(dev, &info);

	if (aer_severity != AER_CORRECTABLE)
		aer_printk(info.level, dev, "aer_uncor_severity: 0x%08x\n",
			   aer->uncor_severity);

	if (tlp_header_valid)
		pcie_print_tlp_log(dev, &aer->header_log, info.level,
				   dev_fmt("  "));

anfe:
	/* Recursive invocation for Advisory Non-Fatal Errors */
	if (aer_severity == AER_CORRECTABLE &&
	    info.status & ~info.mask & PCI_ERR_COR_ADV_NFAT) {
		u32 anfe_status = aer_compute_anfe_status(PCI_EXP_DEVSTA_CED,
							  aer->uncor_status,
							  aer->uncor_mask,
							  aer->uncor_severity);
		if (anfe_status) {
			aer->uncor_status = anfe_status;
			aer->uncor_mask = 0;
			__pci_print_aer(dev, AER_NONFATAL, aer,
					ratelimit_print, level);
		}
	}
}
/* …（同文件无关代码省略）… */
void pci_print_aer(struct pci_dev *dev, int aer_severity,
		   struct aer_capability_regs *aer)
{
	/*
	 * Precalculate ratelimit counter and log level so that Advisory
	 * Non-Fatal Errors are treated like the accompanying Correctable Error
	 */
	bool ratelimit_print = aer_ratelimit(dev, aer_severity);
	const char *level = aer_severity == AER_CORRECTABLE ? KERN_WARNING
							    : KERN_ERR;

	__pci_print_aer(dev, aer_severity, aer, ratelimit_print, level);
}
/* …（同文件无关代码省略）… */
struct aer_recover_entry {
	u8	bus;
	u8	devfn;
	u16	domain;
	int	severity;
	struct aer_capability_regs *regs;
};
/* …（同文件无关代码省略）… */

static void aer_recover_work_func(struct work_struct *work)
{
	struct aer_recover_entry entry;
	struct pci_dev *pdev;

	while (kfifo_get(&aer_recover_ring, &entry)) {
		pdev = pci_get_domain_bus_and_slot(entry.domain, entry.bus,
/* …（同文件无关代码省略）… */
		}
		pci_print_aer(pdev, entry.severity, entry.regs);

		/*
		 * Memory for aer_capability_regs(entry.regs) is being
		 * allocated from the ghes_estatus_pool to protect it from
/* …（同文件无关代码省略）… */
		ghes_estatus_pool_region_free((unsigned long)entry.regs,
					    sizeof(struct aer_capability_regs));

		if (entry.severity == AER_NONFATAL)
			pcie_do_recovery(pdev, pci_channel_io_normal,
					 aer_root_reset);
		else if (entry.severity == AER_FATAL)
			pcie_do_recovery(pdev, pci_channel_io_frozen,
					 aer_root_reset);
		pci_dev_put(pdev);
	}
}
/* …（同文件无关代码省略）… */
static void aer_enable_irq(struct pci_dev *pdev)
{
	int aer = pdev->aer_cap;
	u32 reg32;

	/* Enable Root Port's interrupt in response to error messages */
	pci_read_config_dword(pdev, aer + PCI_ERR_ROOT_COMMAND, &reg32);
	reg32 |= ROOT_PORT_INTR_ON_MESG_MASK;
	pci_write_config_dword(pdev, aer + PCI_ERR_ROOT_COMMAND, reg32);
}
/* …（同文件无关代码省略）… */
static void aer_disable_irq(struct pci_dev *pdev)
{
	int aer = pdev->aer_cap;
	u32 reg32;

	/* Disable Root Port's interrupt in response to error messages */
	pci_read_config_dword(pdev, aer + PCI_ERR_ROOT_COMMAND, &reg32);
	reg32 &= ~ROOT_PORT_INTR_ON_MESG_MASK;
	pci_write_config_dword(pdev, aer + PCI_ERR_ROOT_COMMAND, reg32);
}
/* …（同文件无关代码省略）… */
static pci_ers_result_t aer_root_reset(struct pci_dev *dev)
{
	int type = pci_pcie_type(dev);
	struct pci_dev *root;
	int aer;
	struct pci_host_bridge *host = pci_find_host_bridge(dev->bus);
	u32 reg32;
	int rc;

	/*
	 * Only Root Ports and RCECs have AER Root Command and Root Status
	 * registers.  If "dev" is an RCiEP, the relevant registers are in
	 * the RCEC.
	 */
	if (type == PCI_EXP_TYPE_RC_END)
		root = dev->rcec;
	else
		root = pcie_find_root_port(dev);

	/*
	 * If the platform retained control of AER, an RCiEP may not have
	 * an RCEC visible to us, so dev->rcec ("root") may be NULL.  In
	 * that case, firmware is responsible for these registers.
	 */
	aer = root ? root->aer_cap : 0;

	if ((host->native_aer || pcie_ports_native) && aer)
		aer_disable_irq(root);

	if (type == PCI_EXP_TYPE_RC_EC || type == PCI_EXP_TYPE_RC_END) {
		rc = pcie_reset_flr(dev, PCI_RESET_DO_RESET);
		if (!rc)
			pci_info(dev, "has been reset\n");
		else
			pci_info(dev, "not reset (no FLR support: %d)\n", rc);
	} else {
		rc = pci_bus_error_reset(dev);
		pci_info(dev, "%s Port link has been reset (%d)\n",
			pci_is_root_bus(dev->bus) ? "Root" : "Downstream", rc);
	}

	if ((host->native_aer || pcie_ports_native) && aer) {
		/* Clear Root Error Status */
		pci_read_config_dword(root, aer + PCI_ERR_ROOT_STATUS, &reg32);
		pci_write_config_dword(root, aer + PCI_ERR_ROOT_STATUS, reg32);

		aer_enable_irq(root);
	}

	return rc ? PCI_ERS_RESULT_DISCONNECT : PCI_ERS_RESULT_RECOVERED;
}
