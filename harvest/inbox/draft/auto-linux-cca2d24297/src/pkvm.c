// AUTO-DRAFT from torvalds/linux PR #eff8d2791c086388ba5bae36385afd9bc6f0507e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <string.h>
  // <<< BUG ANCHOR
		if (test_bit(KVM_ARCH_FLAG_WRITABLE_IMP_ID_REGS, &host_arch_flags))
			hyp_vm->kvm.arch.midr_el1 = host_kvm->arch.midr_el1;

		return;
	}

	if (kvm_pkvm_ext_allowed(kvm, KVM_CAP_ARM_MTE))
/* …（同文件无关代码省略）… */
	if (kvm_pkvm_ext_allowed(kvm, KVM_CAP_ARM_PTRAUTH_GENERIC))
		set_bit(KVM_ARM_VCPU_PTRAUTH_GENERIC, allowed_features);

	if (kvm_pkvm_ext_allowed(kvm, KVM_CAP_ARM_SVE)) {
		set_bit(KVM_ARM_VCPU_SVE, allowed_features);
		kvm->arch.flags |= host_arch_flags & BIT(KVM_ARCH_FLAG_GUEST_HAS_SVE);
	}

	bitmap_and(kvm->arch.vcpu_features, host_kvm->arch.vcpu_features,
		   allowed_features, KVM_VCPU_MAX_FEATURES);
}

static void unpin_host_vcpu(struct kvm_vcpu *host_vcpu)
/* …（同文件无关代码省略）… */
{
	void *sve_state;

	if (!vcpu_has_feature(&hyp_vcpu->vcpu, KVM_ARM_VCPU_SVE))
		return;

	sve_state = hyp_vcpu->vcpu.arch.sve_state;
	hyp_unpin_shared_mem(sve_state,
			     sve_state + vcpu_sve_state_size(&hyp_vcpu->vcpu));
}
/* …（同文件无关代码省略）… */
	unsigned int sve_max_vl;
	size_t sve_state_size;
	void *sve_state;
	int ret = 0;

	if (!vcpu_has_feature(vcpu, KVM_ARM_VCPU_SVE)) {
		vcpu_clear_flag(vcpu, VCPU_SVE_FINALIZED);
/* …（同文件无关代码省略）… */

	/* Limit guest vector length to the maximum supported by the host. */
	sve_max_vl = min(READ_ONCE(host_vcpu->arch.sve_max_vl), kvm_host_sve_max_vl);
	sve_state_size = sve_state_size_from_vl(sve_max_vl);
	sve_state = kern_hyp_va(READ_ONCE(host_vcpu->arch.sve_state));

	if (!sve_state || !sve_state_size) {
		ret = -EINVAL;
		goto err;
	}

	ret = hyp_pin_shared_mem(sve_state, sve_state + sve_state_size);
	if (ret)
		goto err;

	vcpu->arch.sve_state = sve_state;
	vcpu->arch.sve_max_vl = sve_max_vl;

	return 0;
err:
	clear_bit(KVM_ARM_VCPU_SVE, vcpu->kvm->arch.vcpu_features);
	return ret;
}

static int vm_copy_id_regs(struct pkvm_hyp_vcpu *hyp_vcpu)
{
	struct pkvm_hyp_vm *hyp_vm = pkvm_hyp_vcpu_to_hyp_vm(hyp_vcpu);
	const struct kvm *host_kvm = hyp_vm->host_kvm;
	struct kvm *kvm = &hyp_vm->kvm;

	if (!test_bit(KVM_ARCH_FLAG_ID_REGS_INITIALIZED, &host_kvm->arch.flags))
		return -EINVAL;

	if (test_and_set_bit(KVM_ARCH_FLAG_ID_REGS_INITIALIZED, &kvm->arch.flags))
		return 0;

	memcpy(kvm->arch.id_regs, host_kvm->arch.id_regs, sizeof(kvm->arch.id_regs));

	return 0;
}
