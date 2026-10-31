// AUTO-DRAFT from torvalds/linux PR #17fa96334e15c6b38e8d9dd3627f3a6c5ca034f8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>

#define NR_VCPUS		1
/* …（同文件无关代码省略）… */
struct vm_gic {
	struct kvm_vm *vm;
	int gic_fd;
	u32 gic_dev_type;
};
/* …（同文件无关代码省略）… */
static int run_vcpu(struct kvm_vcpu *vcpu)
{
	return __vcpu_run(vcpu) ? -errno : 0;
}
/* …（同文件无关代码省略）… */
static void vm_gic_destroy(struct vm_gic *v)
{
	close(v->gic_fd);
	kvm_vm_free(v->vm);
}
/* …（同文件无关代码省略）… */
static void test_vgic_v5_addr_attrs(void)
{
	struct kvm_vcpu *vcpu;
	struct vm_gic v;
	u64 addr;
	int ret;

	v.gic_dev_type = KVM_DEV_TYPE_ARM_VGIC_V5;
	v.vm = __vm_create(VM_SHAPE_DEFAULT, NR_VCPUS, 0);
	v.gic_fd = kvm_create_device(v.vm, v.gic_dev_type);

	/* Check existing group/attributes */
	kvm_has_device_attr(v.gic_fd, KVM_DEV_ARM_VGIC_GRP_ADDR, gic_v5_irs_region.attr);

	/* check non existing attribute */
	ret = __kvm_has_device_attr(v.gic_fd, KVM_DEV_ARM_VGIC_GRP_ADDR, -1);
	TEST_ASSERT(ret && errno == ENXIO, "attribute not supported");

	/* get IRS base address before setting*/
	ret = __kvm_device_attr_get(v.gic_fd, KVM_DEV_ARM_VGIC_GRP_ADDR,
				    KVM_VGIC_V5_ADDR_TYPE_IRS, &addr);
	TEST_ASSERT(!ret && addr == (-1ULL), "GICv5 IRS returns VGIC_ADDR_UNDEF");

	/* misaligned IRS address settings */
	addr = gic_v5_irs_region.alignment / 0x10;
	ret = __kvm_device_attr_set(v.gic_fd, KVM_DEV_ARM_VGIC_GRP_ADDR,
				    KVM_VGIC_V5_ADDR_TYPE_IRS, &addr);
	TEST_ASSERT(ret && errno == EINVAL, "GIC IRS base not aligned");

	/* out of range address */
	addr = max_phys_size;
	ret = __kvm_device_attr_set(v.gic_fd, KVM_DEV_ARM_VGIC_GRP_ADDR,
				    KVM_VGIC_V5_ADDR_TYPE_IRS, &addr);
	TEST_ASSERT(ret && errno == E2BIG, "IRS address beyond IPA limit");

	/* Space for half an IRS (an IRS is: 2 * irs.alignment). */
	addr = max_phys_size - gic_v5_irs_region.alignment;
	ret = __kvm_device_attr_set(v.gic_fd, KVM_DEV_ARM_VGIC_GRP_ADDR,
				    KVM_VGIC_V5_ADDR_TYPE_IRS, &addr);
	TEST_ASSERT(ret && errno == E2BIG,
			"half of the IRS is beyond IPA limit");

	/* set IRS base address @0x0*/
	addr = 0x00000;
	ret = __kvm_device_attr_set(v.gic_fd, KVM_DEV_ARM_VGIC_GRP_ADDR,
				    KVM_VGIC_V5_ADDR_TYPE_IRS, &addr);
	TEST_ASSERT(!ret, "GICv5 IRS base correctly set");

	/* get IRS base address */
	addr = 0xbad;
	ret = __kvm_device_attr_get(v.gic_fd, KVM_DEV_ARM_VGIC_GRP_ADDR,
				    KVM_VGIC_V5_ADDR_TYPE_IRS, &addr);
	TEST_ASSERT(!ret && addr == 0, "GICv5 IRS base correctly set");

	/* Attempt to create a second IRS region */
	addr = 0xE0000;
	ret = __kvm_device_attr_set(v.gic_fd, KVM_DEV_ARM_VGIC_GRP_ADDR,
				    KVM_VGIC_V5_ADDR_TYPE_IRS, &addr);
	TEST_ASSERT(ret && errno == EEXIST, "GICv5 IRS base set again");

	vm_gic_destroy(&v);

	/* Try running a VM without ever setting the IRS base addr */
	v.vm = __vm_create(VM_SHAPE_DEFAULT, NR_VCPUS, 0);
	v.gic_fd = kvm_create_device(v.vm, v.gic_dev_type);
	vcpu = vm_vcpu_add(v.vm, 0, NULL);
	kvm_device_attr_set(v.gic_fd, KVM_DEV_ARM_VGIC_GRP_CTRL,
			    KVM_DEV_ARM_VGIC_CTRL_INIT, NULL);
	ret = run_vcpu(vcpu);
	TEST_ASSERT(ret && errno == ENXIO, "GICv5 IRS base not set");

	vm_gic_destroy(&v);
}
/* …（同文件无关代码省略）… */
	return 0;
}

void run_tests(u32 gic_dev_type)
{
	pr_info("Test VGICv5 address attrs\n");
	test_vgic_v5_addr_attrs();
