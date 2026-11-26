// AUTO-DRAFT from torvalds/linux PR #40212b29a6f718d4e7fb3275cede181cb8e137cf
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define BTRFS_DELAYED_BACKGROUND	128
#define BTRFS_DELAYED_BATCH		16
/* …（同文件无关代码省略）… */
static void finish_one_item(struct btrfs_delayed_root *delayed_root)
{
	int seq = atomic_inc_return(&delayed_root->items_seq);

	/* atomic_dec_return implies a barrier */
	if ((atomic_dec_return(&delayed_root->items) <
	    BTRFS_DELAYED_BACKGROUND || seq % BTRFS_DELAYED_BATCH == 0))
		cond_wake_up_nomb(&delayed_root->wait);
}
/* …（同文件无关代码省略）… */
static void btrfs_delayed_inode_release_metadata(struct btrfs_fs_info *fs_info,
						struct btrfs_delayed_node *node,
						bool qgroup_free)
{
	struct btrfs_block_rsv *rsv;

	if (!node->bytes_reserved)
		return;

	rsv = &fs_info->delayed_block_rsv;
	trace_btrfs_space_reservation(fs_info, "delayed_inode",
				      node->inode_id, node->bytes_reserved, 0);
	btrfs_block_rsv_release(fs_info, rsv, node->bytes_reserved, NULL);
	if (qgroup_free)
		btrfs_qgroup_free_meta_prealloc(node->root,
				node->bytes_reserved);
	else
		btrfs_qgroup_convert_reserved_meta(node->root,
				node->bytes_reserved);
	node->bytes_reserved = 0;
}
/* …（同文件无关代码省略）… */
static void btrfs_release_delayed_inode(struct btrfs_delayed_node *delayed_node)
{
	if (delayed_node &&
	    test_bit(BTRFS_DELAYED_NODE_INODE_DIRTY, &delayed_node->flags)) {
		ASSERT(delayed_node->root);
		clear_bit(BTRFS_DELAYED_NODE_INODE_DIRTY, &delayed_node->flags);
		delayed_node->count--;
		finish_one_item(&delayed_node->root->fs_info->delayed_root);
	}
}
/* …（同文件无关代码省略）… */
static void btrfs_release_delayed_iref(struct btrfs_delayed_node *delayed_node)
{
	if (test_and_clear_bit(BTRFS_DELAYED_NODE_DEL_IREF, &delayed_node->flags)) {
		ASSERT(delayed_node->root);
		delayed_node->count--;
		finish_one_item(&delayed_node->root->fs_info->delayed_root);
	}
}
/* …（同文件无关代码省略）… */
		ret = btrfs_search_slot(trans, root, &key, path, -1, 1);
		if (unlikely(ret < 0)) {
			btrfs_abort_transaction(trans, ret);
			goto err_out;
		}
		ASSERT(ret > 0);
		ASSERT(path->slots[0] > 0);
/* …（同文件无关代码省略）… */
out:
	btrfs_release_delayed_iref(node);
	btrfs_release_path(path);
err_out:
	btrfs_delayed_inode_release_metadata(fs_info, node, (ret < 0));
	btrfs_release_delayed_inode(node);
	return ret;
