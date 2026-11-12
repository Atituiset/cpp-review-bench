// AUTO-DRAFT from torvalds/linux PR #79eb6679ef6a0d5ea68da53a8321fb1dc72299b9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

union loongarch_instruction {
	unsigned int word;
	struct reg0i15_format	reg0i15_format;
	struct reg0i26_format	reg0i26_format;
	struct reg1i20_format	reg1i20_format;
	struct reg1i21_format	reg1i21_format;
	struct reg2_format	reg2_format;
	struct reg2i5_format	reg2i5_format;
	struct reg2i6_format	reg2i6_format;
	struct reg2i12_format	reg2i12_format;
	struct reg2i14_format	reg2i14_format;
	struct reg2i16_format	reg2i16_format;
	struct reg2bstrd_format	reg2bstrd_format;
	struct reg2csr_format   reg2csr_format;
	struct reg3_format	reg3_format;
	struct reg3sa2_format	reg3sa2_format;
};
/* …（同文件无关代码省略）… */
enum loongarch_gpr {
	LOONGARCH_GPR_ZERO = 0,
	LOONGARCH_GPR_RA = 1,
	LOONGARCH_GPR_TP = 2,
	LOONGARCH_GPR_SP = 3,
	LOONGARCH_GPR_A0 = 4,	/* Reused as V0 for return value */
	LOONGARCH_GPR_A1,	/* Reused as V1 for return value */
	LOONGARCH_GPR_A2,
	LOONGARCH_GPR_A3,
	LOONGARCH_GPR_A4,
	LOONGARCH_GPR_A5,
	LOONGARCH_GPR_A6,
	LOONGARCH_GPR_A7,
	LOONGARCH_GPR_T0 = 12,
	LOONGARCH_GPR_T1,
	LOONGARCH_GPR_T2,
	LOONGARCH_GPR_T3,
	LOONGARCH_GPR_T4,
	LOONGARCH_GPR_T5,
	LOONGARCH_GPR_T6,
	LOONGARCH_GPR_T7,
	LOONGARCH_GPR_T8,
	LOONGARCH_GPR_U0 = 21,	/* Kernel per-CPU base register ($r21) */
	LOONGARCH_GPR_FP = 22,
	LOONGARCH_GPR_S0 = 23,
	LOONGARCH_GPR_S1,
	LOONGARCH_GPR_S2,
	LOONGARCH_GPR_S3,
	LOONGARCH_GPR_S4,
	LOONGARCH_GPR_S5,
	LOONGARCH_GPR_S6,
	LOONGARCH_GPR_S7,
	LOONGARCH_GPR_S8,
	LOONGARCH_GPR_MAX
};
/* …（同文件无关代码省略）… */
u32 larch_insn_gen_lu52id(enum loongarch_gpr rd, enum loongarch_gpr rj, int imm);
u32 larch_insn_gen_beq(enum loongarch_gpr rd, enum loongarch_gpr rj, int imm);
u32 larch_insn_gen_bne(enum loongarch_gpr rd, enum loongarch_gpr rj, int imm);
u32 larch_insn_gen_jirl(enum loongarch_gpr rd, enum loongarch_gpr rj, int imm);

static inline bool signed_imm_check(long val, unsigned int bit)
{
	return -(1L << (bit - 1)) <= val && val < (1L << (bit - 1));
}
/* …（同文件无关代码省略）… */
#define DEF_EMIT_REG1I20_FORMAT(NAME, OP)				\
static inline void emit_##NAME(union loongarch_instruction *insn,	\
			       enum loongarch_gpr rd, int imm)		\
{									\
	insn->reg1i20_format.opcode = OP;				\
	insn->reg1i20_format.immediate = imm;				\
	insn->reg1i20_format.rd = rd;					\
}
/* …（同文件无关代码省略）… */
DEF_EMIT_REG1I20_FORMAT(lu32id, lu32id_op)
DEF_EMIT_REG1I20_FORMAT(pcaddu18i, pcaddu18i_op)

#define DEF_EMIT_REG2_FORMAT(NAME, OP)					\
static inline void emit_##NAME(union loongarch_instruction *insn,	\
			       enum loongarch_gpr rd,			\
