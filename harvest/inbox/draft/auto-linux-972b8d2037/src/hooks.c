// AUTO-DRAFT from torvalds/linux PR #67f0943b394d920b6c142aad8c6af94340342ae7
static inline struct inode_security_struct *backing_inode_security_novalidate(struct dentry *dentry)
{
	return selinux_inode(d_backing_inode(dentry));
}
/* …（同文件无关代码省略）… */
	 * will be used for both mounts)
	 */
	if ((sbsec->flags & SE_SBINITIALIZED) && (sb->s_type->fs_flags & FS_BINARY_MOUNTDATA)
	    && !opts)  // <<< BUG ANCHOR
		goto out;

	root_isec = backing_inode_security_novalidate(root);

/* …（同文件无关代码省略）… */
		/* previously mounted with options, but not on this attempt? */
		if ((sbsec->flags & SE_MNTMASK) && !opts)
			goto out_double_mount;
		rc = 0;
		goto out;
	}
