// AUTO-DRAFT from torvalds/linux PR #0ff8f40110872ecd3a9918e56ddd0ff1b46fb10e
#include "debug.h"
#include "ntfs.h"
#include "ntfs_fs.h"

/*
 * cifx, btrfs, exfat, ext4, f2fs use this constant.
/* …（同文件无关代码省略）… */
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (copy_from_user(user, buf, FSLABEL_MAX))
		return -EFAULT;

/* …（同文件无关代码省略）… */
static int check_read_restriction(struct inode *inode)
{
	struct ntfs_inode *ni = ntfs_i(inode);

	/* Avoid any operation if inode is bad. */
	if (unlikely(is_bad_ni(ni)))
		return -EINVAL;

	if (unlikely(ntfs3_forced_shutdown(inode->i_sb)))
		return -EIO;

	if (is_encrypted(ni)) {
		ntfs_inode_warn(inode, "encrypted i/o not supported");
		return -EOPNOTSUPP;
	}

#ifndef CONFIG_NTFS3_LZX_XPRESS
	if (ni->ni_flags & NI_FLAG_COMPRESSED_MASK) {
		ntfs_inode_warn(
			inode,
			"activate CONFIG_NTFS3_LZX_XPRESS to read external compressed files");
		return -EOPNOTSUPP;
	}
#endif

	if (is_dedup(ni)) {
		ntfs_inode_warn(inode, "read deduplicated not supported");
		return -EOPNOTSUPP;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
	unsigned int dio_flags;
	ssize_t ret;

	ret = check_read_restriction(inode);
	if (ret)
		return ret;
/* …（同文件无关代码省略）… */
	loff_t vbo, endbyte;
	ssize_t ret, err;

	if (!inode_trylock(inode)) {
		if (iocb->ki_flags & IOCB_NOWAIT)
			return -EAGAIN;
