// AUTO-DRAFT from torvalds/linux PR #d89e1d186ec9271bf78e62771779e9fc03bf0fc3
	if (ext4_test_inode_flag(inode, EXT4_INODE_DAX))
		return -EOPNOTSUPP;

	res = ext4_convert_inline_data(inode);
	if (res)
		return res;
