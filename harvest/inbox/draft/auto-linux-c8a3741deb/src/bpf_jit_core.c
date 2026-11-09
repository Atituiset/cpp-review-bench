// AUTO-DRAFT from torvalds/linux PR #e0d8e8de8bcb1ef7b749033b5af5518ba8df7bec
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
struct jit_buffer {
	u8	*buf;
	u32	len;
	u32	index;
};
/* …（同文件无关代码省略）… */
struct arc_jit_data {
	struct bpf_binary_header *bpf_header;
	u32                      *bpf2insn;
};
/* …（同文件无关代码省略）… */
struct jit_context {
	struct bpf_prog			*prog;
	struct jit_buffer		jit;
	struct bpf_binary_header	*bpf_header;
	bool				emit;
	bool				do_zext;
	u32				*bpf2insn;
	bool				bpf2insn_valid;
	struct arc_jit_data		*jit_data;
	u32				arc_regs_clobbered;
	bool				save_blink;
	u16				frame_size;
	u32				epilogue_offset;
	bool				need_extra_pass;
	bool				is_extra_pass;
	bool				user_bpf_prog;
	bool				success;
};
/* …（同文件无关代码省略）… */
/* Piece of memory that can be allocated at the beginning of jit_prepare(). */
static int jit_prepare_early_mem_alloc(struct jit_context *ctx)
{
	ctx->bpf2insn = kcalloc(ctx->prog->len, sizeof(ctx->jit.len),
				GFP_KERNEL);

	if (!ctx->bpf2insn) {
		pr_err("bpf-jit: could not allocate memory for "
