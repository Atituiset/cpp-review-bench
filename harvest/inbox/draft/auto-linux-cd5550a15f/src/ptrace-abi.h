// AUTO-DRAFT from torvalds/linux PR #06ac073129191a01d77933ed381f89c67674db6f
#ifndef _ASM_X86_PTRACE_ABI_H
#define _ASM_X86_PTRACE_ABI_H

#ifdef __i386__

#define EBX 0
/* …（同文件无关代码省略）… */

#else /* __i386__ */

#if defined(__ASSEMBLER__) || defined(__FRAME_OFFSETS)
/*
 * C ABI says these regs are callee-preserved. They aren't saved on kernel entry
 * unless syscall needs a complete, fully filled "struct pt_regs".
/* …（同文件无关代码省略）… */
#define EFLAGS 144
#define RSP 152
#define SS 160
#endif /* __ASSEMBLER__ */

/* top of stack page */
#define FRAME_SIZE 168

#endif /* !__i386__ */

/* Arbitrarily choose the same ptrace numbers as used by the Sparc code. */
#define PTRACE_GETREGS            12
