// AUTO-DRAFT from torvalds/linux PR #8f150ccedfbd610aa25509ff42365d70fb20478f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
}

static int invoke_bpf_prog(u32 *image, u32 *ro_image, struct codegen_context *ctx,
			   struct bpf_tramp_node *n, int regs_off, int retval_off,
			   int run_ctx_off, bool save_ret)
{
	struct bpf_prog *p = n->link->prog;
	ppc_inst_t branch_insn;
	u32 jmp_idx;
	int ret = 0;

	/* Save cookie */
	if (IS_ENABLED(CONFIG_PPC64)) {
		PPC_LI64(_R3, n->cookie);
/* …（同文件无关代码省略）… */
	EMIT(PPC_RAW_ADDI(_R5, _R1, run_ctx_off));
	ret = bpf_jit_emit_func_call_rel(image, ro_image, ctx,
					 (unsigned long)bpf_trampoline_exit(p));

	return ret;
}

static int invoke_bpf_mod_ret(u32 *image, u32 *ro_image, struct codegen_context *ctx,
			      struct bpf_tramp_nodes *tn, int regs_off, int retval_off,
			      int run_ctx_off, u32 *branches)
{
	int i;

/* …（同文件无关代码省略）… */
	EMIT(PPC_RAW_LI(_R3, 0));
	EMIT(PPC_RAW_STL(_R3, _R1, retval_off));
	for (i = 0; i < tn->nr_nodes; i++) {
		if (invoke_bpf_prog(image, ro_image, ctx, tn->nodes[i], regs_off, retval_off,
				    run_ctx_off, true))
			return -EINVAL;

		/*
/* …（同文件无关代码省略）… */
static void bpf_trampoline_setup_tail_call_info(u32 *image, struct codegen_context *ctx,
						int bpf_frame_size, int r4_off)
{
	if (IS_ENABLED(CONFIG_PPC64)) {
		EMIT(PPC_RAW_LD(_R4, _R1, bpf_frame_size));
		/* Refer to trampoline's Generated stack layout */
		EMIT(PPC_RAW_LD(_R3, _R4, -BPF_PPC_TAILCALL));

		/*
		 * Setting the tail_call_info in trampoline's frame
		 * depending on if previous frame had value or reference.
		 */
		EMIT(PPC_RAW_CMPLWI(_R3, MAX_TAIL_CALL_CNT));
		PPC_BCC_CONST_SHORT(COND_GT, 8);
		EMIT(PPC_RAW_ADDI(_R3, _R4, -BPF_PPC_TAILCALL));

		/*
		 * Trampoline's tail_call_info is at the same offset, as that of
		 * any bpf program, with reference to previous frame. Update the
		 * address of main's tail_call_info in trampoline frame.
		 */
		EMIT(PPC_RAW_STL(_R3, _R1, bpf_frame_size - BPF_PPC_TAILCALL));
	} else {
		/* See bpf_jit_stack_offsetof() and BPF_PPC_TC */
		EMIT(PPC_RAW_LL(_R4, _R1, r4_off));
	}
}
/* …（同文件无关代码省略）… */
static void bpf_trampoline_restore_tail_call_cnt(u32 *image, struct codegen_context *ctx,
						 int bpf_frame_size, int r4_off)
{
	if (IS_ENABLED(CONFIG_PPC32)) {
		/*
		 * Restore tailcall for 32-bit powerpc
		 * See bpf_jit_stack_offsetof() and BPF_PPC_TC
		 */
		EMIT(PPC_RAW_STL(_R4, _R1, r4_off));
	}
}
/* …（同文件无关代码省略）… */
static void bpf_trampoline_save_args(u32 *image, struct codegen_context *ctx,
				     int bpf_frame_size, int nr_regs, int regs_off)
{
	int param_save_area_offset;

	param_save_area_offset = bpf_frame_size;
	param_save_area_offset += STACK_FRAME_MIN_SIZE; /* param save area is past frame header */

	for (int i = 0; i < nr_regs; i++) {
		if (i < 8) {
			EMIT(PPC_RAW_STL(_R3 + i, _R1, regs_off + i * SZL));
		} else {
			EMIT(PPC_RAW_LL(_R3, _R1, param_save_area_offset + i * SZL));
			EMIT(PPC_RAW_STL(_R3, _R1, regs_off + i * SZL));
		}
	}
}
/* …（同文件无关代码省略）… */
static void bpf_trampoline_restore_args_regs(u32 *image, struct codegen_context *ctx,
					     int nr_regs, int regs_off)
{
	for (int i = 0; i < nr_regs && i < 8; i++)
		EMIT(PPC_RAW_LL(_R3 + i, _R1, regs_off + i * SZL));
}
/* …（同文件无关代码省略）… */
static void bpf_trampoline_restore_args_stack(u32 *image, struct codegen_context *ctx,
					      int bpf_frame_size, int nr_regs, int regs_off)
{
	int param_save_area_offset;

	param_save_area_offset = bpf_frame_size;
	param_save_area_offset += STACK_FRAME_MIN_SIZE; /* param save area is past frame header */

	for (int i = 8; i < nr_regs; i++) {
		EMIT(PPC_RAW_LL(_R3, _R1, param_save_area_offset + i * SZL));
		EMIT(PPC_RAW_STL(_R3, _R1, STACK_FRAME_MIN_SIZE + i * SZL));
	}
	bpf_trampoline_restore_args_regs(image, ctx, nr_regs, regs_off);
}
/* …（同文件无关代码省略）… */
static int __arch_prepare_bpf_trampoline(struct bpf_tramp_image *im, void *rw_image,
					 void *rw_image_end, void *ro_image,
					 const struct btf_func_model *m, u32 flags,
					 struct bpf_tramp_nodes *tnodes,
					 void *func_addr)
{
	int regs_off, func_meta_off, ip_off, run_ctx_off, retval_off;
	int nvr_off, alt_lr_off, r4_off = 0;
	struct bpf_tramp_nodes *fmod_ret = &tnodes[BPF_TRAMP_MODIFY_RETURN];
	struct bpf_tramp_nodes *fentry = &tnodes[BPF_TRAMP_FENTRY];
	struct bpf_tramp_nodes *fexit = &tnodes[BPF_TRAMP_FEXIT];
	int i, ret, nr_regs, retaddr_off, bpf_frame_size = 0;
	struct codegen_context codegen_ctx, *ctx;
	int cookie_off, cookie_cnt, cookie_ctx_off;
	int fsession_cnt = bpf_fsession_cnt(tnodes);
	u64 func_meta;
	u32 *image = (u32 *)rw_image;
	ppc_inst_t branch_insn;
	u32 *branches = NULL;
	bool save_ret;

	if (IS_ENABLED(CONFIG_PPC32))
		return -EOPNOTSUPP;

	nr_regs = m->nr_args;
	/* Extra registers for struct arguments */
	for (i = 0; i < m->nr_args; i++)
		if (m->arg_size[i] > SZL)
			nr_regs += round_up(m->arg_size[i], SZL) / SZL - 1;

	if (nr_regs > MAX_BPF_FUNC_ARGS)
		return -EOPNOTSUPP;

	ctx = &codegen_ctx;
	memset(ctx, 0, sizeof(*ctx));

	/*
	 * Generated stack layout:
	 *
	 * func prev back chain         [ back chain        ]
	 *                              [ tail_call_info    ] optional - 64-bit powerpc
	 *                              [ padding           ] align stack frame
	 *       r4_off                 [ r4 (tailcallcnt)  ] optional - 32-bit powerpc
	 *       alt_lr_off             [ real lr (ool stub)] optional - actual lr
	 *       retaddr_off            [ return address    ]
	 *                              [ r26               ]
	 *       nvr_off                [ r25               ] nvr save area
	 *       retval_off             [ return value      ]
	 *                              [ reg argN          ]
	 *                              [ ...               ]
	 *       regs_off               [ reg_arg1          ] prog_ctx
	 *       func_meta_off          [ args count        ] ((u64 *)prog_ctx)[-1]
	 *       ip_off                 [ traced function   ] ((u64 *)prog_ctx)[-2]
	 *                              [ stack c
