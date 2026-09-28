// AUTO-DRAFT from torvalds/linux PR #efb44d93a620c294c049db37c810c8ab7ee1122d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#define SPINUNIT		100	/* 100ns */
/* …（同文件无关代码省略）… */
void mce_prep_record_common(struct mce *m)
{
	m->cpuid	= cpuid_eax(1);
	m->cpuvendor	= boot_cpu_data.x86_vendor;
	m->mcgcap	= native_rdmsrq(MSR_IA32_MCG_CAP);
	/* need the internal __ version to avoid deadlocks */
	m->time		= __ktime_get_real_seconds();
}
/* …（同文件无关代码省略）… */
void mce_prep_record_per_cpu(unsigned int cpu, struct mce *m)
{
	m->cpu		= cpu;
	m->extcpu	= cpu;
	m->apicid	= cpu_data(cpu).topo.initial_apicid;
	m->microcode	= cpu_data(cpu).microcode;
	m->ppin		= topology_ppin(cpu);
	m->socketid	= topology_physical_package_id(cpu);
}
/* …（同文件无关代码省略）… */
void mce_prep_record(struct mce_hw_err *err)
{
	struct mce *m = &err->m;

	memset(err, 0, sizeof(struct mce_hw_err));
	mce_prep_record_common(m);
	mce_prep_record_per_cpu(smp_processor_id(), m);
}
/* …（同文件无关代码省略）… */
void mce_log(struct mce_hw_err *err)
{
	if (mce_gen_pool_add(err)) {
		pr_info(HW_ERR "Machine check events logged\n");
		irq_work_queue(&mce_irq_work);
	}
}
/* …（同文件无关代码省略）… */
static void __print_mce(struct mce_hw_err *err)
{
	struct mce *m = &err->m;

	pr_emerg(HW_ERR "CPU %d: Machine Check%s: %Lx Bank %d: %016Lx\n",
		 m->extcpu,
		 (m->mcgstatus & MCG_STATUS_MCIP ? " Exception" : ""),
		 m->mcgstatus, m->bank, m->status);

	if (m->ip) {
		pr_emerg(HW_ERR "RIP%s %02x:<%016Lx> ",
			!(m->mcgstatus & MCG_STATUS_EIPV) ? " !INEXACT!" : "",
			m->cs, m->ip);

		if (m->cs == __KERNEL_CS)
			pr_cont("{%pS}", (void *)(unsigned long)m->ip);
		pr_cont("\n");
	}

	pr_emerg(HW_ERR "TSC %llx ", m->tsc);
	if (m->addr)
		pr_cont("ADDR %llx ", m->addr);
	if (m->misc)
		pr_cont("MISC %llx ", m->misc);
	if (m->ppin)
		pr_cont("PPIN %llx ", m->ppin);

	if (mce_flags.smca) {
		if (m->synd)
			pr_cont("SYND %llx ", m->synd);
		if (err->vendor.amd.synd1)
			pr_cont("SYND1 %llx ", err->vendor.amd.synd1);
		if (err->vendor.amd.synd2)
			pr_cont("SYND2 %llx ", err->vendor.amd.synd2);
		if (m->ipid)
			pr_cont("IPID %llx ", m->ipid);
	}

	pr_cont("\n");

	/*
	 * Note this output is parsed by external tools and old fields
	 * should not be changed.
	 */
	pr_emerg(HW_ERR "PROCESSOR %u:%x TIME %llu SOCKET %u APIC %x microcode %x\n",
		m->cpuvendor, m->cpuid, m->time, m->socketid, m->apicid,
		m->microcode);
}
/* …（同文件无关代码省略）… */
static void print_mce(struct mce_hw_err *err)
{
	struct mce *m = &err->m;

	__print_mce(err);

	if (m->cpuvendor != X86_VENDOR_AMD && m->cpuvendor != X86_VENDOR_HYGON)
		pr_emerg_ratelimited(HW_ERR "Run the above through 'mcelog --ascii'\n");
}
/* …（同文件无关代码省略）… */
#define PANIC_TIMEOUT 5 /* 5 seconds */
/* …（同文件无关代码省略）… */
static void wait_for_panic(void)
{
	long timeout = PANIC_TIMEOUT*USEC_PER_SEC;

	preempt_disable();
	local_irq_enable();
	while (timeout-- > 0)
		udelay(1);
	if (panic_timeout == 0)
		panic_timeout = mca_cfg.panic_timeout;
	panic("Panicing machine check CPU died");
}
/* …（同文件无关代码省略）… */
static const char *mce_dump_aux_info(struct mce *m)
{
	if (boot_cpu_has_bug(X86_BUG_TDX_PW_MCE))
		return tdx_dump_mce_info(m);

	return NULL;
}
/* …（同文件无关代码省略）… */
static noinstr void mce_panic(const char *msg, struct mce_hw_err *final, char *exp)
{
	struct llist_node *pending;
	struct mce_evt_llist *l;
	int apei_err = 0;
	const char *memmsg;

	/*
	 * Allow instrumentation around external facilities usage. Not that it
	 * matters a whole lot since the machine is going to panic anyway.
	 */
	instrumentation_begin();

	if (!fake_panic) {
		/*
		 * Make sure only one CPU runs in machine check panic
		 */
		if (atomic_inc_return(&mce_panicked) > 1)
			wait_for_panic();
		barrier();

		bust_spinlocks(1);
		console_verbose();
	} else {
		/* Don't log too much for fake panic */
		if (atomic_inc_return(&mce_fake_panicked) > 1)
			goto out;
	}
	pending = mce_gen_pool_prepare_records();
	/* First print corrected ones that are still unlogged */
	llist_for_each_entry(l, pending, llnode) {
		struct mce_hw_err *err = &l->err;
		struct mce *m = &err->m;
		if (!(m->status & MCI_STATUS_UC)) {
			print_mce(err);
			if (!apei_err)
				apei_err = apei_write_mce(m);
		}
	}
	/* Now print uncorrected but with the final one last */
	llist_for_each_entry(l, pending, llnode) {
		struct mce_hw_err *err = &l->err;
		struct mce *m = &err->m;
		if (!(m->status & MCI_STATUS_UC))
			continue;
		if (!final || mce_cmp(m, &final->m)) {
			print_mce(err);
			if (!apei_err)
				apei_err = apei_write_mce(m);
		}
	}
	if (final) {
		print_mce(final);
		if (!apei_err)
			apei_err = apei_write_mce(&final->m);
	}
	if (exp)
		pr_emerg(HW_ERR "Machine check: %s\n", exp);

	memmsg = mce_dump_aux_info(&final->m);
	if (memmsg)
		pr_emerg(HW_ERR "Machine check: %s\n", memmsg);

	if (!fake_panic) {
		if (panic_timeout == 0)
			panic_timeout = mca_cfg.panic_timeout;

		/*
		 * Kdump skips the poisoned page in order to avoid
		 * touching the error bits again. Poison the page even
		 * if the error is fatal and the machine is about to
		 * panic.
		 */
		if (kexec_crash_loaded()) {
			if (final && (final->m.status & MCI_STATUS_ADDRV)) {
				struct page *p;
				p = pfn_to_online_page(final->m.addr >> PAGE_SHIFT);
				if (p)
					SetPageHWPoison(p);
			}
		}
		panic(msg);
	} else
		pr_emerg(HW_ERR "Fake kernel panic: %s\n", msg);

out:
	instrumentation_end();
}
/* …（同文件无关代码省略）… */
static int msr_to_offset(u32 msr)
{
	unsigned bank = __this_cpu_read(injectm.bank);

	if (msr == mca_cfg.rip_msr)
		return offsetof(struct mce, ip);
	if (msr == mca_msr_reg(bank, MCA_STATUS))
		return offsetof(struct mce, status);
	if (msr == mca_msr_reg(bank, MCA_ADDR))
		return offsetof(struct mce, addr);
	if (msr == mca_msr_reg(bank, MCA_MISC))
		return offsetof(struct mce, misc);
	if (msr == MSR_IA32_MCG_STATUS)
		return offsetof(struct mce, mcgstatus);
	return -1;
}
/* …（同文件无关代码省略）… */
noinstr u64 mce_rdmsrq(u32 msr)
{
	EAX_EDX_DECLARE_ARGS(val, low, high);

	if (__this_cpu_read(injectm.finished)) {
		int offset;
		u64 ret;

		instrumentation_begin();

		offset = msr_to_offset(msr);
		if (offset < 0)
			ret = 0;
		else
			ret = *(u64 *)((char *)this_cpu_ptr(&injectm) + offset);

		instrumentation_end();

		return ret;
	}

	/*
	 * RDMSR on MCA MSRs should not fault. If they do, this is very much an
	 * architectural violation and needs to be reported to hw vendor. Panic
	 * the box to not allow any further progress.
	 */
	asm volatile("1: rdmsr\n"
		     "2:\n"
		     _ASM_EXTABLE_TYPE(1b, 2b, EX_TYPE_RDMSR_IN_MCE)
		     : EAX_EDX_RET(val, low, high) : "c" (msr));


	return EAX_EDX_VAL(val, low, high);
}
/* …（同文件无关代码省略）… */
noinstr void mce_wrmsrq(u32 msr, u64 v)
{
	u32 low, high;

	if (__this_cpu_read(injectm.finished)) {
		int offset;

		instrumentation_begin();

		offset = msr_to_offset(msr);
		if (offset >= 0)
			*(u64 *)((char *)this_cpu_ptr(&injectm) + offset) = v;

		instrumentation_end();

		return;
	}

	low  = (u32)v;
	high = (u32)(v >> 32);

	/* See comment in mce_rdmsrq() */
	asm volatile("1: wrmsr\n"
		     "2:\n"
		     _ASM_EXTABLE_TYPE(1b, 2b, EX_TYPE_WRMSR_IN_MCE)
		     : : "c" (msr), "a"(low), "d" (high) : "memory");
}
/* …（同文件无关代码省略）… */
static noinstr void mce_gather_info(struct mce_hw_err *err, struct pt_regs *regs)
{
	struct mce *m;
	/*
	 * Enable instrumentation around mce_prep_record() which calls external
	 * facilities.
	 */
	instrumentation_begin();
	mce_prep_record(err);
	instrumentation_end();

	m = &err->m;
	m->mcgstatus = mce_rdmsrq(MSR_IA32_MCG_STATUS);
	if (regs) {
		/*
		 * Get the address of the instruction at the time of
		 * the machine check error.
		 */
		if (m->mcgstatus & (MCG_STATUS_RIPV|MCG_STATUS_EIPV)) {
			m->ip = regs->ip;
			m->cs = regs->cs;

			/*
			 * When in VM86 mode make the cs look like ring 3
			 * always. This is a lie, but it's better than passing
			 * the additional vm86 bit around everywhere.
			 */
			if (v8086_mode(regs))
				m->cs |= 3;
		}
		/* Use accurate RIP reporting if available. */
		if (mca_cfg.rip_msr)
			m->ip = mce_rdmsrq(mca_cfg.rip_msr);
	}
}
/* …（同文件无关代码省略）… */
bool mce_usable_address(struct mce *m)
{
	if (!(m->status & MCI_STATUS_ADDRV))
		return false;

	switch (m->cpuvendor) {
	case X86_VENDOR_AMD:
		return amd_mce_usable_address(m);

	case X86_VENDOR_INTEL:
	case X86_VENDOR_ZHAOXIN:
		return intel_mce_usable_address(m);

	default:
		return true;
	}
}
/* …（同文件无关代码省略）… */
static bool whole_page(struct mce *m)
{
	if (!mca_cfg.ser || !(m->status & MCI_STATUS_MISCV))
		return true;

	return MCI_MISC_ADDR_LSB(m->misc) >= PAGE_SHIFT;
}
/* …（同文件无关代码省略）… */
static noinstr void mce_read_aux(struct mce_hw_err *err, int i)
{
	struct mce *m = &err->m;

	if (m->status & MCI_STATUS_MISCV)
		m->misc = mce_rdmsrq(mca_msr_reg(i, MCA_MISC));

	if (m->status & MCI_STATUS_ADDRV) {
		if (m->kflags & MCE_CHECK_DFR_REGS)
			m->addr = mce_rdmsrq(MSR_AMD64_SMCA_MCx_DEADDR(i));
		else
			m->addr = mce_rdmsrq(mca_msr_reg(i, MCA_ADDR));

		/*
		 * Mask the reported address by the reported granularity.
		 */
		if (mca_cfg.ser && (m->status & MCI_STATUS_MISCV)) {
			u8 shift = MCI_MISC_ADDR_LSB(m->misc);
			m->addr >>= shift;
			m->addr <<= shift;
		}

		smca_extract_err_addr(m);
	}

	if (mce_flags.smca) {
		m->ipid = mce_rdmsrq(MSR_AMD64_SMCA_MCx_IPID(i));

		if (m->status & MCI_STATUS_SYNDV) {
			m->synd = mce_rdmsrq(MSR_AMD64_SMCA_MCx_SYND(i));
			err->vendor.amd.synd1 = mce_rdmsrq(MSR_AMD64_SMCA_MCx_SYND1(i));
			err->vendor.amd.synd2 = mce_rdmsrq(MSR_AMD64_SMCA_MCx_SYND2(i));
		}
	}
}
/* …（同文件无关代码省略）… */
static __always_inline void
quirk_sandybridge_ifu(int bank, struct mce *m, struct pt_regs *regs)
{
	if (bank != 0)
		return;
	if ((m->mcgstatus & (MCG_STATUS_EIPV|MCG_STATUS_RIPV)) != 0)
		return;
	if ((m->status & (MCI_STATUS_OVER|MCI_STATUS_UC|
		          MCI_STATUS_EN|MCI_STATUS_MISCV|MCI_STATUS_ADDRV|
			  MCI_STATUS_PCC|MCI_STATUS_S|MCI_STATUS_AR|
			  MCACOD)) !=
			 (MCI_STATUS_UC|MCI_STATUS_EN|
			  MCI_STATUS_MISCV|MCI_STATUS_ADDRV|MCI_STATUS_S|
			  MCI_STATUS_AR|MCACOD_INSTR))
		return;

	m->mcgstatus |= MCG_STATUS_EIPV;
	m->ip = regs->ip;
	m->cs = regs->cs;
}
/* …（同文件无关代码省略）… */
static noinstr bool quirk_skylake_repmov(void)
{
	u64 mcgstatus   = mce_rdmsrq(MSR_IA32_MCG_STATUS);
	u64 misc_enable = mce_rdmsrq(MSR_IA32_MISC_ENABLE);
	u64 mc1_status;

	/*
	 * Apply the quirk only to local machine checks, i.e., no broadcast
	 * sync is needed.
	 */
	if (!(mcgstatus & MCG_STATUS_LMCES) ||
	    !(misc_enable & MSR_IA32_MISC_ENABLE_FAST_STRING))
		return false;

	mc1_status = mce_rdmsrq(MSR_IA32_MCx_STATUS(1));

	/* Check for a software-recoverable data fetch error. */
	if ((mc1_status &
	     (MCI_STATUS_VAL | MCI_STATUS_OVER | MCI_STATUS_UC | MCI_STATUS_EN |
	      MCI_STATUS_ADDRV | MCI_STATUS_MISCV | MCI_STATUS_PCC |
	      MCI_STATUS_AR | MCI_STATUS_S)) ==
	     (MCI_STATUS_VAL |                   MCI_STATUS_UC | MCI_STATUS_EN |
	      MCI_STATUS_ADDRV | MCI_STATUS_MISCV |
	      MCI_STATUS_AR | MCI_STATUS_S)) {
		misc_enable &= ~MSR_IA32_MISC_ENABLE_FAST_STRING;
		mce_wrmsrq(MSR_IA32_MISC_ENABLE, misc_enable);
		mce_wrmsrq(MSR_IA32_MCx_STATUS(1), 0);

		instrumentation_begin();
		pr_err_once("Erratum detected, disable fast string copy instructions.\n");
		instrumentation_end();

		return true;
	}

	return false;
}
/* …（同文件无关代码省略）… */
static __always_inline void quirk_zen_ifu(int bank, struct mce *m, struct pt_regs *regs)
{
	if (bank != 1)
		return;
	if (!(m->status & MCI_STATUS_POISON))
		return;

	m->cs = regs->cs;
}
/* …（同文件无关代码省略）… */
static __always_inline int mce_no_way_out(struct mce_hw_err *err, char **msg, unsigned long *validp,
					  struct pt_regs *regs)
{
	struct mce *m = &err->m;
	char *tmp = *msg;
	int i;

	for (i = 0; i < this_cpu_read(mce_num_banks); i++) {
		m->status = mce_rdmsrq(mca_msr_reg(i, MCA_STATUS));
		if (!(m->status & MCI_STATUS_VAL))
			continue;

		arch___set_bit(i, validp);
		if (mce_flags.snb_ifu_quirk)
			quirk_sandybridge_ifu(i, m, regs);

		if (mce_flags.zen_ifu_quirk)
			quirk_zen_ifu(i, m, regs);

		m->bank = i;
		if (mce_severity(m, regs, &tmp, true) >= MCE_PANIC_SEVERITY) {
			mce_read_aux(err, i);
			*msg = tmp;
			return 1;
		}
	}
	return 0;
}
/* …（同文件无关代码省略）… */
static noinstr int mce_timed_out(u64 *t, const char *msg)
{
	int ret = 0;

	/* Enable instrumentation around calls to external facilities */
	instrumentation_begin();

	/*
	 * The others already did panic for some reason.
	 * Bail out like in a timeout.
	 * rmb() to tell the compiler that system_state
	 * might have been modified by someone else.
	 */
	rmb();
	if (atomic_read(&mce_panicked))
		wait_for_panic();
	if (!mca_cfg.monarch_timeout)
		goto out;
	if ((s64)*t < SPINUNIT) {
		if (cpumask_and(&mce_missing_cpus, cpu_online_mask, &mce_missing_cpus))
			pr_emerg("CPUs not responding to MCE broadcast (may include false positives): %*pbl\n",
				 cpumask_pr_args(&mce_missing_cpus));
		mce_panic(msg, NULL, NULL);

		ret = 1;
		goto out;
	}
	*t -= SPINUNIT;

out:
	touch_nmi_watchdog();

	instrumentation_end();

	return ret;
}
/* …（同文件无关代码省略）… */
static void mce_reign(void)
{
	struct mce_hw_err *err = NULL;
	struct mce *m = NULL;
	int global_worst = 0;
	char *msg = NULL;
	int cpu;

	/*
	 * This CPU is the Monarch and the other CPUs have run
	 * through their handlers.
	 * Grade the severity of the errors of all the CPUs.
	 */
	for_each_possible_cpu(cpu) {
		struct mce_hw_err *etmp = &per_cpu(hw_errs_seen, cpu);
		struct mce *mtmp = &etmp->m;

		if (mtmp->severity > global_worst) {
			global_worst = mtmp->severity;
			err = &per_cpu(hw_errs_seen, cpu);
			m = &err->m;
		}
	}

	/*
	 * Cannot recover? Panic here then.
	 * This dumps all the mces in the log buffer and stops the
	 * other CPUs.
	 */
	if (m && global_worst >= MCE_PANIC_SEVERITY) {
		/* call mce_severity() to get "msg" for panic */
		mce_severity(m, NULL, &msg, true);
		mce_panic("Fatal machine check", err, msg);
	}

	/*
	 * For UC somewhere we let the CPU who detects it handle it.
	 * Also must let continue the others, otherwise the handling
	 * CPU could deadlock on a lock.
	 */

	/*
	 * No machine check event found. Must be some external
	 * source or one CPU is hung. Panic.
	 */
	if (global_worst <= MCE_KEEP_SEVERITY)
		mce_panic("Fatal machine check from unknown source", NULL, NULL);

	/*
	 * Now clear all the hw_errs_seen so that they don't reappear on
	 * the next mce.
	 */
	for_each_possible_cpu(cpu)
		memset(&per_cpu(hw_errs_seen, cpu), 0, sizeof(struct mce_hw_err));
}
/* …（同文件无关代码省略）… */
static noinstr int mce_start(int *no_way_out)
{
	u64 timeout = (u64)mca_cfg.monarch_timeout * NSEC_PER_USEC;
	int order, ret = -1;

	if (!timeout)
		return ret;

	raw_atomic_add(*no_way_out, &global_nwo);
	/*
	 * Rely on the implied barrier below, such that global_nwo
	 * is updated before mce_callin.
	 */
	order = raw_atomic_inc_return(&mce_callin);
	arch_cpumask_clear_cpu(smp_processor_id(), &mce_missing_cpus);

	/* Enable instrumentation around calls to external facilities */
	instrumentation_begin();

	/*
	 * Wait for everyone.
	 */
	while (raw_atomic_read(&mce_callin) != num_online_cpus()) {
		if (mce_timed_out(&timeout,
				  "Timeout: Not all CPUs entered broadcast exception handler")) {
			raw_atomic_set(&global_nwo, 0);
			goto out;
		}
		ndelay(SPINUNIT);
	}

	/*
	 * mce_callin should be read before global_nwo
	 */
	smp_rmb();

	if (order == 1) {
		/*
		 * Monarch: Starts executing now, the others wait.
		 */
		raw_atomic_set(&mce_executing, 1);
	} else {
		/*
		 * Subject: Now start the scanning loop one by one in
		 * the original callin order.
		 * This way when there are any shared banks it will be
		 * only seen by one CPU before cleared, avoiding duplicates.
		 */
		while (raw_atomic_read(&mce_executing) < order) {
			if (mce_timed_out(&timeout,
					  "Timeout: Subject CPUs unable to finish machine check processing")) {
				raw_atomic_set(&global_nwo, 0);
				goto out;
			}
			ndelay(SPINUNIT);
		}
	}

	/*
	 * Cache the global no_way_out state.
	 */
	*no_way_out = raw_atomic_read(&global_nwo);

	ret = order;

out:
	instrumentation_end();

	return ret;
}
/* …（同文件无关代码省略）… */
static noinstr int mce_end(int order)
{
	u64 timeout = (u64)mca_cfg.monarch_timeout * NSEC_PER_USEC;
	int ret = -1;

	/* Allow instrumentation around external facilities. */
	instrumentation_begin();

	if (!timeout)
		goto reset;
	if (order < 0)
		goto reset;

	/*
	 * Allow others to run.
	 */
	atomic_inc(&mce_executing);

	if (order == 1) {
		/*
		 * Monarch: Wait for everyone to go through their scanning
		 * loops.
		 */
		while (atomic_read(&mce_executing) <= num_online_cpus()) {
			if (mce_timed_out(&timeout,
					  "Timeout: Monarch CPU unable to finish machine check processing"))
				goto reset;
			ndelay(SPINUNIT);
		}

		mce_reign();
		barrier();
		ret = 0;
	} else {
		/*
		 * Subject: Wait for Monarch to finish.
		 */
		while (atomic_read(&mce_executing) != 0) {
			if (mce_timed_out(&timeout,
					  "Timeout: Monarch CPU did not finish machine check processing"))
				goto reset;
			ndelay(SPINUNIT);
		}

		/*
		 * Don't reset anything. That's done by the Monarch.
		 */
		ret = 0;
		goto out;
	}

	/*
	 * Reset all global state.
	 */
reset:
	atomic_set(&global_nwo, 0);
	atomic_set(&mce_callin, 0);
	cpumask_setall(&mce_missing_cpus);
	barrier();

	/*
	 * Let others run again.
	 */
	atomic_set(&mce_executing, 0);

out:
	instrumentation_end();

	return ret;
}
/* …（同文件无关代码省略）… */
static __always_inline void mce_clear_state(unsigned long *toclear)
{
	int i;

	for (i = 0; i < this_cpu_read(mce_num_banks); i++) {
		if (arch_test_bit(i, toclear))
			mce_wrmsrq(mca_msr_reg(i, MCA_STATUS), 0);
	}
}
/* …（同文件无关代码省略）… */
static noinstr bool mce_check_crashing_cpu(void)
{
	unsigned int cpu = smp_processor_id();

	if (arch_cpu_is_offline(cpu) ||
	    (crashing_cpu != -1 && crashing_cpu != cpu)) {
		u64 mcgstatus;

		mcgstatus = native_rdmsrq(MSR_IA32_MCG_STATUS);

		if (boot_cpu_data.x86_vendor == X86_VENDOR_ZHAOXIN) {
			if (mcgstatus & MCG_STATUS_LMCES)
				return false;
		}

		if (mcgstatus & MCG_STATUS_RIPV) {
			native_wrmsrq(MSR_IA32_MCG_STATUS, 0);
			return true;
		}
	}
	return false;
}
/* …（同文件无关代码省略）… */
static __always_inline int
__mc_scan_banks(struct mce_hw_err *err, struct pt_regs *regs,
		struct mce_hw_err *final, unsigned long *toclear,
		unsigned long *valid_banks, int no_way_out, int *worst)
{
	struct mce_bank *mce_banks = this_cpu_ptr(mce_banks_array);
	struct mca_config *cfg = &mca_cfg;
	int severity, i, taint = 0;
	struct mce *m = &err->m;

	for (i = 0; i < this_cpu_read(mce_num_banks); i++) {
		arch___clear_bit(i, toclear);
		if (!arch_test_bit(i, valid_banks))
			continue;

		if (!mce_banks[i].ctl)
			continue;

		m->misc = 0;
		m->addr = 0;
		m->bank = i;

		m->status = mce_rdmsrq(mca_msr_reg(i, MCA_STATUS));
		if (!(m->status & MCI_STATUS_VAL))
			continue;

		/*
		 * Corrected or non-signaled errors are handled by
		 * machine_check_poll(). Leave them alone, unless this panics.
		 */
		if (!(m->status & (cfg->ser ? MCI_STATUS_S : MCI_STATUS_UC)) &&
			!no_way_out)
			continue;

		/* Set taint even when machine check was not enabled. */
		taint++;

		severity = mce_severity(m, regs, NULL, true);

		/*
		 * When machine check was for corrected/deferred handler don't
		 * touch, unless we're panicking.
		 */
		if ((severity == MCE_KEEP_SEVERITY ||
		     severity == MCE_UCNA_SEVERITY) && !no_way_out)
			continue;

		arch___set_bit(i, toclear);

		/* Machine check event was not enabled. Clear, but ignore. */
		if (severity == MCE_NO_SEVERITY)
			continue;

		mce_read_aux(err, i);

		/* assuming valid severity level != 0 */
		m->severity = severity;

		/*
		 * Enable instrumentation around the mce_log() call which is
		 * done in #MC context, where instrumentation is disabled.
		 */
		instrumentation_begin();
		mce_log(err);
		instrumentation_end();

		if (severity > *worst) {
			*final = *err;
			*worst = severity;
		}
	}

	/* mce_clear_state will clear *final, save locally for use later */
	*err = *final;

	return taint;
}
/* …（同文件无关代码省略）… */
static void kill_me_now(struct callback_head *ch)
{
	struct task_struct *p = container_of(ch, struct task_struct, mce_kill_me);

	p->mce_count = 0;
	force_sig(SIGBUS);
}
/* …（同文件无关代码省略）… */
static void kill_me_maybe(struct callback_head *cb)
{
	struct task_struct *p = container_of(cb, struct task_struct, mce_kill_me);
	int flags = MF_ACTION_REQUIRED;
	unsigned long pfn;
	int ret;

	p->mce_count = 0;
	pr_err("Uncorrected hardware memory error in user-access at %llx", p->mce_addr);

	if (!p->mce_ripv)
		flags |= MF_MUST_KILL;

	pfn = (p->mce_addr & MCI_ADDR_PHYSADDR) >> PAGE_SHIFT;
	ret = memory_failure(pfn, flags);
	if (!ret) {
		set_mce_nospec(pfn);
		sync_core();
		return;
	}

	/*
	 * -EHWPOISON from memory_failure() means that it already sent SIGBUS
	 * to the current process with the proper error info,
	 * -EOPNOTSUPP means hwpoison_filter() filtered the error event,
	 *
	 * In both cases, no further processing is required.
	 */
	if (ret == -EHWPOISON || ret == -EOPNOTSUPP)
		return;

	pr_err("Memory error not recovered");
	kill_me_now(cb);
}
/* …（同文件无关代码省略）… */
static void kill_me_never(struct callback_head *cb)
{
	struct task_struct *p = container_of(cb, struct task_struct, mce_kill_me);
	unsigned long pfn;

	p->mce_count = 0;
	pr_err("Kernel accessed poison in user space at %llx\n", p->mce_addr);
	pfn = (p->mce_addr & MCI_ADDR_PHYSADDR) >> PAGE_SHIFT;
	if (!memory_failure(pfn, 0))
		set_mce_nospec(pfn);
}
/* …（同文件无关代码省略）… */
static void queue_task_work(struct mce_hw_err *err, char *msg, void (*func)(struct callback_head *))
{
	int count = ++current->mce_count;
	struct mce *m = &err->m;

	/* First call, save all the details */
	if (count == 1) {
		current->mce_addr = m->addr;
		current->mce_kflags = m->kflags;
		current->mce_ripv = !!(m->mcgstatus & MCG_STATUS_RIPV);
		current->mce_whole_page = whole_page(m);
		current->mce_kill_me.func = func;
	}

	/* Ten is likely overkill. Don't expect more than two faults before task_work() */
	if (count > 10)
		mce_panic("Too many consecutive machine checks while accessing user data",
			  err, msg);

	/* Second or later call, make sure page address matches the one from first call */
	if (count > 1 && (current->mce_addr >> PAGE_SHIFT) != (m->addr >> PAGE_SHIFT))
		mce_panic("Consecutive machine checks to different user pages", err, msg);

	/* Do not call task_work_add() more than once */
	if (count > 1)
		return;

	task_work_add(current, &current->mce_kill_me, TWA_RESUME);
}
/* …（同文件无关代码省略）… */
static noinstr void unexpected_machine_check(struct pt_regs *regs)
{
	instrumentation_begin();
	pr_err("CPU#%d: Unexpected int18 (Machine Check)\n",
	       smp_processor_id());
	instrumentation_end();
}
/* …（同文件无关代码省略）… */
noinstr void do_machine_check(struct pt_regs *regs)
{
	int worst = 0, order, no_way_out, kill_current_task, lmce, taint = 0;
	DECLARE_BITMAP(valid_banks, MAX_NR_BANKS) = { 0 };
	DECLARE_BITMAP(toclear, MAX_NR_BANKS) = { 0 };
	struct mce_hw_err *final;
	struct mce_hw_err err;
	char *msg = NULL;
	struct mce *m;

	if (unlikely(mce_flags.p5))
		return pentium_machine_check(regs);
	else if (unlikely(mce_flags.winchip))
		return winchip_machine_check(regs);
	else if (unlikely(!mca_cfg.initialized))
		return unexpected_machine_check(regs);

	if (mce_flags.skx_repmov_quirk && quirk_skylake_repmov())
		goto clear;

	/*
	 * Establish sequential order between the CPUs entering the machine
	 * check handler.
	 */
	order = -1;

	/*
	 * If no_way_out gets set, there is no safe way to recover from this
	 * MCE.
	 */
	no_way_out = 0;

	/*
	 * If kill_current_task is not set, there might be a way to recover from this
	 * error.
	 */
	kill_current_task = 0;

	/*
	 * MCEs are always local on AMD. Same is determined by MCG_STATUS_LMCES
	 * on Intel.
	 */
	lmce = 1;

	inc_irq_stat(MCE_EXCEPTION);

	mce_gather_info(&err, regs);
	m = &err.m;
	m->tsc = rdtsc();

	final = this_cpu_ptr(&hw_errs_seen);
	*final = err;

	no_way_out = mce_no_way_out(&err, &msg, valid_banks, regs);

	barrier();

	/*
	 * When no restart IP might need to kill or panic.
	 * Assume the worst for now, but if we find the
	 * severity is MCE_AR_SEVERITY we have other options.
	 */
	if (!(m->mcgstatus & MCG_STATUS_RIPV))
		kill_current_task = 1;
	/*
	 * Check if this MCE is signaled to only this logical processor,
	 * on Intel, Zhaoxin only.
	 */
	if (m->cpuvendor == X86_VENDOR_INTEL ||
	    m->cpuvendor == X86_VENDOR_ZHAOXIN)
		lmce = m->mcgstatus & MCG_STATUS_LMCES;

	/*
	 * Local machine check may already know that we have to panic.
	 * Broadcast machine check begins rendezvous in mce_start()
	 * Go through all banks in exclusion of the other CPUs. This way we
	 * don't report duplicated events on shared banks because the first one
	 * to see it will clear it.
	 */
	if (lmce) {
		if (no_way_out)
			mce_panic("Fatal local machine check", &err, msg);
	} else {
		order = mce_start(&no_way_out);
	}

	taint = __mc_scan_banks(&err, regs, final, toclear, valid_banks, no_way_out, &worst);

	if (!no_way_out)
		mce_clear_state(toclear);

	/*
	 * Do most of the synchronization with other CPUs.
	 * When there's any problem use only local no_way_out state.
	 */
	if (!lmce) {
		if (mce_end(order) < 0) {
			if (!no_way_out)
				no_way_out = worst >= MCE_PANIC_SEVERITY;

			if (no_way_out)
				mce_panic("Fatal machine check on current CPU", &err, msg);
		}
	} else {
		/*
		 * If there was a fatal machine check we should have
		 * already called mce_panic earlier in this function.
		 * Since we re-read the banks, we might have found
		 * something new. Check again to see if we found a
		 * fatal error. We call "mce_severity()" again to
		 * make sure we have the right "msg".
		 */
		if (worst >= MCE_PANIC_SEVERITY) {
			mce_severity(m, regs, &msg, true);
			mce_panic("Local fatal machine check!", &err, msg);
		}
	}

	/*
	 * Enable instrumentation around the external facilities like task_work_add()
	 * (via queue_task_work()), fixup_exception() etc. For now, that is. Fixing this
	 * properly would need a lot more involved reorganization.
	 */
	instrumentation_begin();

	if (taint)
		add_taint(TAINT_MACHINE_CHECK, LOCKDEP_NOW_UNRELIABLE);

	if (worst != MCE_AR_SEVERITY && !kill_current_task)
		goto out;

	/* Fault was in user mode and we need to take some action */
	if ((m->cs & 3) == 3) {
		/* If this triggers there is no way to recover. Die hard. */
		BUG_ON(!on_thread_stack() || !user_mode(regs));

		if (!mce_usable_address(m))
			queue_task_work(&err, msg, kill_me_now);
		else
			queue_task_work(&err, msg, kill_me_maybe);

	} else if (m->mcgstatus & MCG_STATUS_SEAM_NR) {
		/*
		 * Saved RIP on stack makes it look like the machine check
		 * was taken in the kernel on the instruction following
		 * the entry to SEAM mode. But MCG_STATUS_SEAM_NR indicates
		 * that the machine check was taken inside SEAM non-root
		 * mode.  CPU core has already marked that guest as dead.
		 * It is OK for the kernel to resume execution at the
		 * apparent point of the machine check as the fault did
		 * not occur there. Mark the page as poisoned so it won't
		 * be added to free list when the guest is terminated.
		 */
		if (mce_usable_address(m)) {
			struct page *p = pfn_to_online_page(m->addr >> PAGE_SHIFT);

			if (p)
				SetPageHWPoison(p);
		}
	} else {
		/*
		 * Handle an MCE which has happened in kernel space but from
		 * which the kernel can recover: ex_has_fault_handler() has
		 * already verified that the rIP at which the error happened is
		 * a rIP from which the kernel can recover (by jumping to
		 * recovery code specified in _ASM_EXTABLE_FAULT()) and the
		 * corresponding exception handler which would do that is the
		 * proper one.
		 */
		if (m->kflags & MCE_IN_KERNEL_RECOV) {
			if (!fixup_exception(regs, X86_TRAP_MC, 0, 0))
				mce_panic("Failed kernel mode recovery", &err, msg);
		}

		if (m->kflags & MCE_IN_KERNEL_COPYIN)
			queue_task_work(&err, msg, kill_me_never);
	}

out:
	/* Given it didn't panic, mark it as recoverable */
	hwerr_log_error_type(HWERR_RECOV_OTHERS);

	instrumentation_end();

clear:
	mce_wrmsrq(MSR_IA32_MCG_STATUS, 0);
}
/* …（同文件无关代码省略）… */
int memory_failure(unsigned long pfn, int flags)
{
	/* mce_severity() should not hand us an ACTION_REQUIRED error */
	BUG_ON(flags & MF_ACTION_REQUIRED);
	pr_err("Uncorrected memory error in page 0x%lx ignored\n"
	       "Rebuild kernel with CONFIG_MEMORY_FAILURE=y for smarter handling\n",
	       pfn);

	return 0;
}
/* …（同文件无关代码省略）… */
static __always_inline void exc_machine_check_kernel(struct pt_regs *regs)
{
	irqentry_state_t irq_state;

	WARN_ON_ONCE(user_mode(regs));

/* …（同文件无关代码省略）… */
	 * mce_check_crashing_cpu() for details.
	 */
	if (mca_cfg.initialized && mce_check_crashing_cpu())
		return;

	irq_state = irqentry_nmi_enter(regs);

	do_machine_check(regs);

	irqentry_nmi_exit(regs, irq_state);
}

static __always_inline void exc_machine_check_user(struct pt_regs *regs)
{
	irqentry_enter_from_user_mode(regs);

	do_machine_check(regs);

	irqentry_exit_to_user_mode(regs);
}
/* …（同文件无关代码省略）… */
/* MCE hit kernel mode */
DEFINE_IDTENTRY_MCE(exc_machine_check)
{
	unsigned long dr7;

	dr7 = local_db_save();
	exc_machine_check_kernel(regs);
	local_db_restore(dr7);
}

/* The user mode variant. */
DEFINE_IDTENTRY_MCE_USER(exc_machine_check)
{
	unsigned long dr7;

	dr7 = local_db_save();
	exc_machine_check_user(regs);
	local_db_restore(dr7);
}

#ifdef CONFIG_X86_FRED
/* …（同文件无关代码省略）… */
 */
DEFINE_FREDENTRY_MCE(exc_machine_check)
{
	unsigned long dr7;

	dr7 = local_db_save();
	if (user_mode(regs))
		exc_machine_check_user(regs);
	else
		exc_machine_check_kernel(regs);
	local_db_restore(dr7);
}
#endif
#else
/* 32bit unified entry point */
DEFINE_IDTENTRY_RAW(exc_machine_check)
{
	unsigned long dr7;

	dr7 = local_db_save();
	if (user_mode(regs))
		exc_machine_check_user(regs);
	else
		exc_machine_check_kernel(regs);
	local_db_restore(dr7);
}
#endif
