// AUTO-DRAFT from torvalds/linux PR #7d1471e50943196f909674d2d22eb4196c7df48e
	for (ds = dentry; !IS_ROOT(ds); ds = ds->d_parent)
		n++;
  // <<< BUG ANCHOR
	wnames = kmalloc_array(n, sizeof(char *), GFP_KERNEL);
	if (!wnames)
		goto err_out;
