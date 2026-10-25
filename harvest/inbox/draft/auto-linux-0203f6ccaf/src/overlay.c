// AUTO-DRAFT from torvalds/linux PR #21a7f44fd0765fcc5194130b28767786eba1d1e7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
struct fragment {
	struct device_node *overlay;
	struct device_node *target;
};
/* …（同文件无关代码省略）… */
struct overlay_changeset {
	int id;
	struct list_head ovcs_list;
	const void *new_fdt;
	const void *overlay_mem;
	struct device_node *overlay_root;
	enum of_overlay_notify_action notify_state;
	int count;
	struct fragment *fragments;
	bool symbols_fragment;
	struct of_changeset cset;
};
/* …（同文件无关代码省略）… */
#define DTSF_APPLY_FAIL		0x01
#define DTSF_REVERT_FAIL	0x02
/* …（同文件无关代码省略）… */
static int devicetree_corrupt(void)
{
	return devicetree_state_flags &
		(DTSF_APPLY_FAIL | DTSF_REVERT_FAIL);
}
/* …（同文件无关代码省略）… */
static int overlay_notify(struct overlay_changeset *ovcs,
		enum of_overlay_notify_action action)
{
	struct of_overlay_notify_data nd;
	int i, ret;

	ovcs->notify_state = action;

	for (i = 0; i < ovcs->count; i++) {
		struct fragment *fragment = &ovcs->fragments[i];

		nd.target = fragment->target;
		nd.overlay = fragment->overlay;

		ret = blocking_notifier_call_chain(&overlay_notify_chain,
						   action, &nd);
		if (notifier_to_errno(ret)) {
			ret = notifier_to_errno(ret);
			pr_err("overlay changeset %s notifier error %d, target: %pOF\n",
			       of_overlay_action_name(action), ret, nd.target);
			return ret;
		}
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static void free_overlay_changeset(struct overlay_changeset *ovcs)
{
	int i;

	if (ovcs->cset.entries.next)
		of_changeset_destroy(&ovcs->cset);

	/* a failed idr_alloc() leaves its negative error in ovcs->id */
	if (ovcs->id > 0) {
		idr_remove(&ovcs_idr, ovcs->id);
		list_del(&ovcs->ovcs_list);
		ovcs->id = 0;
	}


	for (i = 0; i < ovcs->count; i++) {
		of_node_put(ovcs->fragments[i].target);
		of_node_put(ovcs->fragments[i].overlay);
	}
	kfree(ovcs->fragments);

	/*
	 * There should be no live pointers into ovcs->overlay_mem and
	 * ovcs->new_fdt due to the policy that overlay notifiers are not
	 * allowed to retain pointers into the overlay devicetree other
	 * than during the window from OF_OVERLAY_PRE_APPLY overlay
	 * notifiers until the OF_OVERLAY_POST_REMOVE overlay notifiers.
	 *
	 * A memory leak will occur here if within the window.
	 */

	if (ovcs->notify_state == OF_OVERLAY_INIT ||
	    ovcs->notify_state == OF_OVERLAY_POST_REMOVE) {
		kfree(ovcs->overlay_mem);
		kfree(ovcs->new_fdt);
	}
	kfree(ovcs);
}
/* …（同文件无关代码省略）… */
static int find_node(const struct device_node *tree, struct device_node *np)
{
	if (tree == np)
		return 1;

	for_each_child_of_node_scoped(tree, child) {
		if (find_node(child, np))
			return 1;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static int node_overlaps_later_cs(struct overlay_changeset *remove_ovcs,
		struct device_node *remove_ce_node)
{
	struct overlay_changeset *ovcs;
	struct of_changeset_entry *ce;

	list_for_each_entry_reverse(ovcs, &ovcs_list, ovcs_list) {
		if (ovcs == remove_ovcs)
			break;

		list_for_each_entry(ce, &ovcs->cset.entries, node) {
			if (find_node(ce->np, remove_ce_node)) {
				pr_err("%s: #%d overlaps with #%d @%pOF\n",
					__func__, remove_ovcs->id, ovcs->id,
					remove_ce_node);
				return 1;
			}
			if (find_node(remove_ce_node, ce->np)) {
				pr_err("%s: #%d overlaps with #%d @%pOF\n",
					__func__, remove_ovcs->id, ovcs->id,
					remove_ce_node);
				return 1;
			}
		}
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static int overlay_removal_is_ok(struct overlay_changeset *remove_ovcs)
{
	struct of_changeset_entry *remove_ce;

	list_for_each_entry(remove_ce, &remove_ovcs->cset.entries, node) {
		if (node_overlaps_later_cs(remove_ovcs, remove_ce->np)) {
			pr_err("overlay #%d is not topmost\n", remove_ovcs->id);
			return 0;
		}
	}

	return 1;
}
/* …（同文件无关代码省略）… */
int of_overlay_remove(int *ovcs_id)
{
	struct overlay_changeset *ovcs;
	int ret, ret_apply, ret_tmp;

	if (*ovcs_id == 0)
		return 0;

	if (devicetree_corrupt()) {
		pr_err("suspect devicetree state, refuse to remove overlay\n");
		ret = -EBUSY;
		goto out;
	}

	mutex_lock(&of_mutex);

	ovcs = idr_find(&ovcs_idr, *ovcs_id);
	if (!ovcs) {
		ret = -ENODEV;
		pr_err("remove: Could not find overlay #%d\n", *ovcs_id);
		goto err_unlock;
	}

	if (!overlay_removal_is_ok(ovcs)) {
		ret = -EBUSY;
		goto err_unlock;
	}

	ret = overlay_notify(ovcs, OF_OVERLAY_PRE_REMOVE);
	if (ret)
		goto err_unlock;

	ret_apply = 0;
	ret = __of_changeset_revert_entries(&ovcs->cset, &ret_apply);
	if (ret) {
		if (ret_apply)
			devicetree_state_flags |= DTSF_REVERT_FAIL;
		goto err_unlock;
	}

	ret = __of_changeset_revert_notify(&ovcs->cset);
	if (ret)
		pr_err("overlay remove changeset entry notify error %d\n", ret);
	/* notify failure is not fatal, continue */

	*ovcs_id = 0;

	/*
	 * Note that the overlay memory will be kfree()ed by
	 * free_overlay_changeset() even if the notifier for
	 * OF_OVERLAY_POST_REMOVE returns an error.
	 */
	ret_tmp = overlay_notify(ovcs, OF_OVERLAY_POST_REMOVE);
	if (ret_tmp)
		if (!ret)
			ret = ret_tmp;

	free_overlay_changeset(ovcs);

err_unlock:
	/*
	 * If jumped over free_overlay_changeset(), then did not kfree()
	 * overlay related memory.  This is a memory leak unless a subsequent
	 * of_overlay_remove() of this overlay is successful.
	 */
	mutex_unlock(&of_mutex);

out:
	pr_debug("%s() err=%d\n", __func__, ret);

	return ret;
}
EXPORT_SYMBOL_GPL(of_overlay_remove);

/**
 * of_overlay_remove_all() - Reverts and frees all overlay changesets
 *
 * Removes all overlays from the system in the correct order.
 *
 * Return: 0 on success, or a negative error number
 */
int of_overlay_remove_all(void)
{
	struct overlay_changeset *ovcs, *ovcs_n;
	int ret;

	/* the tail of list is guaranteed to be safe to remove */
	list_for_each_entry_safe_reverse(ovcs, ovcs_n, &ovcs_list, ovcs_list) {
		ret = of_overlay_remove(&ovcs->id);
		if (ret)
			return ret;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(of_overlay_remove_all);
