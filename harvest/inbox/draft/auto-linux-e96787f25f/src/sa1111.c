// AUTO-DRAFT from torvalds/linux PR #e0d8e8de8bcb1ef7b749033b5af5518ba8df7bec
	if (!pd)
		return -EINVAL;
  // <<< BUG ANCHOR
	sachip = devm_kzalloc(me, sizeof(struct sa1111), GFP_KERNEL);
	if (!sachip)
		return -ENOMEM;
