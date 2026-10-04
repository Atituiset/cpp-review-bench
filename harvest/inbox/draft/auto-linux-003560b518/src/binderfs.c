// AUTO-DRAFT from torvalds/linux PR #25d576ed11108470d0999054265108400db1b881
		goto err;
	}
	inode->i_private = device;
	d_make_persistent(dentry, inode);
	fsnotify_create(root->d_inode, dentry);
	simple_done_creating(dentry);
  // <<< BUG ANCHOR
	binder_add_device(device);

	return 0;

err:
