// AUTO-DRAFT from torvalds/linux PR #79eb6679ef6a0d5ea68da53a8321fb1dc72299b9
	return insn.word;
}

u32 larch_insn_gen_jirl(enum loongarch_gpr rd, enum loongarch_gpr rj, int imm)
{
	union loongarch_instruction insn;
