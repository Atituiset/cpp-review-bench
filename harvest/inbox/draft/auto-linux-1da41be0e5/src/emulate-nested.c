// AUTO-DRAFT from torvalds/linux PR #a940b03cee1524c10c16e0f73ec878bbd36202a2
#define TC_CGT_BITS	10
#define TC_FGT_BITS	4
#define TC_FGF_BITS	5
#define TC_SRI_BITS	10
/* …（同文件无关代码省略）… */
union trap_config {
	u64	val;
	struct {
		unsigned long	cgt:TC_CGT_BITS; /* Coarse Grained Trap id */
		unsigned long	fgt:TC_FGT_BITS; /* Fine Grained Trap id */
		unsigned long	bit:6;		 /* Bit number */
		unsigned long	pol:1;		 /* Polarity */
		unsigned long	fgf:TC_FGF_BITS; /* Fine Grained Filter */
		unsigned long	sri:TC_SRI_BITS; /* SysReg Index */
		unsigned long	unused:27;	 /* Unused, should be zero */
		unsigned long	mbz:1;		 /* Must Be Zero */
	};
};
/* …（同文件无关代码省略）… */
struct encoding_to_trap_config {
	const u32			encoding;
	const u32			end;
	const union trap_config		tc;
	const unsigned int		line;
};
/* …（同文件无关代码省略）… */
static __init void print_nv_trap_error(const struct encoding_to_trap_config *tc,
				       const char *type, int err)
{
	kvm_err("%s line %d encoding range "
		"(%d, %d, %d, %d, %d) - (%d, %d, %d, %d, %d) (err=%d)\n",
		type, tc->line,
		sys_reg_Op0(tc->encoding), sys_reg_Op1(tc->encoding),
		sys_reg_CRn(tc->encoding), sys_reg_CRm(tc->encoding),
		sys_reg_Op2(tc->encoding),
		sys_reg_Op0(tc->end), sys_reg_Op1(tc->end),
		sys_reg_CRn(tc->end), sys_reg_CRm(tc->end),
		sys_reg_Op2(tc->end),
		err);
}
/* …（同文件无关代码省略）… */
				print_nv_trap_error(fgt, "FGT bit is reserved", ret);
			}
  // <<< BUG ANCHOR
			if (!cpus_have_final_cap(ARM64_HAS_FGT))
				continue;

			prev = xa_store(&sr_forward_xa, enc,
					xa_mk_value(tc.val), GFP_KERNEL);
