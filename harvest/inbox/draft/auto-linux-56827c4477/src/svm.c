// AUTO-DRAFT from torvalds/linux PR #03741bcea0a9ec4c8f386fd5c6051c1a14070240
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
	svm->msrpm = svm_vcpu_alloc_msrpm();
	if (!svm->msrpm) {
		err = -ENOMEM;
		goto error_free_sev;
	}

	svm->x2avic_msrs_intercepted = true;
/* …（同文件无关代码省略）… */

	return 0;

error_free_sev:
	sev_free_vcpu(vcpu);
error_free_vmcb_page:
/* …（同文件无关代码省略）… */

	WARN_ON_ONCE(!list_empty(&svm->ir_list));

	svm_leave_nested(vcpu);
	svm_free_nested(svm);
