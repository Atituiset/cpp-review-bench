// AUTO-DRAFT from torvalds/linux PR #ebb7a8dc1e74c705daffe11c2faf38ec2628f838
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
struct scmi_xfers_info {
	unsigned long *xfer_alloc_table;
	spinlock_t xfer_lock;
	int max_msg;
	struct hlist_head free_xfers;
	DECLARE_HASHTABLE(pending_xfers, SCMI_PENDING_XFERS_HT_ORDER_SZ);
};
/* …（同文件无关代码省略）… */
struct scmi_info {
	int id;
	struct device *dev;
	const struct scmi_desc *desc;
	struct scmi_base_info version;
	struct scmi_handle handle;
	struct scmi_xfers_info tx_minfo;
	struct scmi_xfers_info rx_minfo;
	struct idr tx_idr;
	struct idr rx_idr;
	struct idr protocols;
	/* Ensure mutual exclusive access to protocols instance array */
	struct mutex protocols_mtx;
	u8 *protocols_imp;
	struct idr active_protocols;
	void *notify_priv;
	struct list_head node;
	int users;
	struct notifier_block bus_nb;
	struct notifier_block dev_req_nb;
	/* Serialize device creation process for this instance */
	struct mutex devreq_mtx;
	struct scmi_debug_info *dbg;
	void *raw;
};
/* …（同文件无关代码省略）… */
static void scmi_debugfs_counters_setup(struct scmi_debug_info *dbg,
					struct dentry *trans)
{
	struct dentry *counters;
	int idx;

	counters = debugfs_create_dir("counters", trans);

	for (idx = 0; idx < SCMI_DEBUG_COUNTERS_LAST; idx++)
		debugfs_create_atomic_t(dbg_counter_strs[idx], 0600, counters,
					&dbg->counters[idx]);

	debugfs_create_file("reset", 0200, counters, dbg, &fops_reset_counts);
}
/* …（同文件无关代码省略）… */
static void scmi_debugfs_common_cleanup(void *d)
{
	struct scmi_debug_info *dbg = d;

	if (!dbg)
		return;

	debugfs_remove_recursive(dbg->top_dentry);
	kfree(dbg->name);
	kfree(dbg->type);
}

static const char *scmi_acpi_device_hid(struct acpi_device *adev)
{
#ifdef CONFIG_ACPI
	return adev ? acpi_device_hid(adev) : "unknown";
#else
	return "unknown";
#endif
}

static struct scmi_debug_info *scmi_debugfs_common_setup(struct scmi_info *info)
{
	char top_dir[16];
	struct dentry *trans, *top_dentry;
	struct scmi_debug_info *dbg;
	const char *c_ptr = NULL;

	dbg = devm_kzalloc_obj(info->dev, *dbg);
	if (!dbg)
		return NULL;

	dbg->name = kstrdup(fwnode_get_name(dev_fwnode(info->dev)), GFP_KERNEL);
	if (!dbg->name) {
		devm_kfree(info->dev, dbg);
		return NULL;
	}

	if (fwnode_property_read_string(dev_fwnode(info->dev), "compatible",
					&c_ptr))
		c_ptr = scmi_acpi_device_hid(ACPI_COMPANION(info->dev));

	dbg->type = kstrdup(c_ptr, GFP_KERNEL);
	if (!dbg->type) {
		kfree(dbg->name);
		devm_kfree(info->dev, dbg);
		return NULL;
	}

	snprintf(top_dir, 16, "%d", info->id);
	top_dentry = debugfs_create_dir(top_dir, scmi_top_dentry);
	trans = debugfs_create_dir("transport", top_dentry);

	dbg->is_atomic = info->desc->atomic_enabled &&
				is_transport_polling_capable(info->desc);

	debugfs_create_str("instance_name", 0400, top_dentry,
			   (char **)&dbg->name);

	debugfs_create_u32("atomic_threshold_us", 0400, top_dentry,
			   (u32 *)&info->desc->atomic_threshold);

	debugfs_create_str("type", 0400, trans, (char **)&dbg->type);

	debugfs_create_bool("is_atomic", 0400, trans, &dbg->is_atomic);

	debugfs_create_u32("max_rx_timeout_ms", 0400, trans,
			   (u32 *)&info->desc->max_rx_timeout_ms);

	debugfs_create_u32("max_msg_size", 0400, trans,
			   (u32 *)&info->desc->max_msg_size);

	debugfs_create_u32("tx_max_msg", 0400, trans,
			   (u32 *)&info->tx_minfo.max_msg);

	debugfs_create_u32("rx_max_msg", 0400, trans,
			   (u32 *)&info->rx_minfo.max_msg);

	if (IS_ENABLED(CONFIG_ARM_SCMI_DEBUG_COUNTERS))
		scmi_debugfs_counters_setup(dbg, trans);

	dbg->top_dentry = top_dentry;

	if (devm_add_action_or_reset(info->dev,
				     scmi_debugfs_common_cleanup, dbg))
		return NULL;

	return dbg;
}
