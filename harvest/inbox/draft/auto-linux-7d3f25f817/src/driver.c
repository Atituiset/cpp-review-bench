// AUTO-DRAFT from torvalds/linux PR #ebb7a8dc1e74c705daffe11c2faf38ec2628f838
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
enum notify_type {
	SECURE_PARTITION,
	NON_SECURE_VM,
	SPM_FRAMEWORK,
	NS_HYP_FRAMEWORK,
};
/* …（同文件无关代码省略）… */
#define FFA_SECURE_PARTITION_ID_FLAG		BIT(15)
/* …（同文件无关代码省略）… */
struct notifier_cb_info {
	struct hlist_node hnode;
	struct ffa_device *dev;
	ffa_fwk_notifier_cb fwk_cb;
	ffa_notifier_cb cb;
	void *cb_data;
};

static int
/* …（同文件无关代码省略）… */
static enum notify_type ffa_notify_type_get(u16 vm_id)
{
	if (vm_id & FFA_SECURE_PARTITION_ID_FLAG)
		return SECURE_PARTITION;
	else
		return NON_SECURE_VM;
}
/* …（同文件无关代码省略）… */
static struct notifier_cb_info *
notifier_hnode_get_by_vmid(u16 notify_id, int vmid)
{
	struct notifier_cb_info *node;

	hash_for_each_possible(drv_info->notifier_hash, node, hnode, notify_id)
		if (node->fwk_cb && vmid == node->dev->vm_id)
			return node;

	return NULL;
}
/* …（同文件无关代码省略）… */
		return notifier_hnode_get_by_vmid(notify_id, vmid);

	hash_for_each_possible(drv_info->notifier_hash, node, hnode, notify_id)
		if (node->fwk_cb && vmid == node->dev->vm_id &&
		    uuid_equal(&node->dev->uuid, uuid))
			return node;

/* …（同文件无关代码省略）… */
	struct notifier_cb_info *node;

	hash_for_each_possible(drv_info->notifier_hash, node, hnode, notify_id)
		if (node->cb && type == ffa_notify_type_get(node->dev->vm_id))
			return node;

	return NULL;
/* …（同文件无关代码省略）… */

	cb_info->dev = dev;
	cb_info->cb_data = cb_data;
	if (is_framework)
		cb_info->fwk_cb = cb;
	else
