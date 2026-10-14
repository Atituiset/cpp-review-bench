// AUTO-DRAFT from torvalds/linux PR #03741bcea0a9ec4c8f386fd5c6051c1a14070240
static inline void kvm_mmu_refresh_passthrough_bits(struct kvm_vcpu *vcpu,
						    struct kvm_pagewalk *w)
{
	/*
	 * When EPT is enabled, KVM may passthrough CR0.WP to the guest, i.e.
	 * @w's snapshot of CR0.WP and thus all related paging metadata may
	 * be stale.  Refresh CR0.WP and the metadata on-demand when checking
	 * for permission faults.  Exempt nested MMUs, i.e. MMUs for shadowing
	 * nEPT and nNPT, as CR0.WP is ignored in both cases.  Note, KVM will
	 * still refresh gva_walk, so as to honor L2's CR0.WP when translating
	 * L2 GVAs to GPAs.
	 */
	if (!tdp_enabled || w == &vcpu->arch.ngpa_walk)
		return;
  // <<< BUG ANCHOR
	__kvm_mmu_refresh_passthrough_bits(vcpu, w);
}
/* …（同文件无关代码省略）… */
				  unsigned pte_access, unsigned pte_pkey,
				  u64 access)
{
	/* strip nested paging fault error codes */
	unsigned int pfec = access;
	unsigned long rflags = kvm_x86_call(get_rflags)(vcpu);

	/*
/* …（同文件无关代码省略）… */
	kvm_mmu_refresh_passthrough_bits(vcpu, w);

	fault = (fmt->permissions[index] >> pte_access) & 1;

	WARN_ON_ONCE(pfec & (PFERR_PK_MASK | PFERR_SS_MASK | PFERR_RSVD_MASK));
	if (unlikely(fmt->pkru_mask)) {
		u32 pkru_bits, offset;
