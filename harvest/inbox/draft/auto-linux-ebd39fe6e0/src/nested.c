// AUTO-DRAFT from torvalds/linux PR #eff8d2791c086388ba5bae36385afd9bc6f0507e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
struct vncr_tlb {
	/* The guest's VNCR_EL2 */
	u64			gva;
	struct s1_walk_info	wi;
	struct s1_walk_result	wr;

	u64			hpa;
	bool			hpa_writable;

	/* -1 when not mapped on a CPU */
	atomic_t		cpu;

	/*
	 * true if the TLB is valid. Can only be changed with the
	 * mmu_lock held.
	 */
	bool			valid;
};
/* …（同文件无关代码省略）… */
 */
#define S2_MMU_PER_VCPU		2

void kvm_init_nested(struct kvm *kvm)
{
	kvm->arch.nested_mmus = NULL;
	kvm->arch.nested_mmus_size = 0;
	atomic_set(&kvm->arch.vncr_tlb_count, 0);
}

static int init_nested_s2_mmu(struct kvm *kvm, struct kvm_s2_mmu *mmu)
/* …（同文件无关代码省略）… */
int kvm_vcpu_init_nested(struct kvm_vcpu *vcpu)
{
	struct kvm *kvm = vcpu->kvm;
	struct kvm_s2_mmu *tmp;
	int num_mmus, ret = 0;

	if (test_bit(KVM_ARM_VCPU_HAS_EL2_E2H0, kvm->arch.vcpu_features) &&
	    !cpus_have_final_cap(ARM64_HAS_HCR_NV1))
		return -EINVAL;

	if (!vcpu->arch.ctxt.vncr_array)
		vcpu->arch.ctxt.vncr_array = (u64 *)__get_free_page(GFP_KERNEL_ACCOUNT |
								    __GFP_ZERO);

	if (!vcpu->arch.ctxt.vncr_array)
		return -ENOMEM;

	/*
	 * Let's treat memory allocation failures as benign: If we fail to
	 * allocate anything, return an error and keep the allocated array
	 * alive. Userspace may try to recover by initializing the vcpu
	 * again, and there is no reason to affect the whole VM for this.
	 */
	num_mmus = atomic_read(&kvm->online_vcpus) * S2_MMU_PER_VCPU;

	if (num_mmus > kvm->arch.nested_mmus_size) {
		tmp = kvzalloc_objs(*tmp, num_mmus, GFP_KERNEL_ACCOUNT);
		if (!tmp)
			return -ENOMEM;

		write_lock(&kvm->mmu_lock);

		if (kvm->arch.nested_mmus_size) {
			memcpy(tmp, kvm->arch.nested_mmus,
			       size_mul(sizeof(*tmp), kvm->arch.nested_mmus_size));

			for (int i = 0; i < kvm->arch.nested_mmus_size; i++)
				tmp[i].pgt->mmu = &tmp[i];
		}

		swap(kvm->arch.nested_mmus, tmp);

		write_unlock(&kvm->mmu_lock);

		kvfree(tmp);
	}

	for (int i = kvm->arch.nested_mmus_size; !ret && i < num_mmus; i++)
		ret = init_nested_s2_mmu(kvm, &kvm->arch.nested_mmus[i]);

	if (ret) {
		for (int i = kvm->arch.nested_mmus_size; i < num_mmus; i++)
			kvm_free_stage2_pgd(&kvm->arch.nested_mmus[i]);

		free_page((unsigned long)vcpu->arch.ctxt.vncr_array);
		vcpu->arch.ctxt.vncr_array = NULL;

		return ret;
	}

	kvm->arch.nested_mmus_size = num_mmus;

	return 0;
}

/* …（同文件无关代码省略）… */
static unsigned int __ttl_to_size(u8 ttl)
{
	int level = ttl & 3;
	int gran = (ttl >> 2) & 3;
	unsigned int max_size = 0;

	switch (gran) {
	case TLBI_TTL_TG_4K:
		switch (level) {
		case 0:
			break;
		case 1:
			max_size = SZ_1G;
			break;
		case 2:
			max_size = SZ_2M;
			break;
		case 3:
			max_size = SZ_4K;
			break;
		}
		break;
	case TLBI_TTL_TG_16K:
		switch (level) {
		case 0:
		case 1:
			break;
		case 2:
			max_size = SZ_32M;
			break;
		case 3:
			max_size = SZ_16K;
			break;
		}
		break;
	case TLBI_TTL_TG_64K:
		switch (level) {
		case 0:
		case 1:
			/* No 52bit IPA support */
			break;
		case 2:
			max_size = SZ_512M;
			break;
		case 3:
			max_size = SZ_64K;
			break;
		}
		break;
	default:			/* No size information */
		break;
	}

	return max_size;
}
/* …（同文件无关代码省略）… */
static unsigned int ttl_to_size(u8 ttl)
{
	return __ttl_to_size(ttl) ?: SZ_1G;
}
/* …（同文件无关代码省略）… */
static u8 pgshift_level_to_ttl(u16 shift, s8 level)
{
	u8 ttl;

	/*
	 * If we don't have a proper level, fallback to the maximum
	 * size.
	 */
	if (level < 0)
		return 0;

	switch(shift) {
	case 12:
		ttl = TLBI_TTL_TG_4K;
		break;
	case 14:
		ttl = TLBI_TTL_TG_16K;
		break;
	case 16:
		ttl = TLBI_TTL_TG_64K;
		break;
	default:
		BUG();
	}

	ttl <<= 2;
	ttl |= level & 3;

	return ttl;
}
/* …（同文件无关代码省略）… */
	write_lock(&kvm->mmu_lock);

	for (int i = 0; i < kvm->arch.nested_mmus_size; i++) {
		struct kvm_s2_mmu *mmu = &kvm->arch.nested_mmus[i];

		if (!kvm_s2_mmu_valid(mmu))
			continue;
/* …（同文件无关代码省略）… */
	 *   if S2 translation is disabled.
	 */
	for (int i = 0; i < kvm->arch.nested_mmus_size; i++) {
		struct kvm_s2_mmu *mmu = &kvm->arch.nested_mmus[i];

		if (!kvm_s2_mmu_valid(mmu))
			continue;
/* …（同文件无关代码省略）… */
	for (i = kvm->arch.nested_mmus_next;
	     i < (kvm->arch.nested_mmus_size + kvm->arch.nested_mmus_next);
	     i++) {
		s2_mmu = &kvm->arch.nested_mmus[i % kvm->arch.nested_mmus_size];

		if (atomic_read(&s2_mmu->refcnt) == 0)
			break;
/* …（同文件无关代码省略）… */
static int unmap_l1_vncr(struct vncr_tlb *vt)
{
	int cpu = atomic_xchg_relaxed(&vt->cpu, -1);

	if (cpu != -1)
		clear_fixmap(vncr_fixmap(cpu));

	return cpu;
}
/* …（同文件无关代码省略）… */
static void invalidate_vncr(struct kvm *kvm, struct vncr_tlb *vt)
{
	BUG_ON(!vt->valid);
	vt->valid = false;
	unmap_l1_vncr(vt);
	atomic_dec(&kvm->arch.vncr_tlb_count);
}
/* …（同文件无关代码省略）… */
static bool vncr_tlb_intersects(struct vncr_tlb *vt, u64 addr,
				u64 scope_start, u64 scope_size)
{
	u64 tlb_size, tlb_start, tlb_end, scope_end;

	tlb_size = ttl_to_size(pgshift_level_to_ttl(vt->wi.pgshift, vt->wr.level));

	tlb_start = addr & ~(tlb_size - 1);
	tlb_end = tlb_start + tlb_size - 1;
	scope_end = scope_start + scope_size - 1;

	return !(tlb_end < scope_start || tlb_start > scope_end);
}
/* …（同文件无关代码省略）… */
static void kvm_invalidate_vncr_ipa(struct kvm *kvm, u64 start, u64 end)
{
	struct kvm_vcpu *vcpu;
	struct vncr_tlb *vt;
	unsigned long i;

	lockdep_assert_held_write(&kvm->mmu_lock);

	if (!kvm_has_feat(kvm, ID_AA64MMFR4_EL1, NV_frac, NV2_ONLY))
		return;

	/*
	 * Note that invalidating the VNCR on the back of an MMU notifier
	 * doesn't require messing with the invalidation counter for a
	 * parallel walk. The notifier itself will have bumped the counter,
	 * making sure we rewalk.
	 */
	kvm_for_each_vncr_tlb(i, vcpu, vt, kvm)
		if (vncr_tlb_intersects(vt, vt->wr.pa, start, end - start))
			invalidate_vncr(kvm, vt);
}
/* …（同文件无关代码省略）… */
struct s1e2_tlbi_scope {
	enum {
		TLBI_ALL,
		TLBI_VA,
		TLBI_VAA,
		TLBI_ASID,
	} type;

	u16 asid;
	u64 va;
	u64 size;
};
/* …（同文件
