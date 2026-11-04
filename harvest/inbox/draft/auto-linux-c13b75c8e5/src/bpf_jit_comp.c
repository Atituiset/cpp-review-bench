// AUTO-DRAFT from torvalds/linux PR #a4da531b171c664a6cf42c5b82d1610cbdf14fe4
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
					(jit->prg - patch_brc) >> 1;
			break;
		}
		switch (insn->off) {
		case 0: /* DST = SRC */
			/* lgr %dst,%src */
/* …（同文件无关代码省略）… */
		if ((void *)func == arch_bpf_timed_may_goto) {
			/*
			 * arch_bpf_timed_may_goto() has a special ABI: the
			 * parameters are in BPF_REG_AX and BPF_REG_10; the
			 * return value is in BPF_REG_AX; and all GPRs except
			 * REG_W0, REG_W1, and BPF_REG_AX are callee-saved.
			 */

			/* brasl %r0,func */
/* …（同文件无关代码省略）… */
	fp->jited_len = jit.size - cfi_get_offset();

	if (!fp->is_func || extra_pass) {
		for (int i = 0; i < fp->len; i++)
			jit.addrs[i] -= cfi_get_offset();
		bpf_prog_fill_jited_linfo(fp, jit.addrs + 1);
free_addrs:
		kvfree(jit.addrs);
/* …（同文件无关代码省略）… */
	return true;
}

bool bpf_jit_inlines_helper_call(s32 imm)
{
	switch (imm) {
