// AUTO-DRAFT from torvalds/linux PR #2c3418fffa9d037b2038a6db48be63f9e2291806
  // <<< BUG ANCHOR
	key_check(keyring);

	if (!(flags & KEY_ALLOC_BYPASS_RESTRICTION))
		restrict_link = keyring->restrict_link;

	key_ref = ERR_PTR(-ENOTDIR);
	if (keyring->type != &key_type_keyring)
		goto error_put_type;
/* …（同文件无关代码省略）… */
		goto error_link_end;
	}

	if (restrict_link && restrict_link->check) {
		ret = restrict_link->check(keyring, index_key.type,
					   &prep.payload, restrict_link->key);
