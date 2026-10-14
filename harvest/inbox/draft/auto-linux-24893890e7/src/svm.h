// AUTO-DRAFT from torvalds/linux PR #03741bcea0a9ec4c8f386fd5c6051c1a14070240
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdint.h>

struct kvm_vmcb_info {
	struct vmcb *ptr;
	unsigned long pa;
	int cpu;
	uint64_t asid_generation;
};
/* …（同文件无关代码省略）… */
struct vmcb_save_area_cached {
	struct vmcb_seg es;
	struct vmcb_seg cs;
	struct vmcb_seg ss;
	struct vmcb_seg ds;
	struct vmcb_seg gdtr;
	struct vmcb_seg idtr;
	u8 cpl;
	u64 efer;
	u64 cr4;
	u64 cr3;
	u64 cr0;
	u64 dr7;
	u64 dr6;
	u64 rflags;
	u64 rip;
	u64 rsp;
	u64 s_cet;
	u64 ssp;
	u64 isst_addr;
	u64 rax;
	u64 cr2;
	u64 g_pat;
	u64 dbgctl;
	u64 br_from;
	u64 br_to;
	u64 last_excp_from;
	u64 last_excp_to;
};
/* …（同文件无关代码省略）… */
struct vmcb_ctrl_area_cached {
	u32 intercepts[MAX_INTERCEPT];
	u16 pause_filter_thresh;
	u16 pause_filter_count;
	u64 iopm_base_pa;
	u64 msrpm_base_pa;
	u64 tsc_offset;
	u32 asid;
	u8 tlb_ctl;
	u8 erap_ctl;
	u32 int_ctl;
	u32 int_vector;
	u32 int_state;
	u64 exit_code;
	u64 exit_info_1;
	u64 exit_info_2;
	u32 exit_int_info;
	u32 exit_int_info_err;
	u64 misc_ctl;
	u32 event_inj;
	u32 event_inj_err;
	u64 next_rip;
	u64 nested_cr3;
	u64 misc_ctl2;
	u32 clean;
	union {
#if IS_ENABLED(CONFIG_HYPERV) || IS_ENABLED(CONFIG_KVM_HYPERV)
		struct hv_vmcb_enlightenments hv_enlightenments;
#endif
		u8 reserved_sw[32];
	};
};
/* …（同文件无关代码省略）… */
struct svm_nested_state {
	struct kvm_vmcb_info vmcb02;
	u64 hsave_msr;
	u64 vm_cr_msr;
	u64 vmcb12_gpa;
	u64 last_vmcb12_gpa;
	u64 last_bus_lock_rip;

	/*
	 * The MSR permissions map used for vmcb02, which is the merge result
	 * of vmcb01 and vmcb12
	 */
	void *msrpm;

	/* cache for control fields of the guest */
	struct vmcb_ctrl_area_cached ctl;

	/*
	 * Note: this struct is not kept up-to-date while L2 runs; it is only
	 * valid within nested_svm_vmrun.
	 */
	struct vmcb_save_area_cached save;

	bool initialized;

	/*
	 * Indicates whether MSR bitmap for L2 needs to be rebuilt due to
	 * changes in MSR bitmap for L1 or switching to a different L2. Note,
	 * this flag can only be used reliably in conjunction with a paravirt L1
	 * which informs L0 whether any changes to MSR bitmap for L2 were done
	 * on its side.
	 */
	bool force_msr_bitmap_recalc;
};
/* …（同文件无关代码省略）… */
struct vcpu_sev_es_state {
	/* SEV-ES support */
	struct sev_es_save_area *vmsa;
	struct ghcb *ghcb;
	u8 valid_bitmap[16];
	struct kvm_host_map ghcb_map;
	bool received_first_sipi;
	unsigned int ap_reset_hold_type;

	/* SEV-ES scratch area support */
	u64 sw_scratch;
	void *ghcb_sa;
	u32 ghcb_sa_len;
	bool ghcb_sa_sync;
	bool ghcb_sa_free;

	/* SNP Page-State-Change buffer entries currently being processed */
	struct {
		u16 cur_idx;
		u16 end_idx;
		u16 batch_size;
		bool is_2m;
	} psc;

	u64 ghcb_registered_gpa;

	struct mutex snp_vmsa_mutex; /* Used to handle concurrent updates of VMSA. */
	gpa_t snp_pending_vmsa_gpa;
	gpa_t snp_guest_vmsa_gpa;
	bool snp_ap_waiting_for_reset;
	bool snp_has_guest_vmsa;
};
/* …（同文件无关代码省略）… */
struct vcpu_svm {
	struct kvm_vcpu vcpu;
	/* vmcb always points at current_vmcb->ptr, it's purely a shorthand. */
	struct vmcb *vmcb;
	struct kvm_vmcb_info vmcb01;
	struct kvm_vmcb_info *current_vmcb;
	u32 asid;
	u32 sysenter_esp_hi;
	u32 sysenter_eip_hi;
	uint64_t tsc_aux;

	u64 msr_decfg;

	u64 next_rip;

	u64 spec_ctrl;

	u64 tsc_ratio_msr;
	/*
	 * Contains guest-controlled bits of VIRT_SPEC_CTRL, which will be
	 * translated into the appropriate L2_CFG bits on the host to
	 * perform speculative control.
	 */
	u64 virt_spec_ctrl;

	void *msrpm;

	ulong nmi_iret_rip;

	struct svm_nested_state nested;

	/* NMI mask value, used when vNMI is not enabled */
	bool nmi_masked;

	/*
	 * True when NMIs are still masked but guest IRET was just intercepted
	 * and KVM is waiting for RIP to change, which will signal that the
	 * intercepted IRET was retired and thus NMI can be unmasked.
	 */
	bool awaiting_iret_completion;

	/*
	 * Set when KVM is awaiting IRET completion and needs to inject NMIs as
	 * soon as the IRET completes (e.g. NMI is pending injection).  KVM
	 * temporarily steals RFLAGS.TF to single-step the guest in this case
	 * in order to regain control as soon as the NMI-blocking condition
	 * goes away.
	 */
	bool nmi_singlestep;
	u64 nmi_singlestep_guest_rflags;

	bool nmi_l1_to_l2;

	unsigned long soft_int_csbase;
	unsigned long soft_int_old_rip;
	unsigned long soft_int_next_rip;
	bool soft_int_injected;

	u32 ldr_reg;
	u32 dfr_reg;

	/* This is essentially a shadow of the vCPU's actual entry in the
	 * Physical ID table that is programmed into the VMCB, i.e. that is
	 * seen by the CPU.  If IPI virtualization is disabled, IsRunning is
	 * only ever set in the shadow, i.e. is never propagated to the "real"
	 * table, so that hardware never sees IsRunning=1.
	 */
	u64 avic_physical_id_entry;

	/*
	 * Per-vCPU list of irqfds that are eligible to post IRQs directly to
	 * the vCPU (a.k.a. device posted IRQs, a.k.a. IRQ bypass).  The list
	 * is used to reconfigure IRTEs when the vCPU is loaded/put (to set the
	 * target pCPU), when AVIC is toggled on/off (to (de)activate bypass),
	 * and if the irqfd becomes ineligible for posting (to put the IRTE
	 * back into remapped mode).
	 */
	struct list_head ir_list;
	raw_spinlock_t ir_list_lock;

	struct vcpu_sev_es_state sev_es;

	bool guest_state_loaded;

	bool avic_irq_window;
	bool x2avic_msrs_intercepted;
	bool lbr_msrs_intercepted;

	/* Guest GIF value, used when vGIF is not enabled */
	bool guest_gif;
};
/* …（同文件无关代码省略）… */
int avic_incomplete_ipi_interception(struct kvm_vcpu *vcpu);
int avic_unaccelerated_access_interception(struct kvm_vcpu *vcpu);
int avic_init_vcpu(struct vcpu_svm *svm);
void avic_vcpu_load(struct kvm_vcpu *vcpu, int cpu);
void avic_vcpu_put(struct kvm_vcpu *vcpu);
void avic_apicv_post_state_restore(struct kvm_vcpu *vcpu);
