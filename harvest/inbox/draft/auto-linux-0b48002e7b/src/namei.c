// AUTO-DRAFT from torvalds/linux PR #a804c5742bff224d4cb4d25dd383554d598be32b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
static int check_acl(const struct mnt_idmap *idmap,
		     struct inode *inode, int mask)
{
#ifdef CONFIG_FS_POSIX_ACL
	struct posix_acl *acl;

	if (mask & MAY_NOT_BLOCK) {
		acl = get_cached_acl_rcu(inode, ACL_TYPE_ACCESS);
	        if (!acl)
	                return -EAGAIN;
		/* no ->get_inode_acl() calls in RCU mode... */
		if (is_uncached_acl(acl))
			return -ECHILD;
	        return posix_acl_permission(idmap, inode, acl, mask);
	}

	acl = get_inode_acl(inode, ACL_TYPE_ACCESS);
	if (IS_ERR(acl))
		return PTR_ERR(acl);
	if (acl) {
	        int error = posix_acl_permission(idmap, inode, acl, mask);
	        posix_acl_release(acl);
	        return error;
	}
#endif

	return -EAGAIN;
}
/* …（同文件无关代码省略）… */
static inline bool no_acl_inode(struct inode *inode)
{
#ifdef CONFIG_FS_POSIX_ACL
	return likely(!READ_ONCE(inode->i_acl));
#else
	return true;
#endif
}
/* …（同文件无关代码省略）… */
static int acl_permission_check(const struct mnt_idmap *idmap,
				struct inode *inode, int mask)
{
	unsigned int mode = inode->i_mode;
	vfsuid_t vfsuid;

	/*
	 * Common cheap case: everybody has the requested
	 * rights, and there are no ACLs to check. No need
	 * to do any owner/group checks in that case.
	 *
	 *  - 'mask&7' is the requested permission bit set
	 *  - multiplying by 0111 spreads them out to all of ugo
	 *  - '& ~mode' looks for missing inode permission bits
	 *  - the '!' is for "no missing permissions"
	 *
	 * After that, we just need to check that there are no
	 * ACL's on the inode - do the 'IS_POSIXACL()' check last
	 * because it will dereference the ->i_sb pointer and we
	 * want to avoid that if at all possible.
	 */
	if (!((mask & 7) * 0111 & ~mode)) {
		if (no_acl_inode(inode))
			return 0;
		if (!IS_POSIXACL(inode))
			return 0;
	}

	/* Are we the owner? If so, ACL's don't matter */
	vfsuid = i_uid_into_vfsuid(idmap, inode);
	if (likely(vfsuid_eq_kuid(vfsuid, current_fsuid()))) {
		mask &= 7;
		mode >>= 6;
		return (mask & ~mode) ? -EACCES : 0;
	}

	/* Do we have ACL's? */
	if (IS_POSIXACL(inode) && (mode & S_IRWXG)) {
		int error = check_acl(idmap, inode, mask);
		if (error != -EAGAIN)
			return error;
	}

	/* Only RWX matters for group/other mode bits */
	mask &= 7;

	/*
	 * Are the group permissions different from
	 * the other permissions in the bits we care
	 * about? Need to check group ownership if so.
	 */
	if (mask & (mode ^ (mode >> 3))) {
		vfsgid_t vfsgid = i_gid_into_vfsgid(idmap, inode);
		if (vfsgid_in_group_p(vfsgid))
			mode >>= 3;
	}

	/* Bits in 'mode' clear that we require? */
	return (mask & ~mode) ? -EACCES : 0;
}
/* …（同文件无关代码省略）… */
int generic_permission(const struct mnt_idmap *idmap, struct inode *inode,
		       int mask)
{
	int ret;

	/*
	 * Do the basic permission checks.
	 */
	ret = acl_permission_check(idmap, inode, mask);
	if (ret != -EACCES)
		return ret;

	if (S_ISDIR(inode->i_mode)) {
		/* DACs are overridable for directories */
		if (!(mask & MAY_WRITE))
			if (capable_wrt_inode_uidgid(idmap, inode,
						     CAP_DAC_READ_SEARCH))
				return 0;
		if (capable_wrt_inode_uidgid(idmap, inode,
					     CAP_DAC_OVERRIDE))
			return 0;
		return -EACCES;
	}

	/*
	 * Searching includes executable on directories, else just read.
	 */
	mask &= MAY_READ | MAY_WRITE | MAY_EXEC;
	if (mask == MAY_READ)
		if (capable_wrt_inode_uidgid(idmap, inode,
					     CAP_DAC_READ_SEARCH))
			return 0;
	/*
	 * Read/write DACs are always overridable.
	 * Executable DACs are overridable when there is
	 * at least one exec bit set.
	 */
	if (!(mask & MAY_EXEC) || (inode->i_mode & S_IXUGO))
		if (capable_wrt_inode_uidgid(idmap, inode,
					     CAP_DAC_OVERRIDE))
			return 0;

	return -EACCES;
}
/* …（同文件无关代码省略）… */
static inline int do_inode_permission(const struct mnt_idmap *idmap,
				      struct inode *inode, int mask)
{
	if (unlikely(!(inode->i_opflags & IOP_FASTPERM))) {
		if (likely(inode->i_op->permission))
			return inode->i_op->permission(idmap, inode, mask);

		/* This gets set once for the inode lifetime */
		spin_lock(&inode->i_lock);
		inode->i_opflags |= IOP_FASTPERM;
		spin_unlock(&inode->i_lock);
	}
	return generic_permission(idmap, inode, mask);
}
/* …（同文件无关代码省略）… */
static int sb_permission(struct super_block *sb, struct inode *inode, int mask)
{
	if (mask & MAY_WRITE) {
		umode_t mode = inode->i_mode;

		/* Nobody gets write access to a read-only fs. */
		if (sb_rdonly(sb) && (S_ISREG(mode) || S_ISDIR(mode) || S_ISLNK(mode)))
			return -EROFS;
	}
	return 0;
}
/* …（同文件无关代码省略）… */
int inode_permission(const struct mnt_idmap *idmap,
		     struct inode *inode, int mask)
{
	int retval;

	retval = sb_permission(inode->i_sb, inode, mask);
	if (unlikely(retval))
		return retval;

	if (mask & MAY_WRITE) {
		/*
		 * Nobody gets write access to an immutable file.
		 */
		if (unlikely(IS_IMMUTABLE(inode)))
			return -EPERM;

		/*
		 * Updating mtime will likely cause i_uid and i_gid to be
		 * written back improperly if their true value is unknown
		 * to the vfs.
		 */
		if (unlikely(HAS_UNMAPPED_ID(idmap, inode)))
			return -EACCES;
	}

	retval = do_inode_permission(idmap, inode, mask);
	if (unlikely(retval))
		return retval;

	retval = devcgroup_inode_permission(inode, mask);
	if (unlikely(retval))
		return retval;

	return security_inode_permission(inode, mask);
}
EXPORT_SYMBOL(inode_permission);

/* …（同文件无关代码省略）… */
	if (unlikely(((inode->i_mode & 0111) != 0111) || !no_acl_inode(inode)))
		return inode_permission(idmap, inode, mask);

	return security_inode_permission(inode, mask);
}

/**
/* …（同文件无关代码省略）… */
static inline umode_t vfs_prepare_mode(const struct mnt_idmap *idmap,
				       const struct inode *dir, umode_t mode,
				       umode_t mask_perms, umode_t type)
{
	mode = mode_strip_sgid(idmap, dir, mode);
	mode = mode_strip_umask(dir, mode);

	/*
	 * Apply the vfs mandated allowed permission mask and set the type of
	 * file to be created before we call into the filesystem.
	 */
	mode &= (mask_perms & ~S_IFMT
