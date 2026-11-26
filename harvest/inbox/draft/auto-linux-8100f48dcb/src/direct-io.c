// AUTO-DRAFT from torvalds/linux PR #40212b29a6f718d4e7fb3275cede181cb8e137cf
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdio.h>
  // <<< BUG ANCHOR
	 * If the write DIO is within EOF, use a shared lock and also only if
	 * security bits will likely not be dropped by file_remove_privs() called
	 * from btrfs_write_check(). Either will need to be rechecked after the
	 * lock was acquired.
	 */
	if (iocb->ki_pos + iov_iter_count(from) <= i_size_read(inode) && IS_NOSEC(inode))
		ilock_flags |= BTRFS_ILOCK_SHARED;

relock:
/* …（同文件无关代码省略）… */
		goto relock;
	}

	ret = generic_write_checks(iocb, from);
	if (ret <= 0) {
		btrfs_inode_unlock(BTRFS_I(inode), ilock_flags);
