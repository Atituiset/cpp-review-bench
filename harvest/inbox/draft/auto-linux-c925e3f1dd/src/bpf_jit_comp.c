// AUTO-DRAFT from torvalds/linux PR #a4da531b171c664a6cf42c5b82d1610cbdf14fe4
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
static u8 *emit_code(u8 *ptr, u32 bytes, unsigned int len)
{
	if (len == 1)
		*ptr = bytes;
	else if (len == 2)
		*(u16 *)ptr = bytes;
	else {
		*(u32 *)ptr = bytes;
		barrier();
	}
	return ptr + len;
}
/* …（同文件无关代码省略）… */
#define EMIT(bytes, len) \
	do { prog = emit_code(prog, bytes, len); } while (0)
/* …（同文件无关代码省略）… */
#define EMIT1(b1)		EMIT(b1, 1)
#define EMIT2(b1, b2)		EMIT((b1) + ((b2) << 8), 2)
#define EMIT3(b1, b2, b3)	EMIT((b1) + ((b2) << 8) + ((b3) << 16), 3)
#define EMIT4(b1, b2, b3, b4)   EMIT((b1) + ((b2) << 8) + ((b3) << 16) + ((b4) << 24), 4)
/* …（同文件无关代码省略）… */
#define EMIT1_off32(b1, off) \
	do { EMIT1(b1); EMIT(off, 4); } while (0)
/* …（同文件无关代码省略）… */
#define EMIT3_off32(b1, b2, b3, off) \
	do { EMIT3(b1, b2, b3); EMIT(off, 4); } while (0)
/* …（同文件无关代码省略）… */
static bool is_imm8(int value)
{
	return value <= 127 && value >= -128;
}
/* …（同文件无关代码省略）… */
#define EMIT_mov(DST, SRC)								 \
	do {										 \
		if (DST != SRC)								 \
			EMIT3(add_2mod(0x48, DST, SRC), 0x89, add_2reg(0xC0, DST, SRC)); \
	} while (0)
/* …（同文件无关代码省略）… */
#define AUX_REG (MAX_BPF_JIT_REG + 1)
#define X86_REG_R9 (MAX_BPF_JIT_REG + 2)
#define X86_REG_R12 (MAX_BPF_JIT_REG + 3)
/* …（同文件无关代码省略）… */
static bool is_ereg(u32 reg)
{
	return (1 << reg) & (BIT(BPF_REG_5) |
			     BIT(AUX_REG) |
			     BIT(BPF_REG_7) |
			     BIT(BPF_REG_8) |
			     BIT(BPF_REG_9) |
			     BIT(X86_REG_R9) |
			     BIT(X86_REG_R12) |
			     BIT(BPF_REG_AX));
}
/* …（同文件无关代码省略）… */
static bool is_ereg_8l(u32 reg)
{
	return is_ereg(reg) ||
	    (1 << reg) & (BIT(BPF_REG_1) |
			  BIT(BPF_REG_2) |
			  BIT(BPF_REG_FP));
}
/* …（同文件无关代码省略）… */
static u8 add_2mod(u8 byte, u32 r1, u32 r2)
{
	if (is_ereg(r1))
		byte |= 1;
	if (is_ereg(r2))
		byte |= 4;
	return byte;
}
/* …（同文件无关代码省略）… */
static u8 add_2reg(u8 byte, u32 dst_reg, u32 src_reg)
{
	return byte + reg2hex[dst_reg] + (reg2hex[src_reg] << 3);
}
/* …（同文件无关代码省略）… */
static void push_r9(u8 **pprog)
{
	u8 *prog = *pprog;

	EMIT2(0x41, 0x51);   /* push r9 */
	*pprog = prog;
}
/* …（同文件无关代码省略）… */
static void emit_sub_rsp(u8 **pprog, u32 depth)
{
	u8 *prog = *pprog;

	if (!depth)
		return;
	if (is_imm8(depth))
		EMIT4(0x48, 0x83, 0xEC, depth); /* sub rsp, imm8 */
	else
		EMIT3_off32(0x48, 0x81, 0xEC, depth); /* sub rsp, imm32 */
	*pprog = prog;
}
/* …（同文件无关代码省略）… */
static void emit_mov_reg(u8 **pprog, bool is64, u32 dst_reg, u32 src_reg)
{
	u8 *prog = *pprog;

	if (is64) {
		/* mov dst, src */
		EMIT_mov(dst_reg, src_reg);
	} else {
		/* mov32 dst, src */
		if (is_ereg(dst_reg) || is_ereg(src_reg))
			EMIT1(add_2mod(0x40, dst_reg, src_reg));
		EMIT2(0x89, add_2reg(0xC0, dst_reg, src_reg));
	}

	*pprog = prog;
}
/* …（同文件无关代码省略）… */
static bool base_needs_disp(u32 reg)
{
	return reg2hex[reg] == 5;
}
/* …（同文件无关代码省略）… */
static void emit_insn_suffix(u8 **pprog, u32 ptr_reg, u32 val_reg, int off)
{
	u8 *prog = *pprog;

	if (!off && !base_needs_disp(ptr_reg)) {
		/* No displacement */
		EMIT1(add_2reg(0x00, ptr_reg, val_reg));
	} else if (is_imm8(off)) {
		/* 1-byte signed displacement */
		EMIT2(add_2reg(0x40, ptr_reg, val_reg), off);
	} else {
		/* 4-byte signed displacement */
		EMIT1_off32(add_2reg(0x80, ptr_reg, val_reg), off);
	}
	*pprog = prog;
}
/* …（同文件无关代码省略）… */
static void maybe_emit_mod(u8 **pprog, u32 dst_reg, u32 src_reg, bool is64)
{
	u8 *prog = *pprog;

	if (is64)
		EMIT1(add_2mod(0x48, dst_reg, src_reg));
	else if (is_ereg(dst_reg) || is_ereg(src_reg))
		EMIT1(add_2mod(0x40, dst_reg, src_reg));
	*pprog = prog;
}
/* …（同文件无关代码省略）… */
static void emit_ldx(u8 **pprog, u32 size, u32 dst_reg, u32 src_reg, int off)
{
	u8 *prog = *pprog;

	switch (size) {
	case BPF_B:
		/* Emit 'movzx eax, byte ptr [rax + off]' */
		if (is_ereg(dst_reg) || is_ereg(src_reg))
			EMIT1(add_2mod(0x40, src_reg, dst_reg));
		EMIT2(0x0F, 0xB6);
		break;
	case BPF_H:
		/* Emit 'movzx eax, word ptr [rax + off]' */
		if (is_ereg(dst_reg) || is_ereg(src_reg))
			EMIT1(add_2mod(0x40, src_reg, dst_reg));
		EMIT2(0x0F, 0xB7);
		break;
	case BPF_W:
		/* Emit 'mov eax, dword ptr [rax+0x14]' */
		if (is_ereg(dst_reg) || is_ereg(src_reg))
			EMIT2(add_2mod(0x40, src_reg, dst_reg), 0x8B);
		else
			EMIT1(0x8B);
		break;
	case BPF_DW:
		/* Emit 'mov rax, qword ptr [rax+0x14]' */
		EMIT2(add_2mod(0x48, src_reg, dst_reg), 0x8B);
		break;
	}
	emit_insn_suffix(&prog, src_reg, dst_reg, off);
	*pprog = prog;
}
/* …（同文件无关代码省略）… */
static void emit_stx(u8 **pprog, u32 size, u32 dst_reg, u32 src_reg, int off)
{
	u8 *prog = *pprog;

	switch (size) {
	case BPF_B:
		/* Emit 'mov byte ptr [rax + off], al' */
		if (is_ereg(dst_reg) || is_ereg_8l(src_reg))
			/* Add extra byte for eregs or SIL,DIL,BPL in src_reg */
			EMIT2(add_2mod(0x40, dst_reg, src_reg), 0x88);
		else
			EMIT1(0x88);
		break;
	case BPF_H:
		if (is_ereg(dst_reg) || is_ereg(src_reg))
			EMIT3(0x66, add_2mod(0x40, dst_reg, src_reg), 0x89);
		else
			EMIT2(0x66, 0x89);
		break;
	case BPF_W:
		if (is_ereg(dst_reg) || is_ereg(src_reg))
			EMIT2(add_2mod(0x40, dst_reg, src_reg), 0x89);
		else
			EMIT1(0x89);
		break;
	case BPF_DW:
		EMIT2(add_2mod(0x48, dst_reg, src_reg), 0x89);
		break;
	}
	emit_insn_suffix(&prog, dst_reg, src_reg, off);
	*pprog = prog;
}
/* …（同文件无关代码省略）… */
static int emit_kfunc_arg_moves(const struct btf_func_model *fm, s32 stack_base, u8 **pprog)
{
	struct bpf_jit_arg_move moves[BPF_JIT_MAX_ARG_MOVES];
	const u8 nreg = x86_arg_abi.nr_arg_regs;
	u8 *prog = *pprog, *start = prog;
	u32 i, n;

	n = bpf_jit_plan_arg_moves(&x86_arg_abi, fm, moves);

	for (i = 0; i < n; i++) {
		u8 dst = moves[i].dst, src = moves[i].src, reg;
		bool dst_mem = dst != BPF_JIT_ARG_TMP && dst >= nreg;
		bool src_mem = src != BPF_JIT_ARG_TMP && src >= nreg;

		/*
		 * Take the value into a register: the one it belongs in, the
		 * scratch when it is carried past its own destination, and
		 * BPF_REG_AX only to pass one stack slot to another.
		 */
		if (src == BPF_JIT_ARG_TMP) {
			reg = AUX_REG;
		} else if (src_mem) {
			reg = dst == BPF_JIT_ARG_TMP ? AUX_REG :
			      dst_mem ? BPF_REG_AX : x86_arg_reg[dst];
			emit_ldx(&prog, BPF_DW, reg, BPF_REG_FP,
				 stack_base + (src - nreg) * 8);
		} else {
			reg = x86_arg_reg[src];
		}

		/* And leave it where the argument belongs. */
		if (dst == BPF_JIT_ARG_TMP)
			emit_mov_reg(&prog, true, AUX_REG, reg);
		else if (dst_mem)
			emit_stx(&prog, BPF_DW, BPF_REG_FP, reg,
				 stack_base + (dst - nreg) * 8);
		else if (reg != x86_arg_reg[dst])
			emit_mov_reg(&prog, true, x86_arg_reg[dst], reg);
	}

	*pprog = prog;
	return prog - start;
}
/* …（同文件无关代码省略）… */
	 * Arg 6 goes into r9 register, not on stack.
	 */
	outgoing_rsp = out_stack_arg_cnt > 1 ? (out_stack_arg_cnt - 1) * 8 : 0;
	if (bpf_prog->aux->exception_boundary)
		bpf_prog->aux->stack_arg_sp_adjust = outgoing_rsp;
	emit_sub_rsp(&prog, outgoing_rsp);

	if (arena_vm_start)
/* …（同文件无关代码省略）… */
				ip += emit_kfunc_arg_moves(fm, outgoing_arg_base -
							   outgoing_rsp, &prog);
			}
			if (func == (u8 *)arch_bpf_timed_may_goto) {
				u32 fp = priv_frame_ptr ? X86_REG_R9 : BPF_REG_FP;

				/*
				 * AX has the offset of count and timestamp
				 * in the stack. Turn it into a pointer.
				 * add r10, rbp or add r10, r9
				 */
				maybe_emit_mod(&prog, BPF_REG_AX, fp, true);
				EMIT2(0x01, add_2reg(0xC0, BPF_REG_AX, fp));
				ip += 3;
			}
			if (priv_frame_ptr) {
				push_r9(&prog);
				ip += 2;
