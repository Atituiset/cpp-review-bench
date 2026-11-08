// AUTO-DRAFT from torvalds/linux PR #fa4ad798f6f681031775266044ba1b0cf3c670bd
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

static void free_root_extent_buffers(struct btrfs_root *root)
{
	if (root) {
		free_extent_buffer(root->node);
		free_extent_buffer(root->commit_root);
		root->node = NULL;
		root->commit_root = NULL;
	}
}
/* …（同文件无关代码省略）… */
		if (root->anon_dev)
			free_anon_bdev(root->anon_dev);
		free_root_extent_buffers(root);
#ifdef CONFIG_BTRFS_DEBUG
		spin_lock(&root->fs_info->fs_roots_radix_lock);
		list_del_init(&root->leak_list);
/* …（同文件无关代码省略）… */
	struct btrfs_root *gang[8];
	int i;

	while (!list_empty(&fs_info->dead_roots)) {
		gang[0] = list_first_entry(&fs_info->dead_roots,
					   struct btrfs_root, root_list);
