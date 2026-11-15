// AUTO-DRAFT from torvalds/linux PR #aacb7797504183a90d66b19b1efb555cfe9b3eec
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
static int ovl_cleanup_locked(struct ovl_fs *ofs, struct inode *wdir,
			      struct dentry *wdentry)
{
	int err;

	dget(wdentry);
	if (d_is_dir(wdentry))
		err = ovl_do_rmdir(ofs, wdir, wdentry);
	else
		err = ovl_do_unlink(ofs, wdir, wdentry);
	dput(wdentry);

	if (err) {
		pr_err("cleanup of '%pd2' failed (%i)\n",
		       wdentry, err);
	}

	return err;
}
/* …（同文件无关代码省略）… */
int ovl_cleanup(struct ovl_fs *ofs, struct dentry *workdir,
		struct dentry *wdentry)
{
	wdentry = start_removing_dentry(workdir, wdentry);
	if (IS_ERR(wdentry))
		return PTR_ERR(wdentry);

	ovl_cleanup_locked(ofs, workdir->d_inode, wdentry);
	end_removing(wdentry);

	return 0;
}
/* …（同文件无关代码省略）… */
static int ovl_set_opaque_xerr(struct dentry *dentry, struct dentry *upper,
			       int xerr)
{
	struct ovl_fs *ofs = OVL_FS(dentry->d_sb);
	int err;

	err = ovl_check_setxattr(ofs, upper, OVL_XATTR_OPAQUE, "y", 1, xerr);
	if (!err)
		ovl_dentry_set_opaque(dentry);

	return err;
}
/* …（同文件无关代码省略）… */
static int ovl_set_opaque(struct dentry *dentry, struct dentry *upperdentry)
{
	/*
	 * Fail with -EIO when trying to create opaque dir and upper doesn't
	 * support xattrs. ovl_rename() calls ovl_set_opaque_xerr(-EXDEV) to
	 * return a specific error for noxattr case.
	 */
	return ovl_set_opaque_xerr(dentry, upperdentry, -EIO);
}
/* …（同文件无关代码省略）… */
static int ovl_instantiate(struct dentry *dentry, struct inode *inode,
			   struct dentry *newdentry, bool hardlink, struct file *tmpfile)
{
	struct ovl_inode_params oip = {
		.upperdentry = newdentry,
		.newinode = inode,
	};

	ovl_dentry_set_upper_alias(dentry);

	if (!hardlink) {
		/*
		 * ovl_obtain_alias() can be called after ovl_create_real()
		 * and before we get here, so we may get an inode from cache
		 * with the same real upperdentry that is not the inode we
		 * pre-allocated.  In this case we will use the cached inode
		 * to instantiate the new dentry.
		 *
		 * XXX: if we ever use ovl_obtain_alias() to decode directory
		 * file handles, need to use ovl_get_inode_locked() and
		 * d_instantiate_new() here to prevent from creating two
		 * hashed directory inode aliases.  We then need to return
		 * the obtained alias to ovl_mkdir().
		 */
		inode = ovl_get_inode(dentry->d_sb, &oip);
		if (IS_ERR(inode))
			return PTR_ERR(inode);
		if (inode == oip.newinode)
			ovl_set_flag(OVL_UPPERDATA, inode);
	} else {
		WARN_ON(ovl_inode_real(inode) != d_inode(newdentry));
		dput(newdentry);
		inc_nlink(inode);
	}

	if (tmpfile)
		d_mark_tmpfile(tmpfile, inode);

	d_instantiate(dentry, inode);
	/*
	 * Unless it is a tmpfile, the dentry is hashed and a path walk can find
	 * it at any time.  Set the revalidate flags only now, because
	 * ovl_dentry_revalidate_common() does not expect them on a negative
	 * dentry.
	 */
	ovl_dentry_init_reval(dentry, ovl_dentry_upper(dentry), NULL);
	if (inode != oip.newinode) {
		pr_warn_ratelimited("newly created inode found in cache (%pd2)\n",
				    dentry);
	}

	/* Force lookup of new upper hardlink to find its lower */
	if (hardlink)
		d_drop(dentry);

	return 0;
}
/* …（同文件无关代码省略）… */
}

static int ovl_create_upper(struct dentry *dentry, struct inode *inode,
			    struct ovl_cattr *attr)
{
	struct ovl_fs *ofs = OVL_FS(dentry->d_sb);
	struct dentry *upperdir = ovl_dentry_upper(dentry->d_parent);
/* …（同文件无关代码省略）… */
		ovl_set_opaque(dentry, newdentry);
	}

	ovl_dir_modified(dentry->d_parent, false);
	err = ovl_instantiate(dentry, inode, newdentry, !!attr->hardlink, NULL);
	if (err)
		goto out_cleanup;
/* …（同文件无关代码省略）… */
}

static int ovl_create_over_whiteout(struct dentry *dentry, struct inode *inode,
				    struct ovl_cattr *cattr)
{
	struct ovl_fs *ofs = OVL_FS(dentry->d_sb);
	struct dentry *workdir = ovl_workdir(dentry);
/* …（同文件无关代码省略）… */
		if (err)
			goto out_cleanup_unlocked;
	}
	ovl_dir_modified(dentry->d_parent, false);
	err = ovl_instantiate(dentry, inode, newdentry, hardlink, NULL);
	if (err) {
		ovl_cleanup(ofs, upperdir, newdentry);
/* …（同文件无关代码省略）… */
static const struct cred *ovl_override_creator_creds(const struct cred *original_creds,
						     struct dentry *dentry, struct inode *inode, umode_t mode)
{
	int err;

	if (WARN_ON_ONCE(current->cred != ovl_creds(dentry->d_sb)))
		return ERR_PTR(-EINVAL);

	CLASS(prepare_creds, override_cred)();
	if (!override_cred)
		return ERR_PTR(-ENOMEM);

	override_cred->fsuid = inode->i_uid;
	override_cred->fsgid = inode->i_gid;

	err = security_dentry_create_files_as(dentry, mode, &dentry->d_name,
					      original_creds, override_cred);
	if (err)
		return ERR_PTR(err);

	return override_creds(no_free_ptr(override_cred));
}
/* …（同文件无关代码省略）… */

static int ovl_create_handle_whiteouts(struct dentry *dentry,
				       struct inode *inode,
				       struct ovl_cattr *attr)
{
	if (!ovl_dentry_is_whiteout(dentry))
		return ovl_create_upper(dentry, inode, attr);

	return ovl_create_over_whiteout(dentry, inode, attr);
}

static int ovl_create_or_link(struct dentry *dentry, struct inode *inode,
			      struct ovl_cattr *attr, bool origin)
{
	int err;
	struct dentry *parent = dentry->d_parent;

	scoped_class(override_creds_ovl, original_creds, dentry->d_sb) {
		/*
		 * When linking a file with copy up origin into a new parent, mark the
		 * new parent dir "impure".
		 */
		if (origin) {
			err = ovl_set_impure(parent, ovl_dentry_upper(parent));
			if (err)
				return err;
		}

		/*
		 * In the creation cases(create, mkdir, mknod, symlink),
		 * ovl should transfer current's fs{u,g}id to underlying
		 * fs. Because underlying fs want to initialize its new
		 * inode owner using current's fs{u,g}id. And in this
		 * case, the @inode is a new inode that is initialized
		 * in inode_init_owner() to current's fs{u,g}id. So use
		 * the inode's i_{u,g}id to override the cred's fs{u,g}id.
		 *
		 * But in the other hardlink case, ovl_link() does not
		 * create a new inode, so j
