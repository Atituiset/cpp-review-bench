// AUTO-DRAFT from torvalds/linux PR #51628610f9187c1c1cfe499d92e066bc22ad9693
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <time.h>
  // <<< BUG ANCHOR
			dir = d_inode_rcu(parent);
			if (!dir)
				return -ECHILD;
			/*
			 * Nothing pins d in RCU walk: eviction of the overlay
			 * inode may drop the last reference at any time.
			 */
			if (!try_take_dentry_name_snapshot(&n, d))
				return -ECHILD;
		} else {
			parent = dget_parent(d);
			dir = d_inode(parent);
			take_dentry_name_snapshot(&n, d);
		}
		ret = d->d_op->d_revalidate(dir, &n.name, d, flags);
		release_dentry_name_snapshot(&n);
		if (!(flags & LOOKUP_RCU))
