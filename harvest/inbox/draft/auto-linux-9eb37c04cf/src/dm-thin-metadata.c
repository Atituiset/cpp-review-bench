// AUTO-DRAFT from torvalds/linux PR #87aecd24f58e15256870befed777bbffc5d4a9d5
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
  // <<< BUG ANCHOR
#define SPACE_MAP_ROOT_SIZE 128
/* …（同文件无关代码省略）… */
struct dm_pool_metadata {
	struct hlist_node hash;

	struct block_device *bdev;
	struct dm_block_manager *bm;
	struct dm_space_map *metadata_sm;
	struct dm_space_map *data_sm;
	struct dm_transaction_manager *tm;
	struct dm_transaction_manager *nb_tm;

	/*
	 * Two-level btree.
	 * First level holds thin_dev_t.
	 * Second level holds mappings.
	 */
	struct dm_btree_info info;

	/*
	 * Non-blocking version of the above.
	 */
	struct dm_btree_info nb_info;

	/*
	 * Just the top level for deleting whole devices.
	 */
	struct dm_btree_info tl_info;

	/*
	 * Just the bottom level for creating new devices.
	 */
	struct dm_btree_info bl_info;

	/*
	 * Describes the device details btree.
	 */
	struct dm_btree_info details_info;

	struct rw_semaphore root_lock;
	uint32_t time;
	dm_block_t root;
	dm_block_t details_root;
	dm_block_t held_root;
	struct list_head thin_devices;
	uint64_t trans_id;
	unsigned long flags;
	sector_t data_block_size;

	/*
	 * Pre-commit callback.
	 *
	 * This allows the thin provisioning target to run a callback before
	 * the metadata are committed.
	 */
	dm_pool_pre_commit_fn pre_commit_fn;
	void *pre_commit_context;

	/*
	 * We reserve a section of the metadata for commit overhead.
	 * All reported space does *not* include this.
	 */
	dm_block_t metadata_reserve;

	/*
	 * Set if a transaction has to be aborted but the attempt to roll back
	 * to the previous (good) transaction failed.  The only pool metadata
	 * operation possible in this state is the closing of the device.
	 */
	bool fail_io:1;

	/*
	 * Set once a thin-pool has been accessed through one of the interfaces
	 * that imply the pool is in-service (e.g. thin devices created/deleted,
	 * thin-pool message, metadata snapshots, etc).
	 */
	bool in_service:1;

	/*
	 * Reading the space map roots can fail, so we read it into these
	 * buffers before the superblock is locked and updated.
	 */
	__u8 data_space_map_root[SPACE_MAP_ROOT_SIZE];
	__u8 metadata_space_map_root[SPACE_MAP_ROOT_SIZE];
};
/* …（同文件无关代码省略）… */
struct dm_thin_device {
	struct list_head list;
	struct dm_pool_metadata *pmd;
	dm_thin_id id;

	int open_count;
	bool changed:1;
	bool aborted_with_changes:1;
	uint64_t mapped_blocks;
	uint64_t transaction_id;
	uint32_t creation_time;
	uint32_t snapshotted_time;
};
/* …（同文件无关代码省略）… */
static void __close_device(struct dm_thin_device *td)
{
	--td->open_count;
}
/* …（同文件无关代码省略）… */
	__close_device(td);

	if (r)
		goto bad;

	return 0;

bad:
	dm_btree_remove(&pmd->tl_info, pmd->root, &key, &pmd->root);
	dm_btree_remove(&pmd->details_info, pmd->details_root,
