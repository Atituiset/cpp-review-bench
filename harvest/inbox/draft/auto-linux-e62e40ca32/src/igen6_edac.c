// AUTO-DRAFT from torvalds/linux PR #68c90773f40f91f48a557e9fa65e2a72cebc58df
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define EDAC_MOD_STR	"igen6_edac"
/* …（同文件无关代码省略）… */
#define igen6_printk(level, fmt, arg...)		\
	edac_printk(level, "igen6", fmt, ##arg)
/* …（同文件无关代码省略）… */

#define GET_BITFIELD(v, lo, hi) (((v) & GENMASK_ULL(hi, lo)) >> (lo))

#define NUM_IMC				2 /* Max memory controllers */
#define NUM_CHANNELS			2 /* Max channels */
#define NUM_DIMMS			2 /* Max DIMMs per channel */

#define _4GB				BIT_ULL(32)
/* …（同文件无关代码省略）… */
#define CAPID_E_OFFSET			0xf0
#define CAPID_E_IBECC			BIT(12)
#define CAPID_E_IBECC_BIT18		BIT(18)
/* …（同文件无关代码省略）… */
#define IBECC_BASE			(res_cfg->ibecc_base)
#define IBECC_ACTIVATE_OFFSET		IBECC_BASE
#define IBECC_ACTIVATE_EN		BIT(0)
/* …（同文件无关代码省略）… */
#define ECC_ERROR_LOG_OFFSET		(IBECC_BASE + res_cfg->ibecc_error_log_offset)
#define ECC_ERROR_LOG_CE		BIT_ULL(62)
#define ECC_ERROR_LOG_UE		BIT_ULL(63)
/* …（同文件无关代码省略）… */
#define MCHBAR_SIZE			0x10000
/* …（同文件无关代码省略）… */
#define IMC_BASE			(res_cfg->imc_base)
#define MAD_INTER_CHANNEL_OFFSET	IMC_BASE
#define MAD_INTER_CHANNEL_DDR_TYPE(v)	GET_BITFIELD(v, 0, 2)
/* …（同文件无关代码省略）… */
#define MAD_INTER_CHANNEL_CH_L_MAP(v)	GET_BITFIELD(v, 4, 4)
#define MAD_INTER_CHANNEL_CH_S_SIZE(v)	((u64)GET_BITFIELD(v, 12, 19) << 29)
/* …（同文件无关代码省略）… */
#define MAD_INTRA_CH0_OFFSET		(IMC_BASE + 4)
#define MAD_INTRA_CH_DIMM_L_MAP(v)	GET_BITFIELD(v, 0, 0)
/* …（同文件无关代码省略）… */
#define MAD_DIMM_CH0_OFFSET		(IMC_BASE + 0xc)
#define MAD_DIMM_CH_DIMM_L_SIZE(v)	((u64)GET_BITFIELD(v, 0, 6) << 29)
#define MAD_DIMM_CH_DLW(v)		GET_BITFIELD(v, 7, 8)
#define MAD_DIMM_CH_DIMM_S_SIZE(v)	((u64)GET_BITFIELD(v, 16, 22) << 29)
#define MAD_DIMM_CH_DSW(v)		GET_BITFIELD(v, 24, 25)
/* …（同文件无关代码省略）… */
#define MAD_MC_HASH_OFFSET		(IMC_BASE + 0x1b8)
#define MAC_MC_HASH_LSB(v)		GET_BITFIELD(v, 1, 3)
/* …（同文件无关代码省略）… */
#define CHANNEL_HASH_OFFSET		(IMC_BASE + 0x24)
/* …（同文件无关代码省略）… */
#define CHANNEL_EHASH_OFFSET		(IMC_BASE + 0x28)
#define CHANNEL_HASH_MASK(v)		(GET_BITFIELD(v, 6, 19) << 6)
#define CHANNEL_HASH_LSB_MASK_BIT(v)	GET_BITFIELD(v, 24, 26)
#define CHANNEL_HASH_MODE(v)		GET_BITFIELD(v, 28, 28)
/* …（同文件无关代码省略）… */
#define MEM_SLICE_HASH_MASK(v)		(GET_BITFIELD(v, 6, 19) << 6)
#define MEM_SLICE_HASH_LSB_MASK_BIT(v)	GET_BITFIELD(v, 24, 26)

struct igen6_imc {
	int mc;
	struct mem_ctl_info *mci;
/* …（同文件无关代码省略）… */
static struct res_config {
	bool machine_check;
	/* The number of present memory controllers. */
	int num_imc;
	/* Host MMIO configuration */
	u64 reg_mchbar_mask;
	/* Top of memory */
	u64 reg_tom_mask;
	/* Top of upper usable DRAM */
	u64 reg_touud_mask;
	/* IBECC error log */
	u64 reg_eccerrlog_addr_mask;
	/* MEMSS_PMA_CR registers. */
	u32 reg_mem_config_offset;
	u32 reg_mem_config_ddr_type_mask;
	u32 reg_mem_config_ibecc_en_mask;
	u32 reg_capabilities_misc_offset;
	u32 reg_capabilities_misc_ibecc_dis;
	/* Memory controller registers. */
	u32 reg_mad_inter_size_mask[NUM_CHANNELS];
	u64 reg_mad_inter_size_granularity;
	u32 reg_mad_intra_rank_mask[NUM_DIMMS];
	u32 reg_mad_intra_width_mask[NUM_DIMMS];
	u32 reg_mad_intra_density_mask[NUM_DIMMS];
	u32 imc_base;
	u32 cmf_base;
	u32 cmf_size;
	u32 ms_hash_offset;
	u32 ibecc_base;
	u32 ibecc_error_log_offset;
	/* Get memory type. */
	enum mem_type (*get_mem_type)(struct igen6_imc *imc);
	/* Get DRAM chip type. */
	enum dev_type (*get_dev_type)(struct igen6_imc *imc, int chan, int dimm_l);
	/* Set imc->ch_{s_size,l_map}. */
	void (*set_chan_params)(struct igen6_imc *imc);
	/* Set imc->dimm_{l_size,s_size,l_map}[chan]. */
	void (*set_dimm_params)(struct igen6_imc *imc, int chan);
	bool (*ibecc_available)(struct pci_dev *pdev);
	/* Extract error address logged in IBECC */
	u64 (*err_addr)(u64 ecclog);
	/* Convert error address logged in IBECC to system physical address */
	u64 (*err_addr_to_sys_addr)(u64 eaddr, int mc);
	/* Convert error address logged in IBECC to integrated memory controller address */
	u64 (*err_addr_to_imc_addr)(u64 eaddr, int mc);
} *res_cfg;

static struct igen6_pvt {
	struct igen6_imc imc[NUM_IMC];
	void __iomem *memss_pma_cr;
	u64 ms_hash;
	u64 ms_s_size;
	int ms_l_map;
} *igen6_pvt;

/* The top of low usable DRAM */
/* …（同文件无关代码省略）… */
struct decoded_addr {
	int mc;
	u64 imc_addr;
	u64 sys_addr;
	int channel_idx;
	u64 channel_addr;
	int sub_channel_idx;
	u64 sub_channel_addr;
};
/* …（同文件无关代码省略）… */
struct ecclog_node {
	struct llist_node llnode;
	int mc;
	u64 ecclog;
};
/* …（同文件无关代码省略）… */
static struct irq_work ecclog_irq_work;
static struct work_struct ecclog_work;

/* Compute die IDs for Elkhart Lake with IBECC */
#define DID_EHL_SKU5	0x4514
#define DID_EHL_SKU6	0x4528
#define DID_EHL_SKU7	0x452a
/* …（同文件无关代码省略）… */
#define DID_EHL_SKU14	0x4534
#define DID_EHL_SKU15	0x4536

/* Compute die IDs for ICL-NNPI with IBECC */
#define DID_ICL_SKU8	0x4581
#define DID_ICL_SKU10	0x4585
#define DID_ICL_SKU11	0x4589
#define DID_ICL_SKU12	0x458d

/* Compute die IDs for Tiger Lake with IBECC */
#define DID_TGL_SKU	0x9a14

/* Compute die IDs for Alder Lake with IBECC */
#define DID_ADL_SKU1	0x4601
#define DID_ADL_SKU2	0x4602
#define DID_ADL_SKU3	0x4621
#define DID_ADL_SKU4	0x4641

/* Compute die IDs for Alder Lake-N with IBECC */
#define DID_ADL_N_SKU1	0x4614
#define DID_ADL_N_SKU2	0x4617
#define DID_ADL_N_SKU3	0x461b
#define DID_ADL_N_SKU4	0x461c
#define DID_ADL_N_SKU5	0x4673
#define DID_ADL_N_SKU6	0x4674
#define DID_ADL_N_SKU7	0x4675
#define DID_ADL_N_SKU8	0x4677
#define DID_ADL_N_SKU9	0x4678
#define DID_ADL_N_SKU10	0x4679
#define DID_ADL_N_SKU11	0x467c
#define DID_ADL_N_SKU12	0x4632

/* Compute die IDs for Arizona Beach with IBECC */
#define DID_AZB_SKU1	0x4676

/* Compute did IDs for Amston Lake with IBECC */
#define DID_ASL_SKU1	0x464a
#define DID_ASL_SKU2	0x4646
#define DID_ASL_SKU3	0x4652

/* Compute die IDs for Raptor Lake-P with IBECC */
#define DID_RPL_P_SKU1	0xa706
#define DID_RPL_P_SKU2	0xa707
#define DID_RPL_P_SKU3	0xa708
#define DID_RPL_P_SKU4	0xa716
#define DID_RPL_P_SKU5	0xa718

/* Compute die IDs for Meteor Lake-PS with IBECC */
#define DID_MTL_PS_SKU1	0x7d21
#define DID_MTL_PS_SKU2	0x7d22
#define DID_MTL_PS_SKU3	0x7d23
#define DID_MTL_PS_SKU4	0x7d24

/* Compute die IDs for Meteor Lake-P with IBECC */
#define DID_MTL_P_SKU1	0x7d01
#define DID_MTL_P_SKU2	0x7d02
#define DID_MTL_P_SKU3	0x7d14

/* Compute die IDs for Arrow Lake-UH with IBECC */
#define DID_ARL_UH_SKU1	0x7d06
#define DID_ARL_UH_SKU2	0x7d20
#define DID_ARL_UH_SKU3	0x7d30

/* Compute die IDs for Panther Lake-H with IBECC */
#define DID_PTL_H_SKU1	0xb000
#define DID_PTL_H_SKU2	0xb001
#define DID_PTL_H_SKU3	0xb002
/* …（同文件无关代码省略）… */
#define DID_PTL_H_SKU12	0xb029
#define DID_PTL_H_SKU13	0xb02a
#define DID_PTL_H_SKU14	0xb00a

/* Compute die IDs for Wildcat Lake with IBECC */
#define DID_WCL_SKU1	0xfd00

/* Compute die IDs for Nova Lake-H/HX with IBECC */
#define DID_NVL_H_SKU1	0xd701
#define DID_NVL_H_SKU2	0xd702
#define DID_NVL_H_SKU3	0xd704
#define DID_NVL_H_SKU4	0xd705

static int get_mchbar(struct pci_dev *pdev, u64 *mchbar)
{
	union  {
/* …（同文件无关代码省略）… */
	return 0;
}

static bool ehl_ibecc_available(struct pci_dev *pdev)
{
	u32 v;
/* …（同文件无关代码省略）… */
static bool tgl_ibecc_available(struct pci_dev *pdev)
{
	u32 v;

	if (pci_read_config_dword(pdev, CAPID_E_OFFSET, &v))
		return false;

	return !(CAPID_E_IBECC & v);
}
/* …（同文件无关代码省略）… */
static bool mtl_p_ibecc_available(struct pci_dev *pdev)
{
	u32 v;

	if (pci_read_config_dword(pdev, CAPID_E_OFFSET, &v))
		return false;

	return !(CAPID_E_IBECC_BIT18 & v);
}
/* …（同文件无关代码省略）… */
static u64 mem_addr_to_sys_addr(u64 maddr)
{
	if (maddr < igen6_tolud)
		return maddr;

	if (igen6_tom <= _4GB)
		return maddr - igen6_tolud + _4GB;

	if (maddr < _4GB)
		return maddr - igen6_tolud + igen6_tom;

	return maddr;
}

static u64 mem_slice_hash(u64 addr, u64 mask, u64 hash_init, int intlv_bit)
{
	u64 hash_addr = addr & mask, hash = hash_init;
	u64 intlv = (addr >> intlv_bit) & 1;
	int i;

	for (i = 6; i < 20; i++)
		hash ^= (hash_addr >> i) & 1;

	return hash ^ intlv;
}

static u64 tgl_err_addr_to_mem_addr(u64 eaddr, int mc)
{
	u64 maddr, hash, mask, ms_s_size;
	int intlv_bit;
	u32 ms_hash;

/* …（同文件无关代码省略）… */
	mask = MEM_SLICE_HASH_MASK(ms_hash);
	intlv_bit = MEM_SLICE_HASH_LSB_MASK_BIT(ms_hash) + 6;

	maddr = GET_BITFIELD(eaddr, intlv_bit, 63) << (intlv_bit + 1) |
		GET_BITFIELD(eaddr, 0, intlv_bit - 1);

	hash = mem_slice_hash(maddr, mask, mc, intlv_bit);

	return maddr | (hash << intlv_bit);
}

static u64 tgl_err_addr_to_sys_addr(u64 eaddr, int mc)
/* …（同文件无关代码省略）… */
static u64 tgl_err_addr_to_imc_addr(u64 eaddr, int mc)
{
	return eaddr;
}
/* …（同文件无关代码省略）… */
static u64 adl_err_addr_to_sys_addr(u64 eaddr, int mc)
{
	return mem_addr_to_sys_addr(eaddr);
}

static u64 adl_err_addr_to_imc_addr(u64 eaddr, int mc)
{
	u64 imc_addr, ms_s_size = igen6_pvt->ms_s_size;
	struct igen6_imc *imc = &igen6_pvt->imc[mc];
	int intlv_bit;
	u32 mc_hash;

	if (eaddr >= 2 * ms_s_size)
		return eaddr - ms_s_size;

	mc_hash = readl(imc->window + MAD_MC_HASH_OFFSET);

	intlv_bit = MAC_MC_HASH_LSB(mc_hash) + 6;

	imc_addr = GET_BITFIELD(eaddr, intlv_bit + 1, 63) << intlv_bit |
		   GET_BITFIELD(eaddr, 0, intlv_bit - 1);

	return imc_addr;
}

static u64 rpl_p_err_addr(u64 ecclog)
{
	return field_get(res_cfg->reg_eccerrlog_addr_mask, ecclog);
}

static enum mem_type ptl_h_get_mem_type(struct igen6_imc *imc)
{
	u32 mtype, val;

	val = readl(igen6_pvt->memss_pma_cr + res_cfg->reg_mem_config_offset);
	mtype = field_get(res_cfg->reg_mem_config_ddr_type_mask, val);

	edac_dbg(2, "mtype %u (reg 0x%x)\n", mtype, val);

	switch (mtype) {
	case 1:
		return MEM_DDR5;
	case 2:
		return MEM_LPDDR5;
	case 3:
		return MEM_LPDDR4;
	default:
		return MEM_UNKNOWN;
	}
}
/* …（同文件无关代码省略）… */
	.err_addr_to_imc_addr	= tgl_err_addr_to_imc_addr,
};

static struct res_config adl_cfg = {
	.machine_check		= true,
	.num_imc		= 2,
/* …（同文件无关代码省略）… */
	.err_addr_to_imc_addr	= adl_err_addr_to_imc_addr,
};

static struct res_config adl_n_cfg = {
	.machine_check		= true,
	.num_imc		= 1,
	.reg_mchbar_mask	= GENMASK_ULL(41, 17),
	.reg_tom_mask		= GENMASK_ULL(41, 20),
	.reg_touud_mask		= GENMASK_ULL(41, 20),
	.reg_eccerrlog_addr_mask = GENMASK_ULL(45, 5),
	.imc_base		= 0xd800,
	.ibecc_base		= 0xd400,
	.ibecc_error_log_offset	= 0x68,
	.ibecc_available	= tgl_ibecc_available,
	.err_addr_to_sys_addr	= adl_err_addr_to_sys_addr,
	.err_addr_to_imc_addr	= adl_err_addr_to_imc_addr,
};

static struct res_config rpl_p_cfg = {
	.machine_check		= true,
	.num_imc		= 2,
	.reg_mchbar_mask	= GENMASK_ULL(41, 17),
	.reg_tom_mask		= GENMASK_ULL(41, 20),
	.reg_touud_mask		= GENMASK_ULL(41, 20),
	.reg_eccerrlog_addr_mask = GENMASK_ULL(45, 5),
	.imc_base		= 0xd800,
	.ibecc_base		= 0xd400,
	.ibecc_error_log_offset	= 0x68,
	.ibecc_available	= tgl_ibecc_available,
	.err_addr		= rpl_p_err_addr,
	.err_addr_to_sys_addr	= adl_err_addr_to_sys_addr,
	.err_addr_to_imc_addr	= adl_err_addr_to_imc_addr,
};

static struct res_config mtl_ps_cfg = {
	.machine_check				= true,
	.num_imc				= 2,
/* …（同文件无关代码省略）… */
	.err_addr_to_imc_addr			= adl_err_addr_to_imc_addr,
};

static struct res_config mtl_p_cfg = {
	.machine_check		= true,
	.num_imc		= 2,
/* …（同文件无关代码省略）… */
	.err_addr_to_imc_addr	= adl_err_addr_to_imc_addr,
};

static struct res_config ptl_h_cfg = {
	.machine_check			= true,
	.num_imc			= 2,
/* …（同文件无关代码省略）… */
	.err_addr_to_imc_addr		= adl_err_addr_to_imc_addr,
};

static struct res_config wcl_cfg = {
	.machine_check		= true,
	.num_imc		= 1,
	.reg_mchbar_mask	= GENMASK_ULL(41, 17),
	.reg_tom_mask		= GENMASK_ULL(41, 20),
	.reg_touud_mask		= GENMASK_ULL(41, 20),
	.reg_eccerrlog_addr_mask = GENMASK_ULL(38, 5),
	.imc_base		= 0xd800,
	.ibecc_base		= 0xd400,
	.ibecc_error_log_offset	= 0x170,
	.ibecc_available	= mtl_p_ibecc_available,
	.err_addr_to_sys_addr	= adl_err_addr_to_sys_addr,
	.err_addr_to_imc_addr	= adl_err_addr_to_imc_addr,
};

static struct res_config nvl_h_cfg = {
	.machine_check			= true,
	.num_imc			= 2,
/* …（同文件无关代码省略）… */
	{ PCI_VDEVICE(INTEL, DID_ADL_SKU2), .driver_data = (kernel_ulong_t)&adl_cfg },
	{ PCI_VDEVICE(INTEL, DID_ADL_SKU3), .driver_data = (kernel_ulong_t)&adl_cfg },
	{ PCI_VDEVICE(INTEL, DID_ADL_SKU4), .driver_data = (kernel_ulong_t)&adl_cfg },
	{ PCI_VDEVICE(INTEL, DID_ADL_N_SKU1), .driver_data = (kernel_ulong_t)&adl_n_cfg },
	{ PCI_VDEVICE(INTEL, DID_ADL_N_SKU2), .driver_data = (kernel_ulong_t)&adl_n_cfg },
	{ PCI_VDEVICE(INTEL, DID_ADL_N_SKU3), .driver_data = (kernel_ulong_t)&adl_n_cfg },
	{ PCI_VDEVICE(INTEL, DID_ADL_N_SKU4), .driver_data = (kernel_ulong_t)&adl_n_cfg },
	{ PCI_VDEVICE(INTEL, DID_ADL_N_SKU5), .driver_data = (kernel_ulong_t)&adl_n_cfg },
	{ PCI_VDEVICE(INTEL, DID_ADL_N_SKU6), .driver_data = (kernel_ulong_t)&adl_n_cfg },
	{ PCI_VDEVICE(INTEL, DID_ADL_N_SKU7), .driver_data = (kernel_ulong_t)&adl_n_cfg },
	{ PCI_VDEVICE(INTEL, DID_ADL_N_SKU8), .driver_data = (kernel_ulong_t)&adl_n_cfg },
	{ PCI_VDEVICE(INTEL, DID_ADL_N_SKU9), .driver_data = (kernel_ulong_t)&adl_n_cfg },
	{ PCI_VDEVICE(INTEL, DID_ADL_N_SKU10), .driver_data = (kernel_ulong_t)&adl_n_cfg },
	{ PCI_VDEVICE(INTEL, DID_ADL_N_SKU11), .driver_data = (kernel_ulong_t)&adl_n_cfg },
	{ PCI_VDEVICE(INTEL, DID_ADL_N_SKU12), .driver_data = (kernel_ulong_t)&adl_n_cfg },
	{ PCI_VDEVICE(INTEL, DID_AZB_SKU1), .driver_data = (kernel_ulong_t)&adl_n_cfg },
	{ PCI_VDEVICE(INTEL, DID_ASL_SKU1), .driver_data = (kernel_ulong_t)&adl_n_cfg },
	{ PCI_VDEVICE(INTEL, DID_ASL_SKU2), .driver_data = (kernel_ulong_t)&adl_n_cfg },
	{ PCI_VDEVICE(INTEL, DID_ASL_SKU3), .driver_data = (kernel_ulong_t)&adl_n_cfg },
	{ PCI_VDEVICE(INTEL, DID_RPL_P_SKU1), .driver_data = (kernel_ulong_t)&rpl_p_cfg },
	{ PCI_VDEVICE(INTEL, DID_RPL_P_SKU2), .driver_data = (kernel_ulong_t)&rpl_p_cfg },
	{ PCI_VDEVICE(INTEL, DID_RPL_P_SKU3), .driver_data = (kernel_ulong_t)&rpl_p_cfg },
	{ PCI_VDEVICE(INTEL, DID_RPL_P_SKU4), .driver_data = (kernel_ulong_t)&rpl_p_cfg },
	{ PCI_VDEVICE(INTEL, DID_RPL_P_SKU5), .driver_data = (kernel_ulong_t)&rpl_p_cfg },
	{ PCI_VDEVICE(INTEL, DID_MTL_PS_SKU1), .driver_data = (kernel_ulong_t)&mtl_ps_cfg },
	{ PCI_VDEVICE(INTEL, DID_MTL_PS_SKU2), .driver_data = (kernel_ulong_t)&mtl_ps_cfg },
	{ PCI_VDEVICE(INTEL, DID_MTL_PS_SKU3), .driver_data = (kernel_ulong_t)&mtl_ps_cfg },
/* …（同文件无关代码省略）… */
	{ PCI_VDEVICE(INTEL, DID_ARL_UH_SKU1), .driver_data = (kernel_ulong_t)&mtl_p_cfg },
	{ PCI_VDEVICE(INTEL, DID_ARL_UH_SKU2), .driver_data = (kernel_ulong_t)&mtl_p_cfg },
	{ PCI_VDEVICE(INTEL, DID_ARL_UH_SKU3), .driver_data = (kernel_ulong_t)&mtl_p_cfg },
	{ PCI_VDEVICE(INTEL, DID_PTL_H_SKU1), .driver_data = (kernel_ulong_t)&ptl_h_cfg },
	{ PCI_VDEVICE(INTEL, DID_PTL_H_SKU2), .driver_data = (kernel_ulong_t)&ptl_h_cfg },
	{ PCI_VDEVICE(INTEL, DID_PTL_H_SKU3), .driver_data = (kernel_ulong_t)&ptl_h_cfg },
/* …（同文件无关代码省略）… */
	{ PCI_VDEVICE(INTEL, DID_PTL_H_SKU12), .driver_data = (kernel_ulong_t)&ptl_h_cfg },
	{ PCI_VDEVICE(INTEL, DID_PTL_H_SKU13), .driver_data = (kernel_ulong_t)&ptl_h_cfg },
	{ PCI_VDEVICE(INTEL, DID_PTL_H_SKU14), .driver_data = (kernel_ulong_t)&ptl_h_cfg },
	{ PCI_VDEVICE(INTEL, DID_WCL_SKU1), .driver_data = (kernel_ulong_t)&wcl_cfg },
	{ PCI_VDEVICE(INTEL, DID_NVL_H_SKU1), .driver_data = (kernel_ulong_t)&nvl_h_cfg },
	{ PCI_VDEVICE(INTEL, DID_NVL_H_SKU2), .driver_data = (kernel_ulong_t)&nvl_h_cfg },
	{ PCI_VDEVICE(INTEL, DID_NVL_H_SKU3), .driver_data = (kernel_ulong_t)&nvl_h_cfg },
/* …（同文件无关代码省略）… */
static enum mem_type get_mem_type(struct igen6_imc *imc)
{
	u32 val;

	if (res_cfg->get_mem_type)
		return res_cfg->get_mem_type(imc);

	val = readl(imc->window + MAD_INTER_CHANNEL_OFFSET);

	switch (MAD_INTER_CHANNEL_DDR_TYPE(val)) {
	case 0:
		return MEM_DDR4;
	case 1:
		return MEM_DDR3;
	case 2:
		return MEM_LPDDR3;
	case 3:
		return MEM_LPDDR4;
	case 4:
		return MEM_WIO2;
	default:
		return MEM_UNKNOWN;
	}
}
/* …（同文件无关代码省略）… */
static bool large_dimm(struct igen6_imc *imc, int chan, int dimm)
{
	return dimm == imc->dimm_l_map[chan];
}
/* …（同文件无关代码省略）… */
static enum dev_type get_dev_type(struct igen6_imc *imc, int chan, int dimm)
{
	u32 width, val;

	if (res_cfg->get_dev_type)
		return res_cfg->get_dev_type(imc, chan, dimm);

	val = readl(imc->window + MAD_DIMM_CH0_OFFSET + chan * 4);
	width = large_dimm(imc, chan, dimm) ? MAD_DIMM_CH_DLW(val) :
					  MAD_DIMM_CH_DSW(val);

	switch (width) {
	case 0:
		return DEV_X8;
	case 1:
		return DEV_X16;
	case 2:
		return DEV_X32;
	default:
		return DEV_UNKNOWN;
	}
}
/* …（同文件无关代码省略）… */
static u64 get_dimm_size(struct igen6_imc *imc, int chan, int dimm)
{
	if (large_dimm(imc, chan, dimm))
		return imc->dimm_l_size[chan];

	return imc->dimm_s_size[chan];
}
/* …（同文件无关代码省略）… */
static void set_chan_params(struct igen6_imc *imc)
{
	u32 val;

	if (res_cfg->set_chan_params) {
		res_cfg->set_chan_params(imc);
		return;
	}

	val = readl(imc->window + MAD_INTER_CHANNEL_OFFSET);
	imc->ch_s_size = MAD_INTER_CHANNEL_CH_S_SIZE(val);
	imc->ch_l_map = MAD_INTER_CHANNEL_CH_L_MAP(val);
}
/* …（同文件无关代码省略）… */
static void set_dimm_params(struct igen6_imc *imc, int chan)
{
	u32 val;

	if (res_cfg->set_dimm_params) {
		res_cfg->set_dimm_params(imc, chan);
		return;
	}

	val = readl(imc->window + MAD_INTRA_CH0_OFFSET + chan * 4);
	imc->dimm_l_map[chan]  = MAD_INTRA_CH_DIMM_L_MAP(val);

	val = readl(imc->window + MAD_DIMM_CH0_OFFSET + chan * 4);
	imc->dimm_l_size[chan] = MAD_DIMM_CH_DIMM_L_SIZE(val);
	imc->dimm_s_size[chan] = MAD_DIMM_CH_DIMM_S_SIZE(val);
}

static int decode_chan_idx(u64 addr, u64 mask, int intlv_bit)
{
	u64 hash_addr = addr & mask, hash = 0;
	u64 intlv = (addr >> intlv_bit) & 1;
	int i;

	for (i = 6; i < 20; i++)
		hash ^= (hash_addr >> i) & 1;

	return (int)hash ^ intlv;
}

static u64 decode_channel_addr(u64 addr, int intlv_bit)
{
	u64 channel_addr;

	/* Remove the interleave bit and shift upper part down to fill gap */
	channel_addr  = GET_BITFIELD(addr, intlv_bit + 1, 63) << intlv_bit;
	channel_addr |= GET_BITFIELD(addr, 0, intlv_bit - 1);

	return channel_addr;
}

static void decode_addr(u64 addr, u32 hash, u64 s_size, int l_map,
			int *idx, u64 *sub_addr)
{
	int intlv_bit = CHANNEL_HASH_LSB_MASK_BIT(hash) + 6;

	if (addr > 2 * s_size) {
		*sub_addr = addr - s_size;
		*idx = l_map;
		return;
	}

	if (CHANNEL_HASH_MODE(hash)) {
		*sub_addr = decode_channel_addr(addr, intlv_bit);
		*idx = decode_chan_idx(addr, CHANNEL_HASH_MASK(hash), intlv_bit);
	} else {
		*sub_addr = decode_channel_addr(addr, 6);
		*idx = GET_BITFIELD(addr, 6, 6);
	}
}

static int igen6_decode(struct decoded_addr *res)
{
	struct igen6_imc *imc = &igen6_pvt->imc[res->mc];
	u64 addr = res->imc_addr, sub_addr, s_size;
	int idx, l_map;
	u32 hash;

	if (addr >= igen6_tom) {
		edac_dbg(0, "Address 0x%llx out of range\n", addr);
/* …（同文件无关代码省略）… */
	hash   = readl(imc->window + CHANNEL_HASH_OFFSET);
	s_size = imc->ch_s_size;
	l_map  = imc->ch_l_map;
	decode_addr(addr, hash, s_size, l_map, &idx, &sub_addr);
	res->channel_idx  = idx;
	res->channel_addr = sub_addr;

	/* Decode sub-channel/DIMM */
	hash   = readl(imc->window + CHANNEL_EHASH_OFFSET);
	s_size = imc->dimm_s_size[idx];
	l_map  = imc->dimm_l_map[idx];
	decode_addr(res->channel_addr, hash, s_size, l_map, &idx, &sub_addr);
	res->sub_channel_idx  = idx;
	res->sub_channel_addr = sub_addr;

	return 0;
}
/* …（同文件无关代码省略）… */
static int ecclog_gen_pool_add(int mc, u64 ecclog)
{
	struct ecclog_node *node;

	node = (void *)gen_pool_alloc(ecclog_pool, sizeof(*node));
	if (!node)
		return -ENOMEM;

	node->mc = mc;
	node->ecclog = ecclog;
	llist_add(&node->llnode, &ecclog_llist);

	return 0;
}
/* …（同文件无关代码省略）… */
static u64 ecclog_read_and_clear(struct igen6_imc *imc)
{
	u64 ecclog = readq(imc->window + ECC_ERROR_LOG_OFFSET);

	/*
	 * Quirk: The ECC_ERROR_LOG register of certain SoCs may contain
	 *        the invalid value ~0. This will result in a flood of invalid
	 *        error reports in polling mode. Skip it.
	 */
	if (ecclog == ~0)
		return 0;

	/* Neither a CE nor a UE. Skip it.*/
	if (!(ecclog & (ECC_ERROR_LOG_CE | ECC_ERROR_LOG_UE)))
		return 0;

	/* Clear CE/UE bits by writing 1s */
	writeq(ecclog, imc->window + ECC_ERROR_LOG_OFFSET);

	return ecclog;
}
/* …（同文件无关代码省略）… */

	llist_for_each_entry_safe(node, tmp, head, llnode) {
		memset(&res, 0, sizeof(res));
		if (res_cfg->err_addr)
			eaddr = res_cfg->err_addr(node->ecclog);
		else
			eaddr = node->ecclog & res_cfg->reg_eccerrlog_addr_mask;

		res.mc	     = node->mc;
		res.sys_addr = res_cfg->err_addr_to_sys_addr(eaddr, res.mc);
		res.imc_addr = res_cfg->err_addr_to_imc_addr(eaddr, res.mc);
/* …（同文件无关代码省略）… */
static bool igen6_check_ecc(struct igen6_imc *imc)
{
	u32 activate = readl(imc->window + IBECC_ACTIVATE_OFFSET);

	return !!(activate & IBECC_ACTIVATE_EN);
}
/* …（同文件无关代码省略）… */
static int igen6_get_dimm_config(struct mem_ctl_info *mci)
{
	struct igen6_imc *imc = mci->pvt_info;
	int i, j, ndimms, mc = imc->mc;
	struct dimm_info *dimm;
	enum mem_type mtype;
	enum dev_type dtype;
	u64 dsize;
	bool ecc;

	edac_dbg(2, "\n");

	mtype = get_mem_type(imc);
	ecc = igen6_check_ecc(imc);
	set_chan_params(imc);

	for (i = 0; i < NUM_CHANNELS; i++) {
		set_dimm_params(imc, i);
		imc->size += imc->dimm_s_size[i];
		imc->size += imc->dimm_l_size[i];
		ndimms = 0;

		for (j = 0; j < NUM_DIMMS; j++) {
			dimm = edac_get_dimm(mci, i, j, 0);
			dtype = get_dev_type(imc, i, j);
			dsize = get_dimm_size(imc, i, j);

			if (!dsize)
				continue;

			dimm->grain = 64;
			dimm->mtype = mtype;
			dimm->dtype = dtype;
			dimm->nr_pages  = MiB_TO_PAGES(dsize >> 20);
			dimm->edac_mode = EDAC_SECDED;
			snprintf(dimm->label, sizeof(dimm->label),
				 "MC#%d_Chan#%d_DIMM#%d", mc, i, j);
			edac_dbg(0, "MC %d, Channel %d, DIMM %d, Size %llu MiB (%u pages)\n",
				 mc, i, j, dsize >> 20, dimm->nr_pages);

			ndimms++;
		}

		if (ndimms && !ecc) {
			igen6_printk(KERN_ERR, "MC%d In-Band ECC is disabled\n", mc);
			return -ENODEV;
		}
	}

	edac_dbg(0, "MC %d, total size %llu MiB\n", mc, imc->size >> 20);

	return 0;
}
/* …（同文件无关代码省略）… */
static void igen6_reg_dump(struct igen6_imc *imc)
{
	int i;

	edac_dbg(2, "CHANNEL_HASH     : 0x%x\n",
		 readl(imc->window + CHANNEL_HASH_OFFSET));
	edac_dbg(2, "CHANNEL_EHASH    : 0x%x\n",
		 readl(imc->window + CHANNEL_EHASH_OFFSET));
	edac_dbg(2, "MAD_INTER_CHANNEL: 0x%x\n",
		 readl(imc->window + MAD_INTER_CHANNEL_OFFSET));
	edac_dbg(2, "ECC_ERROR_LOG    : 0x%llx\n",
		 readq(imc->window + ECC_ERROR_LOG_OFFSET));

	for (i = 0; i < NUM_CHANNELS; i++) {
		edac_dbg(2, "MAD_INTRA_CH%d    : 0x%x\n", i,
			 readl(imc->window + MAD_INTRA_CH0_OFFSET + i * 4));
		edac_dbg(2, "MAD_DIMM_CH%d     : 0x%x\n", i,
			 readl(imc->window + MAD_DIMM_CH0_OFFSET + i * 4));
	}
	edac_dbg(2, "TOLUD            : 0x%x", igen6_tolud);
	edac_dbg(2, "TOUUD            : 0x%llx", igen6_touud);
	edac_dbg(2, "TOM              : 0x%llx", igen6_tom);
}
/* …（同文件无关代码省略）… */
{
	void __iomem *memss_pma_cr;
	struct igen6_pvt *pvt;
	u64 mchbar;
	int rc;

	pvt = kzalloc_obj(*igen6_pvt);
	if (!pvt)
		return NULL;

	rc = get_mchbar(pdev, &mchbar);
	if (rc) {
		kfree(pvt);
		return NULL;
	}

	memss_pma_cr = ioremap(mchbar, MCHBAR_SIZE * 2);
	if (!memss_pma_cr) {
/* …（同文件无关代码省略）… */
static void igen6_check(struct mem_ctl_info *mci)
{
	struct igen6_imc *imc = mci->pvt_info;
	u64 ecclog;

	/* errsts_clear() isn't NMI-safe. Delay it in the IRQ context */
	ecclog = ecclog_read_and_clear(imc);
	if (!ecclog)
		return;

	if (!ecclog_gen_pool_add(imc->mc, ecclog))
		irq_work_queue(&ecclog_irq_work);
}

/* Check whether the memory controller is absent. */
static bool igen6_imc_absent(void __iomem *window)
{
	return readl(window + MAD_INTER_CHANNEL_OFFSET) == ~0;
}

static void imc_release(struct device *dev)
{
	/* Nothing to do, the 'imc' owns the 'dev' and will also release it. */
/* …（同文件无关代码省略）… */
static int igen6_register_mci(int mc, void __iomem *window, struct pci_dev *pdev)
{
	struct edac_mc_layer layers[2];
	struct mem_ctl_info *mci;
	struct igen6_imc *imc;
	int rc;

	edac_dbg(2, "\n");

	layers[0].type = EDAC_MC_LAYER_CHANNEL;
	layers[0].size = NUM_CHANNELS;
	layers[0].is_virt_csrow = false;
	layers[1].type = EDAC_MC_LAYER_SLOT;
	layers[1].size = NUM_DIMMS;
	layers[1].is_virt_csrow = true;

	mci = edac_mc_alloc(mc, ARRAY_SIZE(layers), layers, 0);
	if (!mci) {
		rc = -ENOMEM;
		goto fail;
	}

	mci->ctl_name = kasprintf(GFP_KERNEL, "Intel_client_SoC MC#%d", mc);
	if (!mci->ctl_name) {
		rc = -ENOMEM;
		goto fail2;
	}

	mci->mtype_cap = MEM_FLAG_LPDDR4 | MEM_FLAG_DDR4;
	mci->edac_ctl_cap = EDAC_FLAG_SECDED;
	mci->edac_cap = EDAC_FLAG_SECDED;
	mci->mod_name = EDAC_MOD_STR;
	mci->dev_name = pci_name(pdev);
	if (edac_op_state == EDAC_OPSTATE_POLL)
		mci->edac_check = igen6_check;
	mci->pvt_info = &igen6_pvt->imc[mc];

	imc = mci->pvt_info;
	imc->dev.release = imc_release;
	device_initialize(&imc->dev);
	/*
	 * EDAC core uses mci->pdev(pointer of structure device) as
	 * memory controller ID. The client SoCs attach one or more
	 * memory controllers to single pci_dev (single pci_dev->dev
	 * can be for multiple memory controllers).
	 *
	 * To make mci->pdev unique, assign pci_dev->dev to mci->pdev
	 * for the first memory controller and assign a unique imc->dev
	 * to mci->pdev for each non-first memory controller.
	 */
	mci->pdev = mc ? &imc->dev : &pdev->dev;
	imc->mc	= mc;
	imc->pdev = pdev;
	imc->window = window;

	igen6_reg_dump(imc);

	rc = igen6_get_dimm_config(mci);
	if (rc)
		goto fail3;

	rc = edac_mc_add_mc(mci);
	if (rc) {
		igen6_printk(KERN_ERR, "Failed to register mci#%d\n", mc);
		goto fail3;
	}

	imc->mci = mci;
	return 0;
fail3:
	put_device(&imc->dev);
	mci->pvt_info = NULL;
	kfree(mci->ctl_name);
fail2:
	edac_mc_free(mci);
fail:
	return rc;
}
/* …（同文件无关代码省略）… */
static void igen6_unregister_mcis(void)
{
	struct mem_ctl_info *mci;
	struct igen6_imc *imc;
	int i;

	edac_dbg(2, "\n");

	for (i = 0; i < res_cfg->num_imc; i++) {
		imc = &igen6_pvt->imc[i];
		mci = imc->mci;
		if (!mci)
			continue;

		edac_mc_del_mc(mci->pdev);
		kfree(mci->ctl_name);
		mci->pvt_info = NULL;
		edac_mc_free(mci);
		put_device(&imc->dev);
		iounmap(imc->window);
	}
}
/* …（同文件无关代码省略）… */
{
	void __iomem *window;
	int lmc, pmc, rc;
	u64 base;

	for (lmc = 0, pmc = 0; pmc < NUM_IMC; pmc++) {
		base   = mchbar + pmc * MCHBAR_SIZE;
		window = ioremap(base, MCHBAR_SIZE);
		if (!window) {
			igen6_printk(KERN_ERR, "Failed to ioremap 0x%llx for mc%d\n", base, pmc);
			rc = -ENOMEM;
			goto out_unregister_mcis;
		}

		if (igen6_imc_absent(window)) {
			iounmap(window);
			edac_dbg(2, "Skip absent mc%d\n", pmc);
			continue;
		}

		rc = igen6_register_mci(lmc, window, pdev);
		if (rc)
			goto out_iounmap;

		/* Done, if all present MCs are detected and registered. */
		if (++lmc >= res_cfg->num_imc)
/* …（同文件无关代码省略）… */

	return 0;

out_iounmap:
	iounmap(window);

out_unregister_mcis:
	igen6_unregister_mcis();

	return rc;
