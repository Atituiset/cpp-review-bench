// AUTO-DRAFT from torvalds/linux PR #a940b03cee1524c10c16e0f73ec878bbd36202a2
static void fpsimd_sve_flush(void)
{
	*host_data_ptr(fp_owner) = FP_STATE_HOST_OWNED;
}
/* …（同文件无关代码省略）… */
static void __copy_vcpu_state(const struct kvm_vcpu *from_vcpu,
			      struct kvm_vcpu *to_vcpu)
{
	int i;
  // <<< BUG ANCHOR
	to_vcpu->arch.ctxt.regs		= from_vcpu->arch.ctxt.regs;
	to_vcpu->arch.ctxt.spsr_abt	= from_vcpu->arch.ctxt.spsr_abt;
	to_vcpu->arch.ctxt.spsr_und	= from_vcpu->arch.ctxt.spsr_und;
	to_vcpu->arch.ctxt.spsr_irq	= from_vcpu->arch.ctxt.spsr_irq;
	to_vcpu->arch.ctxt.spsr_fiq	= from_vcpu->arch.ctxt.spsr_fiq;
	to_vcpu->arch.ctxt.fp_regs	= from_vcpu->arch.ctxt.fp_regs;

	/*
	 * Copy the sysregs, but don't mess with the timer state which
	 * is directly handled by EL1 and is expected to be preserved.
	 * enum vcpu_sysreg is sparse: VNCR-mapped registers take values
	 * derived from their VNCR page offset, so the timer registers do
	 * not form a contiguous numeric range and must be skipped by name.
	 */
	for (i = 1; i < NR_SYS_REGS; i++) {
		switch (i) {
		case CNTVOFF_EL2:
		case CNTV_CVAL_EL0:
		case CNTV_CTL_EL0:
		case CNTP_CVAL_EL0:
		case CNTP_CTL_EL0:
			continue;
		}
		to_vcpu->arch.ctxt.sys_regs[i] = from_vcpu->arch.ctxt.sys_regs[i];
	}
}
/* …（同文件无关代码省略）… */
static void flush_hyp_vcpu_state(struct pkvm_hyp_vcpu *hyp_vcpu)
{
	__copy_vcpu_state(hyp_vcpu->host_vcpu, &hyp_vcpu->vcpu);
}
/* …（同文件无关代码省略）… */
static void flush_debug_state(struct pkvm_hyp_vcpu *hyp_vcpu)
{
	struct kvm_vcpu *host_vcpu = hyp_vcpu->host_vcpu;

	hyp_vcpu->vcpu.arch.debug_owner = host_vcpu->arch.debug_owner;

	if (kvm_guest_owns_debug_regs(&hyp_vcpu->vcpu)) {
		hyp_vcpu->vcpu.arch.vcpu_debug_state = host_vcpu->arch.vcpu_debug_state;
	} else if (kvm_host_owns_debug_regs(&hyp_vcpu->vcpu)) {
		hyp_vcpu->vcpu.arch.external_debug_state = host_vcpu->arch.external_debug_state;
		/*
		 * The world switch loads MDSCR_EL1 from external_mdscr_el1
		 * (ctxt_mdscr_el1()).
		 */
		hyp_vcpu->vcpu.arch.external_mdscr_el1 = host_vcpu->arch.external_mdscr_el1;
	}
}
/* …（同文件无关代码省略）… */
static void flush_hyp_vcpu(struct pkvm_hyp_vcpu *hyp_vcpu)
{
	struct kvm_vcpu *host_vcpu = hyp_vcpu->host_vcpu;

	fpsimd_sve_flush();
	flush_debug_state(hyp_vcpu);
/* …（同文件无关代码省略）… */
	if (!pkvm_hyp_vcpu_is_protected(hyp_vcpu)) {
		if (vcpu_get_flag(host_vcpu, PKVM_HOST_STATE_DIRTY))
			flush_hyp_vcpu_state(hyp_vcpu);
	} else {
		hyp_vcpu->vcpu.arch.ctxt = host_vcpu->arch.ctxt;
	}
/* …（同文件无关代码省略）… */
	 * trap-control bit, so it must flow to the hyp vCPU alongside TWI/TWE
	 * for the vSError to be delivered. sync_hyp_vcpu() reflects it back.
	 */
	hyp_vcpu->vcpu.arch.hcr_el2 &= ~(HCR_TWI | HCR_TWE | HCR_VSE);
	hyp_vcpu->vcpu.arch.hcr_el2 |= READ_ONCE(host_vcpu->arch.hcr_el2) &
						 (HCR_TWI | HCR_TWE | HCR_VSE);

	hyp_vcpu->vcpu.arch.iflags	= host_vcpu->arch.iflags;
