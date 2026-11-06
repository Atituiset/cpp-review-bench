// AUTO-DRAFT from torvalds/linux PR #eedaa7d61f1ec8ae053a9ece0d553785350ac486
	int ret;
  // <<< BUG ANCHOR
	if (IS_ERR(efi_name))
		return PTR_ERR(no_free_ptr(efi_name));

	ret = efivar_lock();
	if (ret)
