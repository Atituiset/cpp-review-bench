// AUTO-DRAFT from torvalds/linux PR #23a38e7bb2f9ad99c317a9f4e4c349054de9dfff
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#include <asm/insn-eval.h>

/*
 * Returns the target address and the expected type when regs->ip points
 * to a compiler-generated CFI trap.
 */
static bool decode_cfi_insn(struct pt_regs *regs, unsigned long *target,
			    u32 *type)
{
	char buffer[MAX_INSN_SIZE];
	struct insn insn;
/* …（同文件无关代码省略）… */
	 *   movl    -<id>, %r10d       ; 6 bytes
	 *   addl    -<pos>(%reg), %r10d; 4 bytes
	 *   je      .Ltmp1             ; 2 bytes
	 *   ud2                        ; <- regs->ip
	 *   .Ltmp1:
	 *
	 * We can decode the expected type and the target address from the
	 * movl/addl instructions.
	 */
	if (copy_from_kernel_nofault(buffer, (void *)regs->ip - 12, MAX_INSN_SIZE))
		return false;
	if (insn_decode_kernel(&insn, &buffer[offset]))
		return false;
/* …（同文件无关代码省略）… */

	*type = -(u32)insn.immediate.value;

	if (copy_from_kernel_nofault(buffer, (void *)regs->ip - 6, MAX_INSN_SIZE))
		return false;
	if (insn_decode_kernel(&insn, &buffer[offset]))
		return false;
/* …（同文件无关代码省略）… */
				return BUG_TRAP_TYPE_NONE;
		}

		if (!decode_cfi_insn(regs, &target, &type))
			return report_cfi_failure_noaddr(regs, addr);

		break;
