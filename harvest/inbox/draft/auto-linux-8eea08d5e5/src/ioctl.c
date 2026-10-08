// AUTO-DRAFT from torvalds/linux PR #7b63ef2d55f24519e7e9e5f4d15dbea03f126e40
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
struct btrfs_uring_priv {
	struct io_uring_cmd *cmd;
	struct page **pages;
	unsigned long nr_pages;
	struct kiocb iocb;
	struct iovec *iov;
	struct iov_iter iter;
	struct extent_state *cached_state;
	u64 count;
	u64 start;
	u64 lockend;
	int err;
	bool compressed;
};
/* …（同文件无关代码省略）… */
	size_t page_offset;
	ssize_t ret;

	/* The inode lock has already been acquired in btrfs_uring_read_extent.  */
	btrfs_lockdep_inode_acquire(inode, i_rwsem);

	if (priv->err) {
/* …（同文件无关代码省略）… */
				   struct iovec *iov, struct io_uring_cmd *cmd)
{
	struct btrfs_inode *inode = BTRFS_I(file_inode(iocb->ki_filp));
	struct extent_io_tree *io_tree = &inode->io_tree;
	struct page **pages = NULL;
	struct btrfs_uring_priv *priv = NULL;
	unsigned long nr_pages;
/* …（同文件无关代码省略）… */
	return -EIOCBQUEUED;

out_fail:
	btrfs_unlock_extent(io_tree, start, lockend, &cached_state);
	btrfs_inode_unlock(inode, BTRFS_ILOCK_SHARED);
	kfree(priv);
	for (int i = 0; i < nr_pages; i++) {
		if (pages[i])
/* …（同文件无关代码省略）… */
	ret = btrfs_encoded_read(&kiocb, &data->iter, &data->args, &cached_state,
				 &disk_bytenr, &disk_io_size);
	if (ret == -EAGAIN)
		goto out_acct;
	if (ret < 0 && ret != -EIOCBQUEUED)
		goto out_free;

/* …（同文件无关代码省略）… */
					      cached_state, disk_bytenr, disk_io_size,
					      count, data->args.compression,
					      data->iov, cmd);

		goto out_acct;
	}

out_free:
/* …（同文件无关代码省略）… */
		add_rchar(current, ret);
	inc_syscr(current);

	if (ret != -EIOCBQUEUED && ret != -EAGAIN)
		kfree(data);

	return ret;
}
/* …（同文件无关代码省略）… */
		goto out_acct;
	}

	if (!data) {
		data = kzalloc_obj(*data, GFP_NOFS);
		if (!data) {
/* …（同文件无关代码省略）… */
		}
	}

	if (issue_flags & IO_URING_F_NONBLOCK) {
		ret = -EAGAIN;
		goto out_acct;
	}

	pos = data->args.offset;
	ret = rw_verify_area(WRITE, file, &pos, data->args.len);
	if (ret < 0)
/* …（同文件无关代码省略）… */
		add_wchar(current, ret);
	inc_syscw(current);

	if (ret != -EAGAIN)
		kfree(data);
	return ret;
}
