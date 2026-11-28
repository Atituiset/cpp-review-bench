// AUTO-DRAFT from torvalds/linux PR #d3a5ea8a82fabeb2314700c2e001130faf99df1f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <linux/filter.h>
#include <linux/memory.h>
#include <linux/printk.h>
#include <linux/slab.h>

#include <asm/asm-extable.h>
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
		 * 12 registers are on the stack
		 */
		emit(A64_SUB_I(1, A64_SP, A64_FP, 96), ctx);
		/* The callback may use its own BPF stack, set up fp for it. */
		ctx->fp_used = true;
	}
