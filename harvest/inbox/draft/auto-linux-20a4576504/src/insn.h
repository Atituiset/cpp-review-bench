// AUTO-DRAFT from torvalds/linux PR #32c52ec87d6245ee2d12ed5dd509e40062939b50
struct insn_field {
	union {
		insn_value_t value;
		insn_byte_t bytes[4];
	};
	/* !0 if we've run insn_get_xxx() for this field */
	unsigned char got;
	unsigned char nbytes;
};
/* …（同文件无关代码省略）… */
struct insn {
	struct insn_field prefixes;	/*
					 * Prefixes
					 * prefixes.bytes[3]: last prefix
					 */
	struct insn_field rex_prefix;	/* REX prefix */
	union {
		struct insn_field vex_prefix;	/* VEX prefix */
		struct insn_field xop_prefix;	/* XOP prefix */
	};
	struct insn_field opcode;	/*
					 * opcode.bytes[0]: opcode1
					 * opcode.bytes[1]: opcode2
					 * opcode.bytes[2]: opcode3
					 */
	struct insn_field modrm;
	struct insn_field sib;
	struct insn_field displacement;
	union {
		struct insn_field immediate;
		struct insn_field moffset1;	/* for 64bit MOV */
		struct insn_field immediate1;	/* for 64bit imm or off16/32 */
	};
	union {
		struct insn_field moffset2;	/* for 64bit MOV */
		struct insn_field immediate2;	/* for 64bit imm or seg16 */
	};
  // <<< BUG ANCHOR
	int	emulate_prefix_size;
	insn_attr_t attr;
	unsigned char opnd_bytes;
	unsigned char addr_bytes;
	unsigned char length;
	unsigned char x86_64;

	const insn_byte_t *kaddr;	/* kernel address of insn to analyze */
	const insn_byte_t *end_kaddr;	/* kernel address of last insn in buffer */
	const insn_byte_t *next_byte;
};
/* …（同文件无关代码省略）… */
 * are repeated, it cannot be used for looping over the prefixes.
 */
#define for_each_insn_prefix(insn, prefix)	\
	for (int idx = 0; idx < ARRAY_SIZE(insn->prefixes.bytes) && (prefix = insn->prefixes.bytes[idx]) != 0; idx++)

#define POP_SS_OPCODE 0x1f
#define MOV_SREG_OPCODE 0x8e
