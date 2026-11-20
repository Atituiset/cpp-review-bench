// AUTO-DRAFT from torvalds/linux PR #d40cb8a12ed25789f39404c5f7968a65e98d89c3
#define IS_NULL_REG(reg) ((reg)->space_id ==  ACPI_ADR_SPACE_SYSTEM_MEMORY && \
				(reg)->address == 0 &&			\
				(reg)->bit_width == 0 &&		\
				(reg)->bit_offset == 0 &&		\
				(reg)->access_width == 0)
/* …（同文件无关代码省略）… */
#define CPC_SUPPORTED(cpc) ((cpc)->type == ACPI_TYPE_INTEGER ?		\
				!!(cpc)->cpc_entry.int_value :		\
				!IS_NULL_REG(&(cpc)->cpc_entry.reg))
/* …（同文件无关代码省略）… */
static unsigned int cpc_reg_access_width(const struct cpc_reg *reg)
{
	if (reg->access_width > 4)
		return 0;
  // <<< BUG ANCHOR
	if (reg->access_width)
		return 8U << (reg->access_width - 1);

	return reg->bit_width;
}
/* …（同文件无关代码省略）… */
static void cpc_apply_platform_quirks(struct cpc_reg *reg,
				      unsigned int reg_idx, u32 quirks)
{
	unsigned int access_width;

	if (!(quirks & CPC_QUIRK_PERF_LIMITED_OWNS_UNIT) ||
	    reg_idx != PERF_LIMITED ||
	    reg->space_id != ACPI_ADR_SPACE_SYSTEM_MEMORY ||
	    reg->bit_width != 2 || reg->bit_offset)
		return;

	access_width = cpc_reg_access_width(reg);
	if (access_width != 32)
		return;

	reg->bit_width = access_width;
	pr_info_once("firmware quirk: Performance Limited owns its access unit, using Bit Width %u\n",
		     access_width);
}
/* …（同文件无关代码省略）… */
			cpc_apply_platform_quirks(gas_t, i - 2,
						  platform_quirks);

			/*
			 * The PCC Subspace index is encoded inside
			 * the CPC table entries. The same PCC index
/* …（同文件无关代码省略）… */
	if (!high || !low || !nom || !ref || !min_nonlinear ||
	    high > U32_MAX || low > U32_MAX || guaranteed > U32_MAX ||
	    nom > U32_MAX || ref > U32_MAX || min_nonlinear > U32_MAX ||
	    high < nom || nom < min_nonlinear || min_nonlinear < low ||
	    (CPC_SUPPORTED(guaranteed_reg) &&
	     (guaranteed < low || guaranteed > nom))) {
		ret = -EFAULT;
		goto out_err;
	}
