// AUTO-DRAFT from torvalds/linux PR #7b63ef2d55f24519e7e9e5f4d15dbea03f126e40
	}
	ret = paths_from_inode(inum, ipath);
  // <<< BUG ANCHOR
	if (ret < 0)
		goto err;

	/*
	 * we deliberately ignore the bit ipath might have been too small to
