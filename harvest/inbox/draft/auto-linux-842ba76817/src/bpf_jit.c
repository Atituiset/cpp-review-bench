// AUTO-DRAFT from torvalds/linux PR #8f150ccedfbd610aa25509ff42365d70fb20478f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#define LOONGARCH_MAX_REG_ARGS 8
/* …（同文件无关代码省略）… */
#define LOONGARCH_LONG_JUMP_NINSNS 5
/* …（同文件无关代码省略）… */
#define LOONGARCH_LONG_JUMP_NBYTES (LOONGARCH_LONG_JUMP_NINSNS * 4)
/* …（同文件无关代码省略）… */
#define LOONGARCH_FENTRY_NINSNS 2
#define LOONGARCH_FENTRY_NBYTES (LOONGARCH_FENTRY_NINSNS * 4)
#define LOONGARCH_BPF_FENTRY_NBYTES (LOONGARCH_LONG_JUMP_NINSNS * 4)
/* …（同文件无关代码省略）… */
#define REG_TCC		LOONGARCH_GPR_A6
/* …（同文件无关代码省略）… */
static void emit_store_stack_imm64(struct jit_ctx *ctx, int reg, int stack_off, u64 imm64)
{
	move_imm(ctx, reg, imm64, false);
	emit_insn(ctx, std, reg, LOONGARCH_GPR_FP, stack_off);
}
/* …（同文件无关代码省略）… */
static int emit_jump_and_link(struct jit_ctx *ctx, u8 rd, u64 target)
{
	if (!target) {
		pr_err("bpf_jit: jump target address is error\n");
		return -EFAULT;
	}

	move_imm(ctx, LOONGARCH_GPR_T1, target, false);
	emit_insn(ctx, jirl, rd, LOONGARCH_GPR_T1, 0);

	return 0;
}
/* …（同文件无关代码省略）… */
static int emit_jump_or_nops(void *target, void *ip, u32 *insns, bool is_call)
{
	int i;
	struct jit_ctx ctx;

	ctx.idx = 0;
	ctx.image = (union loongarch_instruction *)insns;

	if (!target) {
		for (i = 0; i < LOONGARCH_LONG_JUMP_NINSNS; i++)
			emit_insn((&ctx), nop);
		return 0;
	}

	return emit_jump_and_link(&ctx, is_call ? LOONGARCH_GPR_RA : LOONGARCH_GPR_ZERO, (u64)target);
}
/* …（同文件无关代码省略）… */
static int emit_call(struct jit_ctx *ctx, u64 addr)
{
	return emit_jump_and_link(ctx, LOONGARCH_GPR_RA, addr);
}
/* …（同文件无关代码省略）… */
	return ret ? ERR_PTR(-EINVAL) : dst;
}

int bpf_arch_text_poke(void *ip, enum bpf_text_poke_type old_t,
		       enum bpf_text_poke_type new_t, void *old_addr,
		       void *new_addr)
{
	int ret;
	bool is_call;
	unsigned long size = 0;
	unsigned long offset = 0;
	void *image = NULL;
	char namebuf[KSYM_NAME_LEN];
	u32 old_insns[LOONGARCH_LONG_JUMP_NINSNS] = {[0 ... 4] = INSN_NOP};
	u32 new_insns[LOONGARCH_LONG_JUMP_NINSNS] = {[0 ... 4] = INSN_NOP};

	/* Only poking bpf text is supported. Since kernel function entry
	 * is set up by ftrace, we rely on ftrace to poke kernel functions.
	 */
	if (!bpf_address_lookup((unsigned long)ip, &size, &offset, namebuf))
		return -ENOTSUPP;

	image = ip - offset;

	/* zero offset means we're poking bpf prog entry */
	if (offset == 0) {
		/* skip to the nop instruction in bpf prog entry:
		 * move t0, ra
		 * nop
		 */
		ip = image + LOONGARCH_INSN_SIZE;
	}

	is_call = old_t == BPF_MOD_CALL;
	ret = emit_jump_or_nops(old_addr, ip, old_insns, is_call);
	if (ret)
		return ret;

	if (memcmp(ip, old_insns, LOONGARCH_LONG_JUMP_NBYTES))
		return -EFAULT;

	is_call = new_t == BPF_MOD_CALL;
	ret = emit_jump_or_nops(new_addr, ip, new_insns, is_call);
	if (ret)
		return ret;

	cpus_read_lock();
	mutex_lock(&text_mutex);
	if (memcmp(ip, new_insns, LOONGARCH_LONG_JUMP_NBYTES))
		ret = larch_insn_text_copy(ip, new_insns, LOONGARCH_LONG_JUMP_NBYTES);
	mutex_unlock(&text_mutex);
	cpus_read_unlock();

	return ret;
}
/* …（同文件无关代码省略）… */
static void store_args(struct jit_ctx *ctx, int nr_arg_slots, int args_off)
{
	int i;

	for (i = 0; i < nr_arg_slots; i++) {
		if (i < LOONGARCH_MAX_REG_ARGS)
			emit_insn(ctx, std, LOONGARCH_GPR_A0 + i, LOONGARCH_GPR_FP, -args_off);
		else {
			/* Skip slots for T0 and FP of traced function */
			emit_insn(ctx, ldd, LOONGARCH_GPR_T1, LOONGARCH_GPR_FP,
				  16 + (i - LOONGARCH_MAX_REG_ARGS) * 8);
			emit_insn(ctx, std, LOONGARCH_GPR_T1, LOONGARCH_GPR_FP, -args_off);
		}
		args_off -= 8;
	}
}
/* …（同文件无关代码省略）… */
static void restore_args(struct jit_ctx *ctx, int nr_reg_args, int args_off)
{
	int i;

	for (i = 0; i < nr_reg_args; i++) {
		emit_insn(ctx, ldd, LOONGARCH_GPR_A0 + i, LOONGARCH_GPR_FP, -args_off);
		args_off -= 8;
	}
}
/* …（同文件无关代码省略）… */
static void restore_stk_args(struct jit_ctx *ctx, int nr_stk_args, int args_off, int stk_args_off)
{
	int i;

	for (i = 0; i < nr_stk_args; i++) {
		emit_insn(ctx, ldd, LOONGARCH_GPR_T1, LOONGARCH_GPR_FP,
			  -(args_off - LOONGARCH_MAX_REG_ARGS * 8));
		emit_insn(ctx, std, LOONGARCH_GPR_T1, LOONGARCH_GPR_FP, -stk_args_off);
		args_off -= 8;
		stk_args_off -= 8;
	}
}

static int invoke_bpf_prog(struct jit_ctx *ctx, struct bpf_tramp_node *n,
			   int args_off, int retval_off, int run_ctx_off, bool save_ret)
{
	int ret;
	u32 *branch;
	struct bpf_prog *p = n->link->prog;
	int cookie_off = offsetof(struct bpf_tramp_run_ctx, bpf_cookie);

	if (n->cookie)
		emit_store_stack_imm64(ctx, LOONGARCH_GPR_T1,
/* …（同文件无关代码省略）… */
	/* arg3: &run_ctx */
	emit_insn(ctx, addid, LOONGARCH_GPR_A2, LOONGARCH_GPR_FP, -run_ctx_off);
	ret = emit_call(ctx, (const u64)bpf_trampoline_exit(p));

	return ret;
}

static int invoke_bpf(struct jit_ctx *ctx, struct bpf_tramp_nodes *tn,
		      int args_off, int retval_off, int run_ctx_off,
		      int func_meta_off, bool save_ret, u64 func_meta, int cookie_off)
{
	int i, cur_cookie = (cookie_off - args_off) / 8;

/* …（同文件无关代码省略）… */
			emit_store_stack_imm64(ctx, LOONGARCH_GPR_T1, -func_meta_off, meta);
			cur_cookie--;
		}
		err = invoke_bpf_prog(ctx, tn->nodes[i], args_off, retval_off, run_ctx_off, save_ret);
		if (err)
			return err;
	}
/* …（同文件无关代码省略）… */
static void sign_extend(struct jit_ctx *ctx, int rd, int rj, u8 size, bool sign)
{
	/* ABI requires unsigned char/short to be zero-extended */
	if (!sign && (size == 1 || size == 2)) {
		if (rd != rj)
			move_reg(ctx, rd, rj);
		return;
	}

	switch (size) {
	case 1:
		emit_insn(ctx, extwb, rd, rj);
		break;
	case 2:
		emit_insn(ctx, extwh, rd, rj);
		break;
	case 4:
		emit_insn(ctx, addiw, rd, rj, 0);
		break;
	case 8:
		if (rd != rj)
			move_reg(ctx, rd, rj);
		break;
	default:
		pr_warn("bpf_jit: invalid size %d for sign_extend\n", size);
	}
}
/* …（同文件无关代码省略）… */
static int __arch_prepare_bpf_trampoline(struct jit_ctx *ctx, struct bpf_tramp_image *im,
					 const struct btf_func_model *m, struct bpf_tramp_nodes *tnodes,
					 void *func_addr,
