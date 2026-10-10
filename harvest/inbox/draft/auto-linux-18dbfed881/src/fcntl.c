// AUTO-DRAFT from torvalds/linux PR #f99005fbed4e8071110faa350cc5fa453afa1455
	u64 __user *argp = (u64 __user *)arg;
	u64 hint;

	if (!inode_owner_or_capable(file_mnt_idmap(file), inode))
		return -EPERM;
