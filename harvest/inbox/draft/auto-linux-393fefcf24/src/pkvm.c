// AUTO-DRAFT from torvalds/linux PR #42e225323067a222230068651cf886bd7fbb1fe2
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
 */
static DEFINE_PER_CPU(struct pkvm_hyp_vcpu *, loaded_hyp_vcpu);

/*
 * The PKVM_HCR_EL2_HOST_{PVM,NPVM} bits of this value come from the host on
 * each entry, and a protected vCPU's TWI and TWE at vCPU load.
/* …（同文件无关代码省略）… */
#define RESERVED_ENTRY ((void *)0xa110ca7ed)
/* …（同文件无关代码省略）… */
{
	struct pkvm_hyp_vcpu *hyp_vcpu = NULL;
	struct pkvm_hyp_vm *hyp_vm;

	/* Cannot load a new vcpu without putting the old one first. */
	if (__this_cpu_read(loaded_hyp_vcpu))
/* …（同文件无关代码省略）… */
unlock:
	hyp_spin_unlock(&vm_table_lock);

	if (hyp_vcpu)
		__this_cpu_write(loaded_hyp_vcpu, hyp_vcpu);
	return hyp_vcpu;
}

/* …（同文件无关代码省略）… */
 */
static int allocate_vm_table_entry(void)
{
	int idx;

	hyp_assert_lock_held(&vm_table_lock);
/* …（同文件无关代码省略）… */

	vm_table[idx] = RESERVED_ENTRY;

	return idx;
}
