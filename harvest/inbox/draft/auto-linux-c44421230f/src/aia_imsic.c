// AUTO-DRAFT from torvalds/linux PR #eff8d2791c086388ba5bae36385afd9bc6f0507e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <time.h>
  // <<< BUG ANCHOR
#define IMSIC_MAX_EIX	(IMSIC_MAX_ID / BITS_PER_TYPE(u64))
/* …（同文件无关代码省略）… */
struct imsic_mrif_eix {
	unsigned long eip[BITS_PER_TYPE(u64) / BITS_PER_LONG];
	unsigned long eie[BITS_PER_TYPE(u64) / BITS_PER_LONG];
};
/* …（同文件无关代码省略）… */
struct imsic_mrif {
	struct imsic_mrif_eix eix[IMSIC_MAX_EIX];
	unsigned long eithreshold;
	unsigned long eidelivery;
};
/* …（同文件无关代码省略）… */
struct imsic {
	struct kvm_io_device iodev;

	u32 nr_msis;
	u32 nr_eix;
	u32 nr_hw_eix;

	/*
	 * At any point in time, the register state is in
	 * one of the following places:
	 *
	 * 1) Hardware: IMSIC VS-file (vsfile_cpu >= 0)
	 * 2) Software: IMSIC SW-file (vsfile_cpu < 0)
	 */

	/* IMSIC VS-file */
	rwlock_t vsfile_lock;
	int vsfile_cpu;
	int vsfile_hgei;
	void __iomem *vsfile_va;
	phys_addr_t vsfile_pa;

	/* IMSIC SW-file */
	struct imsic_mrif *swfile;
	phys_addr_t swfile_pa;
	raw_spinlock_t swfile_extirq_lock;
};
/* …（同文件无关代码省略）… */
#define imsic_vs_csr_read(__c)			\
({						\
	unsigned long __r;			\
	csr_write(CSR_VSISELECT, __c);		\
	__r = csr_read(CSR_VSIREG);		\
	__r;					\
})
/* …（同文件无关代码省略）… */
#define imsic_read_switchcase_32(__ireg)		\
	imsic_read_switchcase_16(__ireg + 0)		\
	imsic_read_switchcase_16(__ireg + 16)
#define imsic_read_switchcase_64(__ireg)		\
	imsic_read_switchcase_32(__ireg + 0)		\
	imsic_read_switchcase_32(__ireg + 32)
/* …（同文件无关代码省略）… */
static unsigned long imsic_eix_read(int ireg)
{
	switch (ireg) {
	imsic_read_switchcase_64(IMSIC_EIP0)
	imsic_read_switchcase_64(IMSIC_EIE0)
	}

	return 0;
}
/* …（同文件无关代码省略）… */
#define imsic_vs_csr_write(__c, __v)		\
do {						\
	csr_write(CSR_VSISELECT, __c);		\
	csr_write(CSR_VSIREG, __v);		\
} while (0)
/* …（同文件无关代码省略）… */
#define imsic_write_switchcase_32(__ireg, __v)		\
	imsic_write_switchcase_16(__ireg + 0, __v)	\
	imsic_write_switchcase_16(__ireg + 16, __v)
#define imsic_write_switchcase_64(__ireg, __v)		\
	imsic_write_switchcase_32(__ireg + 0, __v)	\
	imsic_write_switchcase_32(__ireg + 32, __v)
/* …（同文件无关代码省略）… */
static void imsic_eix_write(int ireg, unsigned long val)
{
	switch (ireg) {
	imsic_write_switchcase_64(IMSIC_EIP0, val)
	imsic_write_switchcase_64(IMSIC_EIE0, val)
	}
}
/* …（同文件无关代码省略）… */
static int imsic_mrif_isel_check(u32 nr_eix, unsigned long isel)
{
	u32 num = 0;

	switch (isel) {
	case IMSIC_EIDELIVERY:
	case IMSIC_EITHRESHOLD:
		break;
	case IMSIC_EIP0 ... IMSIC_EIP63:
		num = isel - IMSIC_EIP0;
		break;
	case IMSIC_EIE0 ... IMSIC_EIE63:
		num = isel - IMSIC_EIE0;
		break;
	default:
		return -ENOENT;
	}
#ifndef CONFIG_32BIT
	if (num & 0x1)
		return -EINVAL;
#endif
	if ((num / 2) >= nr_eix)
		return -EINVAL;

	return 0;
}
/* …（同文件无关代码省略）… */
struct imsic_vsfile_rw_data {
	int hgei;
	int isel;
	bool write;
	unsigned long val;
};
/* …（同文件无关代码省略）… */
static void imsic_vsfile_local_rw(void *data)
{
	struct imsic_vsfile_rw_data *idata = data;
	unsigned long new_hstatus, old_hstatus, old_vsiselect;

	old_vsiselect = csr_read(CSR_VSISELECT);
	old_hstatus = csr_read(CSR_HSTATUS);
	new_hstatus = old_hstatus & ~HSTATUS_VGEIN;
	new_hstatus |= ((unsigned long)idata->hgei) << HSTATUS_VGEIN_SHIFT;
	csr_write(CSR_HSTATUS, new_hstatus);

	switch (idata->isel) {
	case IMSIC_EIDELIVERY:
		if (idata->write)
			imsic_vs_csr_write(IMSIC_EIDELIVERY, idata->val);
		else
			idata->val = imsic_vs_csr_read(IMSIC_EIDELIVERY);
		break;
	case IMSIC_EITHRESHOLD:
		if (idata->write)
			imsic_vs_csr_write(IMSIC_EITHRESHOLD, idata->val);
		else
			idata->val = imsic_vs_csr_read(IMSIC_EITHRESHOLD);
		break;
	case IMSIC_EIP0 ... IMSIC_EIP63:
	case IMSIC_EIE0 ... IMSIC_EIE63:
#ifndef CONFIG_32BIT
		if (idata->isel & 0x1)
			break;
#endif
		if (idata->write)
			imsic_eix_write(idata->isel, idata->val);
		else
			idata->val = imsic_eix_read(idata->isel);
		break;
	default:
		break;
	}

	csr_write(CSR_HSTATUS, old_hstatus);
	csr_write(CSR_VSISELECT, old_vsiselect);
}
/* …（同文件无关代码省略）… */
static int imsic_vsfile_rw(int vsfile_hgei, int vsfile_cpu, u32 nr_eix,
			   unsigned long isel, bool write,
			   unsigned long *val)
{
	int rc;
	struct imsic_vsfile_rw_data rdata;

	/* We can only access register if we have a IMSIC VS-file */
	if (vsfile_cpu < 0 || vsfile_hgei <= 0)
		return -EINVAL;

	/* Check IMSIC register iselect */
	rc = imsic_mrif_isel_check(nr_eix, isel);
	if (rc)
		return rc;

	/* We can only access register on local CPU */
	rdata.hgei = vsfile_hgei;
	rdata.isel = isel;
	rdata.write = write;
	rdata.val = (write) ? *val : 0;
	on_each_cpu_mask(cpumask_of(vsfile_cpu),
			 imsic_vsfile_local_rw, &rdata, 1);

	if (!write)
		*val = rdata.val;

	return 0;
}
/* …（同文件无关代码省略）… */
	if (!vcpu)
		return -ENODEV;

	imsic = vcpu->arch.aia_context.imsic_state;
	if (!imsic)
		return -ENODEV;
	isel = KVM_DEV_RISCV_AIA_IMSIC_GET_ISEL(type);

	read_lock_irqsave(&imsic->vsfile_lock, flags);
/* …（同文件无关代码省略）… */
		rc = imsic_vsfile_rw(vsfile_hgei, vsfile_cpu, imsic->nr_eix,
				     isel, write, val);

	return rc;
}
