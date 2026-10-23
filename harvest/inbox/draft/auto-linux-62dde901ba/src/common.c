// AUTO-DRAFT from torvalds/linux PR #864c07de0c7dcbca58b61625dfa2e461ae63b242
void __no_profile native_write_cr4(unsigned long val)
{
	unsigned long bits_changed = 0;
  // <<< BUG ANCHOR
set_register:
	asm volatile("mov %0,%%cr4": "+r" (val) : : "memory");

	if (static_branch_likely(&cr_pinning)) {
		if (unlikely((val & cr4_pinned_mask) != cr4_pinned_bits)) {
			bits_changed = (val & cr4_pinned_mask) ^ cr4_pinned_bits;
			val = (val & ~cr4_pinned_mask) | cr4_pinned_bits;
			goto set_register;
		}
		/* Warn after we've corrected the changed bits. */
		WARN_ONCE(bits_changed, "pinned CR4 bits changed: 0x%lx!?\n",
			  bits_changed);
	}
}
#if IS_MODULE(CONFIG_LKDTM)
EXPORT_SYMBOL_GPL(native_write_cr4);
#endif

void cr4_update_irqsoff(unsigned long set, unsigned long clear)
{
