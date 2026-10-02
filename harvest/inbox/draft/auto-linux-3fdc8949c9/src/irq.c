// AUTO-DRAFT from torvalds/linux PR #ce1e0223d8ad4211275c82a17ed6d43ab81e13d9

	list_for_each_entry_safe(desc, temp_desc, &intc_parent_list, list) {
		list_del(&desc->list);
		kfree(desc);
	}
err:
