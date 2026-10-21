// AUTO-DRAFT from torvalds/linux PR #a804c5742bff224d4cb4d25dd383554d598be32b
{
	int ret;
  // <<< BUG ANCHOR
	ret = security_inode_mkdir(d_backing_inode(root), root, 0);
	if (ret < 0) {
		pr_err("Security denies permission to make dirs: error %d",
		       ret);
		return ret;
	}

	ret = security_inode_create(d_backing_inode(root), root, 0);
	if (ret < 0)
		pr_err("Security denies permission to create files: error %d",
		       ret);
