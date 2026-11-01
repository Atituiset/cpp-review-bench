// AUTO-DRAFT from torvalds/linux PR #ca62fc8c6b1841039aee6de1ef03c9863751ec04
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
static bool ovl_must_copy_xattr(const char *name)
{
	return !strcmp(name, XATTR_POSIX_ACL_ACCESS) ||
	       !strcmp(name, XATTR_POSIX_ACL_DEFAULT) ||
	       !strncmp(name, XATTR_SECURITY_PREFIX, XATTR_SECURITY_PREFIX_LEN);
}
/* …（同文件无关代码省略）… */
	return err;
}

int ovl_copy_xattr(struct super_block *sb, const struct path *oldpath, struct dentry *new)
{
	struct dentry *old = oldpath->dentry;
/* …（同文件无关代码省略）… */
			break;
		}

retry:
		size = ovl_do_getxattr(oldpath, name, value, value_size);
		if (size == -ERANGE)
			size = ovl_do_getxattr(oldpath, name, NULL, 0);

		if (size < 0) {
			error = size;
			break;
		}

		if (size > value_size) {
			void *new;

			new = kvmalloc(size, GFP_KERNEL);
			if (!new) {
				error = -ENOMEM;
				break;
			}
			kvfree(value);
			value = new;
			value_size = size;
			goto retry;
		}

		error = ovl_do_setxattr(OVL_FS(sb), new, name, value, size, 0);
		if (error) {
			if (error != -EOPNOTSUPP || ovl_must_copy_xattr(name))
/* …（同文件无关代码省略）… */
struct ovl_fh *ovl_encode_real_fh(struct ovl_fs *ofs, struct inode *realinode,
				  bool is_upper)
{
	struct ovl_fh *fh;
	int fh_type, dwords;
	int buflen = MAX_HANDLE_SZ;
	uuid_t *uuid = &realinode->i_sb->s_uuid;
	int err;

	/* Make sure the real fid stays 32bit aligned */
	BUILD_BUG_ON(OVL_FH_FID_OFFSET % 4);
	BUILD_BUG_ON(MAX_HANDLE_SZ + OVL_FH_FID_OFFSET > 255);

	fh = kzalloc(buflen + OVL_FH_FID_OFFSET, GFP_KERNEL);
	if (!fh)
		return ERR_PTR(-ENOMEM);

	/*
	 * We encode a non-connectable file handle for non-dir, because we
	 * only need to find the lower inode number and we don't want to pay
	 * the price or reconnecting the dentry.
	 */
	dwords = buflen >> 2;
	fh_type = exportfs_encode_inode_fh(realinode, (void *)fh->fb.fid,
					   &dwords, NULL, 0);
	buflen = (dwords << 2);

	err = -EIO;
	if (fh_type < 0 || fh_type == FILEID_INVALID ||
	    WARN_ON(buflen > MAX_HANDLE_SZ))
		goto out_err;

	fh->fb.version = OVL_FH_VERSION;
	fh->fb.magic = OVL_FH_MAGIC;
	fh->fb.type = fh_type;
	fh->fb.flags = OVL_FH_FLAG_CPU_ENDIAN;
	/*
	 * When we will want to decode an overlay dentry from this handle
	 * and all layers are on the same fs, if we get a disconncted real
	 * dentry when we decode fid, the only way to tell if we should assign
	 * it to upperdentry or to lowerstack is by checking this flag.
	 */
	if (is_upper)
		fh->fb.flags |= OVL_FH_FLAG_PATH_UPPER;
	fh->fb.len = sizeof(fh->fb) + buflen;
	if (ovl_origin_uuid(ofs))
		fh->fb.uuid = *uuid;

	return fh;

out_err:
	kfree(fh);
	return ERR_PTR(err);
}
/* …（同文件无关代码省略）… */
	 * When lower layer doesn't support export operations store a 'null' fh,
	 * so we can use the overlay.origin xattr to distignuish between a copy
	 * up and a pure upper inode.
	 */
	if (!ovl_can_decode_fh(origin->d_sb))
		return NULL;

	return ovl_encode_real_fh(ofs, d_inode(origin), false);
/* …（同文件无关代码省略）… */
	rd.old_parent = c->workdir;
	rd.new_parent = c->destdir;
	rd.flags = 0;
	err = start_renaming_dentry(&rd, 0, temp,
				    &QSTR_LEN(c->destname.name, c->destname.len));
	if (err) {
		/* temp or workdir moved underneath us? map to -EIO */
		err = -EIO;
	}
/* …（同文件无关代码省略）… */

	if (!c->metacopy)
		ovl_set_upperdata(inode);
	ovl_inode_update(inode, temp);
	if (S_ISDIR(inode->i_mode))
		ovl_set_flag(OVL_WHITEOUTS, inode);
out:
	ovl_end_write(c->dentry);

/* …（同文件无关代码省略）… */
	struct ovl_fs *ofs = OVL_FS(c->dentry->d_sb);
	struct dentry *origin = c->lowerpath.dentry;
	struct ovl_fh *fh = NULL;
	bool to_index = false;

	/*
/* …（同文件无关代码省略）… */
		goto out_free_fh;
	} else {
		/*
		 * c->dentry->d_name is stabilzed by ovl_copy_up_start(),
		 * because if we got here, it means that c->dentry has no upper
		 * alias and changing ->d_name means going through ovl_rename()
		 * that will call ovl_copy_up() on source and target dentry.
		 */
		c->destname = c->dentry->d_name;
		/*
		 * Mark parent "impure" because it may now contain non-pure
		 * upper
/* …（同文件无关代码省略）… */
		err = ovl_set_impure(c->parent, c->destdir);
		ovl_end_write(c->dentry);
		if (err)
			goto out_free_fh;
	}

	/* Should we copyup with O_TMPFILE or with workdir? */
/* …（同文件无关代码省略）… */
out:
	if (to_index)
		kfree(c->destname.name);
out_free_fh:
	kfree(fh);
	return err;
/* …（同文件无关代码省略）… */
	return true;
}

static ssize_t ovl_getxattr_value(const struct path *path, char *name, char **value)
{
	ssize_t res;
	char *buf;

	res = ovl_do_getxattr(path, name, NULL, 0);
	if (res == -ENODATA || res == -EOPNOTSUPP)
		res = 0;

	if (res > 0) {
		buf = kzalloc(res, GFP_KERNEL);
		if (!buf)
			return -ENOMEM;

		res = ovl_do_getxattr(path, name, buf, res);
		if (res < 0)
			kfree(buf);
		else
			*value = buf;
	}
	return res;
}

/* …（同文件无关代码省略）… */
	struct path upperpath;
	int err;
	char *capability = NULL;
	ssize_t cap_size;

	ovl_path_upper(c->dentry, &upperpath);
	if (WARN_ON(upperpath.dentry == NULL))
		return -EIO;

	if (c->stat.size) {
		err = cap_size = ovl_getxattr_value(&upperpath, XATTR_NAME_CAPS,
						    &capability);
		if (cap_size < 0)
			goto out;
	}

/* …（同文件无关代码省略）… */
	ovl_clear_flag(OVL_VERIFIED_DIGEST, d_inode(c->dentry));
	ovl_set_upperdata(d_inode(c->dentry));
out_free:
	kfree(capability);
out:
	return err;
}
