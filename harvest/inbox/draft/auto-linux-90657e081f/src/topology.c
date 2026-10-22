// AUTO-DRAFT from torvalds/linux PR #df37e3924292fb41d325721008520ba93deced42
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#define read_corecnt()	read_sysreg_s(SYS_AMEVCNTR0_CORE_EL0)
#define read_constcnt()	read_sysreg_s(SYS_AMEVCNTR0_CONST_EL0)
/* …（同文件无关代码省略）… */
struct amu_ffh_ctrs {
	u64 corecnt;
	u64 constcnt;
};
/* …（同文件无关代码省略）… */
static void cpu_read_corecnt(void *val)
{
	/*
	 * A value of 0 can be returned if the current CPU does not support AMUs
	 * or if the counter is disabled for this CPU. A return value of 0 at
	 * counter read is properly handled as an error case by the users of the
	 * counter.
	 */
	*(u64 *)val = read_corecnt();
}
/* …（同文件无关代码省略）… */
static void cpu_read_constcnt(void *val)
{
	/*
	 * Return 0 if the current CPU is affected by a HW erratum.
	 * A value of 0 is also returned if the current CPU does not
	 * support AMUs or if the counter is disabled. A return
	 * value of 0 at counter read is properly handled as an error
	 * case by the users of the counter.
	 */
	*(u64 *)val = this_cpu_has_cap(ARM64_WORKAROUND_BROKEN_AMU_CONSTCNT) ?
		      0UL : read_constcnt();
}
/* …（同文件无关代码省略）… */
static inline
int counters_read_on_cpu(int cpu, smp_call_func_t func, void *val)
{
	/*
	 * Abort call on counterless CPU.
	 */
	if (!cpu_has_amu_feat(cpu))
		return -EOPNOTSUPP;

	if (irqs_disabled()) {
		/*
		 * When IRQs are disabled (tick path: sched_tick ->
		 * topology_scale_freq_tick or cppc_scale_freq_tick), only local
		 * CPU counter reads are allowed. Remote CPU counter read would
		 * require smp_call_function_single() which is unsafe with IRQs
		 * disabled.
		 */
		if (WARN_ON_ONCE(cpu != smp_processor_id()))
			return -EPERM;
		func(val);
	} else {
		smp_call_function_single(cpu, func, val, 1);
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static void amu_read_core_const_ctrs(void *val)
{
	struct amu_ffh_ctrs *ctrs = val;

	/*
	 * cpu_read_constcnt() incurs slight latency due to the
	 * ARM64_WORKAROUND_BROKEN_AMU_CONSTCNT check.
	 * Read it first to minimize the sampling skew between the const
	 * and core counters.
	 */
	cpu_read_constcnt(&ctrs->constcnt);
	cpu_read_corecnt(&ctrs->corecnt);
}

static u64 cpc_ffh_extract_bits(const struct cpc_reg *reg, u64 val)
{
	val &= GENMASK_ULL(reg->bit_offset + reg->bit_width - 1,
/* …（同文件无关代码省略）… */
static bool is_amu_ctr_reg(const struct cpc_reg *reg)
{
	return reg->address == CPC_FFH_CTR_CORE ||
		reg->address == CPC_FFH_CTR_CONST;
}
/* …（同文件无关代码省略）… */
	struct amu_ffh_ctrs ctrs;
	int ret;

	if (!is_amu_ctr_reg(reg1) || !is_amu_ctr_reg(reg2))
		return -EINVAL;

	ret = counters_read_on_cpu(cpu, amu_read_core_const_ctrs, &ctrs);
/* …（同文件无关代码省略）… */
{
	int ret = -EOPNOTSUPP;

	switch ((u64)reg->address) {
	case CPC_FFH_CTR_CORE:
		ret = counters_read_on_cpu(cpu, cpu_read_corecnt, val);
