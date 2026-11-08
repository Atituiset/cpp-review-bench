// AUTO-DRAFT from torvalds/linux PR #fa4ad798f6f681031775266044ba1b0cf3c670bd
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>
  // <<< BUG ANCHOR
		goto cleanup;
	}

	if (cur >= i_size_read(&inode->vfs_inode)) {
		iput(&inode->vfs_inode);
		goto cleanup;
	}

	/* Do a chunk of defrag */
	clear_bit(BTRFS_INODE_IN_DEFRAG, &inode->runtime_flags);
	memset(&range, 0, sizeof(range));
	range.len = (u64)-1;
	range.start = cur;
