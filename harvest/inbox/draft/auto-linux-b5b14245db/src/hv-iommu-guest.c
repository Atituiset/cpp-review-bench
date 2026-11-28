// AUTO-DRAFT from torvalds/linux PR #8c8611425a367d250339cdb05aa898971d35abdb
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
static int hv_create_device_domain(struct hv_iommu_domain *hv_domain)
{
	int ret;
	u64 status;
	unsigned long flags;
	struct hv_input_create_device_domain *input;

	ret = ida_alloc_range(&hv_iommu_device->domain_ids,
			      hv_iommu_device->first_domain,
			      hv_iommu_device->last_domain, GFP_KERNEL);
	if (ret < 0)
		return ret;

	hv_domain->device_domain.partition_id = HV_PARTITION_ID_SELF;
	hv_domain->device_domain.domain_id.type = HV_DEVICE_DOMAIN_TYPE_S1;
	hv_domain->device_domain.domain_id.id = ret;
	hv_domain->hv_iommu = hv_iommu_device;

	local_irq_save(flags);

	input = *this_cpu_ptr(hyperv_pcpu_input_arg);
	memset(input, 0, sizeof(*input));
	input->device_domain = hv_domain->device_domain;
	input->create_device_domain_flags.forward_progress_required = 1;
	input->create_device_domain_flags.inherit_owning_vtl = 0;
	status = hv_do_hypercall(HVCALL_CREATE_DEVICE_DOMAIN, input, NULL);

	local_irq_restore(flags);

	if (!hv_result_success(status)) {
		hv_status_err(status, "HVCALL_CREATE_DEVICE_DOMAIN failed\n");
		ida_free(&hv_iommu_device->domain_ids,
			 hv_domain->device_domain.domain_id.id);
	}

	return hv_result_to_errno(status);
}

static void hv_delete_device_domain(struct hv_iommu_domain *hv_domain)
{
	u64 status;
	unsigned long flags;
/* …（同文件无关代码省略）… */

	local_irq_restore(flags);

	if (!hv_result_success(status))
		hv_status_err(status, "HVCALL_DELETE_DEVICE_DOMAIN failed\n");

	ida_free(&hv_domain->hv_iommu->domain_ids,
		 hv_domain->device_domain.domain_id.id);
}

static int
hv_configure_device_domain(struct hv_iommu_domain *hv_domain,
			   const struct hv_device_domain_settings *settings)
{
	u64 status;
	unsigned long flags;
	struct hv_input_configure_device_domain *input;

	local_irq_save(flags);

	input = *this_cpu_ptr(hyperv_pcpu_input_arg);
	memset(input, 0, sizeof(*input));
	input->device_domain = hv_domain->device_domain;
	input->settings = *settings;
	status = hv_do_hypercall(HVCALL_CONFIGURE_DEVICE_DOMAIN, input, NULL);

	local_irq_restore(flags);

	if (!hv_result_success(status))
		hv_status_err(status,
			      "HVCALL_CONFIGURE_DEVICE_DOMAIN failed\n");

	return hv_result_to_errno(status);
}
/* …（同文件无关代码省略）… */
static int
hv_create_configure_device_domain(struct hv_iommu_domain *hv_domain,
				  const struct hv_device_domain_settings *settings)
{
	int ret;

	ret = hv_create_device_domain(hv_domain);
	if (ret)
		return ret;

	ret = hv_configure_device_domain(hv_domain, settings);
	if (ret)
		hv_delete_device_domain(hv_domain);

	return ret;
}
/* …（同文件无关代码省略）… */
{
	struct hv_iommu_domain *hv_domain = to_hv_iommu_domain(domain);

	/* Free all remaining mappings */
	pt_iommu_deinit(&hv_domain->pt_iommu);

	hv_delete_device_domain(hv_domain);

	kfree(hv_domain);
}

/* …（同文件无关代码省略）… */
	settings.page_table_root = pt_info.gcr3_pt;
	settings.flags.first_stage_paging_mode = pt_info.levels == 5;

	ret = hv_create_configure_device_domain(hv_domain, &settings);
	if (ret)
		goto err_pt_deinit;

	return &hv_domain->domain;

err_pt_deinit:
