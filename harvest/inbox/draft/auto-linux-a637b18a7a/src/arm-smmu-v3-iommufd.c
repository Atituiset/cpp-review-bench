// AUTO-DRAFT from torvalds/linux PR #1021679bed5acc46e1b2dac78bdd006bd50970f4
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
static int arm_vsmmu_vsid_to_sid(struct arm_vsmmu *vsmmu, u32 vsid, u32 *sid)
{
	struct arm_smmu_master *master;
	struct device *dev;
	int ret = 0;

	xa_lock(&vsmmu->core.vdevs);
	dev = iommufd_viommu_find_dev(&vsmmu->core, (unsigned long)vsid);
	if (!dev) {
		ret = -EIO;
		goto unlock;
	}
	master = dev_iommu_priv_get(dev);

	/* At this moment, iommufd only supports PCI device that has one SID */
	if (sid)
		*sid = master->streams[0].id;
unlock:
	xa_unlock(&vsmmu->core.vdevs);
	return ret;
}
/* …（同文件无关代码省略）… */
struct arm_vsmmu_invalidation_cmd {
	union {
		struct arm_smmu_cmd cmd;
		struct iommu_viommu_arm_smmuv3_invalidate ucmd;
	};
};

/*
 * Convert, in place, the raw invalidation command into an internal format that
 * can be passed to arm_smmu_cmdq_issue_cmdlist(). Internally commands are
/* …（同文件无关代码省略）… */
static int arm_vsmmu_convert_user_cmd(struct arm_vsmmu *vsmmu,
				      struct arm_vsmmu_invalidation_cmd *cmd)
{
	/* Commands are le64 stored in u64 */
	cmd->cmd.data[0] = le64_to_cpu(cmd->ucmd.cmd[0]);
	cmd->cmd.data[1] = le64_to_cpu(cmd->ucmd.cmd[1]);

	switch (cmd->cmd.data[0] & CMDQ_0_OP) {
	case CMDQ_OP_TLBI_NSNH_ALL:
		/* Convert to NH_ALL */
		cmd->cmd.data[0] = CMDQ_OP_TLBI_NH_ALL |
			      FIELD_PREP(CMDQ_TLBI_0_VMID, vsmmu->vmid);
		cmd->cmd.data[1] = 0;
		break;
	case CMDQ_OP_TLBI_NH_VA:
	case CMDQ_OP_TLBI_NH_VAA:
	case CMDQ_OP_TLBI_NH_ALL:
	case CMDQ_OP_TLBI_NH_ASID:
		cmd->cmd.data[0] &= ~CMDQ_TLBI_0_VMID;
		cmd->cmd.data[0] |= FIELD_PREP(CMDQ_TLBI_0_VMID, vsmmu->vmid);
		break;
	case CMDQ_OP_ATC_INV:
	case CMDQ_OP_CFGI_CD:
	case CMDQ_OP_CFGI_CD_ALL: {
		u32 sid, vsid = FIELD_GET(CMDQ_CFGI_0_SID, cmd->cmd.data[0]);

		if (arm_vsmmu_vsid_to_sid(vsmmu, vsid, &sid))
			return -EIO;
		cmd->cmd.data[0] &= ~CMDQ_CFGI_0_SID;
		cmd->cmd.data[0] |= FIELD_PREP(CMDQ_CFGI_0_SID, sid);
		break;
	}
	default:
/* …（同文件无关代码省略）… */
			       struct iommu_user_data_array *array)
{
	struct arm_vsmmu *vsmmu = container_of(viommu, struct arm_vsmmu, core);
	struct arm_smmu_device *smmu = vsmmu->smmu;
	struct arm_vsmmu_invalidation_cmd *last;
	struct arm_vsmmu_invalidation_cmd *cmds;
	struct arm_vsmmu_invalidation_cmd *cur;
	struct arm_vsmmu_invalidation_cmd *end;
	int ret;

	cmds = kzalloc_objs(*cmds, array->entry_num);
	if (!cmds)
		return -ENOMEM;
	cur = cmds;
	end = cmds + array->entry_num;

	static_assert(sizeof(*cmds) == 2 * sizeof(u64));
	ret = iommu_copy_struct_from_full_user_array(
		cmds, sizeof(*cmds), array,
		IOMMU_VIOMMU_INVALIDATE_DATA_ARM_SMMUV3);
	if (ret)
		goto out;

	last = cmds;
	while (cur != end) {
		ret = arm_vsmmu_convert_user_cmd(vsmmu, cur);
		if (ret)
			goto out;

		/* FIXME work in blocks of CMDQ_BATCH_ENTRIES and copy each block? */
		cur++;
		if (cur != end && (cur - last) != CMDQ_BATCH_ENTRIES - 1)
			continue;

		/* FIXME always uses the main cmdq rather than trying to group by type */
		ret = __arm_smmu_cmdq_issue_cmdlist(smmu, &smmu->cmdq, &last->cmd,
						    cur - last, true);
		if (ret) {
			cur--;
			goto out;
		}
		last = cur;
	}
out:
	array->entry_num = cur - cmds;
	kfree(cmds);
	return ret;
}
