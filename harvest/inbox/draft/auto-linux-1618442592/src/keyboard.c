// AUTO-DRAFT from torvalds/linux PR #cb8fef4b07f71da01843457b66e579ad248a625e
return 0;
#endif
  // <<< BUG ANCHOR
	unsigned short __free(kfree) *new_map = kmalloc(sizeof(plain_map), GFP_KERNEL);
	if (!new_map)
		return -ENOMEM;
