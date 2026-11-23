// AUTO-DRAFT from torvalds/linux PR #2681617eb9317444247380298564f58142fc4ff8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

#define __riscv_v_vstate_or(_val, TYPE) ({				\
	typeof(_val) _res = _val;					\
	if (has_xtheadvector()) \
		_res = (_res & ~SR_VS_THEAD) | SR_VS_##TYPE##_THEAD;	\
	else								\
		_res = (_res & ~SR_VS) | SR_VS_##TYPE;			\
	_res;								\
})
/* …（同文件无关代码省略）… */
#define __riscv_v_vstate_check(_val, TYPE) ({				\
	bool _res;							\
	if (has_xtheadvector()) \
		_res = ((_val) & SR_VS_THEAD) == SR_VS_##TYPE##_THEAD;	\
	else								\
		_res = ((_val) & SR_VS) == SR_VS_##TYPE;		\
	_res;								\
})
/* …（同文件无关代码省略）… */
static __always_inline bool has_xtheadvector(void)
{
	if (IS_ENABLED(CONFIG_RISCV_ISA_XTHEADVECTOR))
		return riscv_has_vendor_extension_unlikely(THEAD_VENDOR_ID,
							   RISCV_ISA_VENDOR_EXT_XTHEADVECTOR);
	else
		return false;
}
/* …（同文件无关代码省略）… */
static inline void __riscv_v_vstate_clean(struct pt_regs *regs)
{
	regs->status = __riscv_v_vstate_or(regs->status, CLEAN);
}
/* …（同文件无关代码省略）… */
static inline bool riscv_v_vstate_query(struct pt_regs *regs)
{
	return !__riscv_v_vstate_check(regs->status, OFF);
}
/* …（同文件无关代码省略）… */
static __always_inline void riscv_v_enable(void)
{
	if (has_xtheadvector())
		csr_set(CSR_SSTATUS, SR_VS_THEAD);
	else
		csr_set(CSR_SSTATUS, SR_VS);
}
/* …（同文件无关代码省略）… */
static __always_inline void riscv_v_disable(void)
{
	if (has_xtheadvector())
		csr_clear(CSR_SSTATUS, SR_VS_THEAD);
	else
		csr_clear(CSR_SSTATUS, SR_VS);
}
/* …（同文件无关代码省略）… */
static __always_inline void __vstate_csr_restore(struct __riscv_v_ext_state *src)
{
	asm volatile (
		".option push\n\t"
		".option arch, +zve32x\n\t"
		"vsetvl	 x0, %2, %1\n\t"
		".option pop\n\t"
		"csrw	" __stringify(CSR_VSTART) ", %0\n\t"
		: : "r" (src->vstart), "r" (src->vtype), "r" (src->vl));

	if (has_xtheadvector()) {
		unsigned long status = csr_read(CSR_SSTATUS);

		/*
		 * Similar to __vstate_csr_save above, restore values for the
		 * separate VXRM and VXSAT CSRs from the vcsr variable.
		 */
		status = csr_read_set(CSR_STATUS, SR_FS_DIRTY);

		csr_write(CSR_VXRM, (src->vcsr >> CSR_VXRM_SHIFT) & CSR_VXRM_MASK);
		csr_write(CSR_VXSAT, src->vcsr & CSR_VXSAT_MASK);

		if ((status & SR_FS) != SR_FS_DIRTY)
			csr_write(CSR_STATUS, status);
	} else {
		csr_write(CSR_VCSR, src->vcsr);
	}
}
/* …（同文件无关代码省略）… */
static inline void __riscv_v_vstate_restore(struct __riscv_v_ext_state *restore_from,
					    void *datap)
{
	unsigned long vl;

	riscv_v_enable();
	if (has_xtheadvector()) {
		asm volatile (
			"mv t0, %0\n\t"
			THEAD_VSETVLI_T4X0E8M8D1
			THEAD_VLB_V_V0T0
			"add		t0, t0, t4\n\t"
			THEAD_VLB_V_V8T0
			"add		t0, t0, t4\n\t"
			THEAD_VLB_V_V16T0
			"add		t0, t0, t4\n\t"
			THEAD_VLB_V_V24T0
			: : "r" (datap) : "memory", "t0", "t4");
	} else {
		asm volatile (
			".option push\n\t"
			".option arch, +zve32x\n\t"
			"vsetvli	%0, x0, e8, m8, ta, ma\n\t"
			"vle8.v		v0, (%1)\n\t"
			"add		%1, %1, %0\n\t"
			"vle8.v		v8, (%1)\n\t"
			"add		%1, %1, %0\n\t"
			"vle8.v		v16, (%1)\n\t"
			"add		%1, %1, %0\n\t"
			"vle8.v		v24, (%1)\n\t"
			".option pop\n\t"
			: "=&r" (vl), "+r" (datap) : : "memory");
	}
	__vstate_csr_restore(restore_from);
	riscv_v_disable();
}
/* …（同文件无关代码省略）… */
static inline void riscv_v_vstate_restore(struct __riscv_v_ext_state *vstate,
					  struct pt_regs *regs)
{
	if (riscv_v_vstate_query(regs)) {
		__riscv_v_vstate_restore(vstate, vstate->datap);
		__riscv_v_vstate_clean(regs);
