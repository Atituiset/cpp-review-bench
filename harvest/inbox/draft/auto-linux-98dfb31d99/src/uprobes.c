// AUTO-DRAFT from torvalds/linux PR #84afebd853437e44cb71f718be11d90e4c1a59dd
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdlib.h>

		return regs->sp < ret->stack;
}

unsigned long
arch_uretprobe_hijack_return_addr(unsigned long trampoline_vaddr,
				  struct pt_regs *regs)
{
	unsigned long orig_ret_vaddr;
	unsigned long gcs_ret_vaddr;
	int err = 0;
	u64 gcspr;

	orig_ret_vaddr = procedure_link_pointer(regs);

	if (task_gcs_el0_enabled(current)) {
		gcspr = read_sysreg_s(SYS_GCSPR_EL0);
		gcs_ret_vaddr = get_user_gcs((__force unsigned long __user *)gcspr, &err);
		if (err) {
			force_sig(SIGSEGV);
			goto out;
		}

		/*
		 * If the LR and GCS return addr don't match, then some kind of PAC
		 * signing or control flow occurred since entering the probed function.
		 * Likely because the user is attempting to retprobe on an instruction
		 * that isn't a function boundary or inside a leaf function. Explicitly
		 * abort this retprobe because it will generate a GCS exception.
		 */
		if (gcs_ret_vaddr != orig_ret_vaddr) {
			orig_ret_vaddr = -1;
			goto out;
		}

		put_user_gcs(trampoline_vaddr, (__force unsigned long __user *)gcspr, &err);
		if (err) {
			force_sig(SIGSEGV);
			goto out;
		}
	}

	/* Replace the return addr with trampoline addr */
	procedure_link_pointer_set(regs, trampoline_vaddr);

out:
	return orig_ret_vaddr;
}
