// AUTO-DRAFT from torvalds/linux PR #dd384658c738d15783a786d3af2851e02490f171
#define KVM_ARM_VCPU_PTRAUTH_GENERIC	6 /* VCPU uses generic authentication */
#define KVM_ARM_VCPU_HAS_EL2		7 /* Support nested virtualization */
#define KVM_ARM_VCPU_HAS_EL2_E2H0	8 /* Limit NV support to E2H RES0 */

struct kvm_vcpu_init {
	__u32 target;
