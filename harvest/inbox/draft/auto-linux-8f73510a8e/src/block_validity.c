// AUTO-DRAFT from torvalds/linux PR #4dce97c6c265544342e716e62fd24bbb232590c5
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include <linux/swap.h>
#include <linux/pagemap.h>
#include <linux/blkdev.h>
#include <linux/slab.h>
#include "ext4.h"

/* …（同文件无关代码省略）… */
static void release_system_zone(struct ext4_system_blocks *system_blks)
{
	struct ext4_system_zone	*entry, *n;

	rbtree_postorder_for_each_entry_safe(entry, n,
				&system_blks->root, node)
		kmem_cache_free(ext4_system_zone_cachep, entry);
}
/* …（同文件无关代码省略）… */
	struct rb_node **n = &system_blks->root.rb_node, *node;
	struct rb_node *parent = NULL, *new_node;

	while (*n) {
		parent = *n;
		entry = rb_entry(parent, struct ext4_system_zone, node);
		if (start_blk < entry->start_blk)
			n = &(*n)->rb_left;
		else if (start_blk >= (entry->start_blk + entry->count))
			n = &(*n)->rb_right;
/* …（同文件无关代码省略）… */
static void ext4_destroy_system_zone(struct rcu_head *rcu)
{
	struct ext4_system_blocks *system_blks;

	system_blks = container_of(rcu, struct ext4_system_blocks, rcu);
	release_system_zone(system_blks);
	kfree(system_blks);
}
/* …（同文件无关代码省略）… */
	kfree(system_blks);
	return ret;
}

/*
 * Called when the filesystem is unmounted or when remounting it with
/* …（同文件无关代码省略）… */
	if (system_blks)
		call_rcu(&system_blks->rcu, ext4_destroy_system_zone);
}

int ext4_sb_block_valid(struct super_block *sb, struct inode *inode,
				ext4_fsblk_t start_blk, unsigned int count)
{
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	struct ext4_system_blocks *system_blks;
	struct ext4_system_zone *entry;
	struct rb_node *n;
	int ret = 1;

	if ((start_blk <= le32_to_cpu(sbi->s_es->s_first_data_block)) ||
	    (start_blk + count < start_blk) ||
	    (start_blk + count > ext4_blocks_count(sbi->s_es)))
		return 0;

	/*
	 * Lock the system zone to prevent it being released concurrently
	 * when doing a remount which inverse current "[no]block_validity"
	 * mount option.
	 */
	rcu_read_lock();
	system_blks = rcu_dereference(sbi->s_system_blks);
	if (system_blks == NULL)
		goto out_rcu;

	n = system_blks->root.rb_node;
	while (n) {
		entry = rb_entry(n, struct ext4_system_zone, node);
		if (start_blk + count - 1 < entry->start_blk)
			n = n->rb_left;
		else if (start_blk >= (entry->start_blk + entry->count))
			n = n->rb_right;
		else {
			ret = 0;
			if (inode)
				ret = (entry->ino == inode->i_ino);
			break;
		}
	}
out_rcu:
	rcu_read_unlock();
	return ret;
}

/*
 * Returns 1 if the passed-in block region (start_blk,
/* …（同文件无关代码省略）… */
int ext4_inode_block_valid(struct inode *inode, ext4_fsblk_t start_blk,
			  unsigned int count)
{
	return ext4_sb_block_valid(inode->i_sb, inode, start_blk, count);
}

int ext4_check_blockref(const char *function, unsigned int line,
			struct inode *inode, __le32 *p, unsigned int max)
{
	__le32 *bref = p;
	unsigned int blk;
	journal_t *journal = EXT4_SB(inode->i_sb)->s_journal;

	if (journal && inode == journal->j_inode)
		return 0;

	while (bref < p+max) {
		blk = le32_to_cpu(*bref++);
		if (blk &&
		    unlikely(!ext4_inode_block_valid(inode, blk, 1))) {
			ext4_error_inode(inode, function, line, blk,
					 "invalid block");
			return -EFSCORRUPTED;
		}
	}
	return 0;
}
