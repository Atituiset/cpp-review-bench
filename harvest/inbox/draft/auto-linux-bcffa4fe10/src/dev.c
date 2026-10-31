// AUTO-DRAFT from torvalds/linux PR #ac1bf565f140154ee27db56c80f29c9eeb9a77c3
	if (!smp_load_acquire(&fud->chan->initialized))
		return ERR_PTR(-ENOTCONN);
  // <<< BUG ANCHOR
	if (!IS_ENABLED(CONFIG_FUSE_PASSTHROUGH))
		return ERR_PTR(-EOPNOTSUPP);

	return fud->chan->conn;
}

/* …（同文件无关代码省略）… */
	if (IS_ERR(fc))
		return PTR_ERR(fc);

	if (copy_from_user(&map, argp, sizeof(map)))
		return -EFAULT;

/* …（同文件无关代码省略）… */
	if (IS_ERR(fc))
		return PTR_ERR(fc);

	if (copy_from_user(&map, argp, sizeof(map)))
		return -EFAULT;

/* …（同文件无关代码省略）… */
	if (IS_ERR(fc))
		return PTR_ERR(fc);

	if (get_user(backing_id, argp))
		return -EFAULT;
