// AUTO-DRAFT from torvalds/linux PR #8f150ccedfbd610aa25509ff42365d70fb20478f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define TMP_REG_1 (MAX_BPF_JIT_REG + 0)
/* …（同文件无关代码省略）… */
struct jit_ctx {
	const struct bpf_prog *prog;
	int idx;
	int epilogue_offset;
	int *offset;
	int exentry_idx;
	int nr_used_callee_reg;
	u8 used_callee_reg[8]; /* r6~r9, fp, arena_vm_start */
	__le32 *image;
	__le32 *ro_image;
	u32 stack_size;
	u16 stack_arg_size;
	u64 user_vm_start;
	u64 arena_vm_start;
	bool fp_used;
	bool priv_sp_used;
	bool write;
};
/* …（同文件无关代码省略）… */
static inline void emit(const u32 insn, struct jit_ctx *ctx)
{
	if (ctx->image != NULL && ctx->write)
		ctx->image[ctx->idx] = cpu_to_le32(insn);

	ctx->idx++;
}
/* …（同文件无关代码省略）… */
static inline void emit_a64_mov_i(const int is64, const int reg,
				  const s32 val, struct jit_ctx *ctx)
{
	u16 hi = val >> 16;
	u16 lo = val & 0xffff;

	if (hi & 0x8000) {
		if (hi == 0xffff) {
			emit(A64_MOVN(is64, reg, (u16)~lo, 0), ctx);
		} else {
			emit(A64_MOVN(is64, reg, (u16)~hi, 16), ctx);
			if (lo != 0xffff)
				emit(A64_MOVK(is64, reg, lo, 0), ctx);
		}
	} else {
		emit(A64_MOVZ(is64, reg, lo, 0), ctx);
		if (hi)
			emit(A64_MOVK(is64, reg, hi, 16), ctx);
	}
}
/* …（同文件无关代码省略）… */
static int i64_i16_blocks(const u64 val, bool inverse)
{
	return (((val >>  0) & 0xffff) != (inverse ? 0xffff : 0x0000)) +
	       (((val >> 16) & 0xffff) != (inverse ? 0xffff : 0x0000)) +
	       (((val >> 32) & 0xffff) != (inverse ? 0xffff : 0x0000)) +
	       (((val >> 48) & 0xffff) != (inverse ? 0xffff : 0x0000));
}
/* …（同文件无关代码省略）… */
static inline void emit_a64_mov_i64(const int reg, const u64 val,
				    struct jit_ctx *ctx)
{
	u64 nrm_tmp = val, rev_tmp = ~val;
	bool inverse;
	int shift;

	if (!(nrm_tmp >> 32))
		return emit_a64_mov_i(0, reg, (u32)val, ctx);

	inverse = i64_i16_blocks(nrm_tmp, true) < i64_i16_blocks(nrm_tmp, false);
	shift = max(round_down((inverse ? (fls64(rev_tmp) - 1) :
					  (fls64(nrm_tmp) - 1)), 16), 0);
	if (inverse)
		emit(A64_MOVN(1, reg, (rev_tmp >> shift) & 0xffff, shift), ctx);
	else
		emit(A64_MOVZ(1, reg, (nrm_tmp >> shift) & 0xffff, shift), ctx);
	shift -= 16;
	while (shift >= 0) {
		if (((nrm_tmp >> shift) & 0xffff) != (inverse ? 0xffff : 0x0000))
			emit(A64_MOVK(1, reg, (nrm_tmp >> shift) & 0xffff, shift), ctx);
		shift -= 16;
	}
}
/* …（同文件无关代码省略）… */
static inline void emit_addr_mov_i64(const int reg, const u64 val,
				     struct jit_ctx *ctx)
{
	u64 tmp = val;
	int shift = 0;

	emit(A64_MOVN(1, reg, ~tmp & 0xffff, shift), ctx);
	while (shift < 32) {
		tmp >>= 16;
		shift += 16;
		emit(A64_MOVK(1, reg, tmp & 0xffff, shift), ctx);
	}
}
/* …（同文件无关代码省略）… */
static bool should_emit_indirect_call(long target, const struct jit_ctx *ctx)
{
	long offset;

	/* when ctx->ro_image is not allocated or the target is unknown,
	 * emit indirect call
	 */
	if (!ctx->ro_image || !target)
		return true;

	offset = target - (long)&ctx->ro_image[ctx->idx];
	return offset < -SZ_128M || offset >= SZ_128M;
}
/* …（同文件无关代码省略）… */
static void emit_direct_call(u64 target, struct jit_ctx *ctx)
{
	u32 insn;
	unsigned long pc;

	pc = (unsigned long)&ctx->ro_image[ctx->idx];
	insn = aarch64_insn_gen_branch_imm(pc, target, AARCH64_INSN_BRANCH_LINK);
	emit(insn, ctx);
}
/* …（同文件无关代码省略）… */
static void emit_indirect_call(u64 target, struct jit_ctx *ctx)
{
	u8 tmp;

	tmp = bpf2a64[TMP_REG_1];
	emit_addr_mov_i64(tmp, target, ctx);
	emit(A64_BLR(tmp), ctx);
}
/* …（同文件无关代码省略）… */
static void emit_call(u64 target, struct jit_ctx *ctx)
{
	if (should_emit_indirect_call((long)target, ctx))
		emit_indirect_call(target, ctx);
	else
		emit_direct_call(target, ctx);
}
/* …（同文件无关代码省略）… */
	return true;
}

static void invoke_bpf_prog(struct jit_ctx *ctx, struct bpf_tramp_node *node,
			    int bargs_off, int retval_off, int run_ctx_off,
			    bool save_ret)
{
	__le32 *branch;
	u64 enter_prog;
	u64 exit_prog;
/* …（同文件无关代码省略）… */
	enter_prog = (u64)bpf_trampoline_enter(p);
	exit_prog = (u64)bpf_trampoline_exit(p);

	if (node->cookie == 0) {
		/* if cookie is zero, one instruction is enough to store it */
		emit(A64_STR64I(A64_ZR, A64_SP, run_ctx_off + cookie_off), ctx);
/* …（同文件无关代码省略）… */
	emit(A64_ADD_I(1, A64_R(2), A64_SP, run_ctx_off), ctx);

	emit_call(exit_prog, ctx);
}

static void invoke_bpf_mod_ret(struct jit_ctx *ctx, struct bpf_tramp_nodes *tn,
			       int bargs_off, int retval_off, int run_ctx_off,
			       __le32 **branches)
{
	int i;

/* …（同文件无关代码省略）… */
	 */
	emit(A64_STR64I(A64_ZR, A64_SP, retval_off), ctx);
	for (i = 0; i < tn->nr_nodes; i++) {
		invoke_bpf_prog(ctx, tn->nodes[i], bargs_off, retval_off,
				run_ctx_off, true);
		/* if (*(u64 *)(sp + retval_off) !=  0)
		 *	goto do_fexit;
/* …（同文件无关代码省略）… */
struct arg_aux {
	/* how many args are passed through registers, the rest of the args are
	 * passed through stack
	 */
	int args_in_regs;
	/* how many registers are used to pass arguments */
	int regs_for_args;
	/* how much stack is used for additional args passed to bpf program
	 * that did not fit in original function registers
	 */
	int bstack_for_args;
	/* home much stack is used for additional args passed to the
	 * original function when called from trampoline (this one needs
	 * arguments to be properly aligned)
	 */
	int ostack_for_args;
};
/* …（同文件无关代码省略）… */
static void store_func_meta(struct jit_ctx *ctx, u64 func_meta, int func_meta_off)
{
	emit_a64_mov_i64(A64_R(10), func_meta, ctx);
	emit(A64_STR64I(A64_R(10), A64_SP, func_meta_off), ctx);
}
/* …（同文件无关代码省略）… */
			store_func_meta(ctx, meta, func_meta_off);
			cookie_bargs_off--;
		}
		invoke_bpf_prog(ctx, fentry->nodes[i], bargs_off,
				retval_off, run_ctx_off,
				flags & BPF_TRAMP_F_RET_FENTRY_RET);
	}
/* …（同文件无关代码省略）… */
		if (!branches)
			return -ENOMEM;

		invoke_bpf_mod_ret(ctx, fmod_ret, bargs_off, retval_off,
				   run_ctx_off, branches);
	}

/* …（同文件无关代码省略）… */
		emit(A64_RET(A64_R(10)), ctx);
		/* store return value */
		emit(A64_STR64I(A64_R(0), A64_SP, retval_off), ctx);
		/* reserve a nop for b
