// AUTO-DRAFT from torvalds/linux PR #4dce97c6c265544342e716e62fd24bbb232590c5
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
ext4_group_t ext4_get_group_number(struct super_block *sb,
				   ext4_fsblk_t block)
{
	ext4_group_t group;

	if (test_opt2(sb, STD_GROUP_SIZE))
		group = (block -
			 le32_to_cpu(EXT4_SB(sb)->s_es->s_first_data_block)) >>
			(EXT4_BLOCK_SIZE_BITS(sb) + EXT4_CLUSTER_BITS(sb) + 3);
	else
		ext4_get_group_no_and_offset(sb, block, &group, NULL);
	return group;
}
/* …（同文件无关代码省略）… */
void ext4_get_group_no_and_offset(struct super_block *sb, ext4_fsblk_t blocknr,
		ext4_group_t *blockgrpp, ext4_grpblk_t *offsetp)
{
	struct ext4_super_block *es = EXT4_SB(sb)->s_es;
	ext4_grpblk_t offset;

	blocknr = blocknr - le32_to_cpu(es->s_first_data_block);
	offset = do_div(blocknr, EXT4_BLOCKS_PER_GROUP(sb)) >>
		EXT4_SB(sb)->s_cluster_bits;
	if (offsetp)
		*offsetp = offset;
	if (blockgrpp)
		*blockgrpp = blocknr;

}
/* …（同文件无关代码省略）… */
static inline int ext4_block_in_group(struct super_block *sb,
				      ext4_fsblk_t block,
				      ext4_group_t block_group)
{
	ext4_group_t actual_group;

	actual_group = ext4_get_group_number(sb, block);
	return (actual_group == block_group) ? 1 : 0;
}
/* …（同文件无关代码省略）… */
static unsigned ext4_num_overhead_clusters(struct super_block *sb,
					   ext4_group_t block_group,
					   struct ext4_group_desc *gdp)
{
	unsigned base_clusters, num_clusters;
	int block_cluster = -1, inode_cluster;
	int itbl_cluster_start = -1, itbl_cluster_end = -1;
	ext4_fsblk_t start = ext4_group_first_block_no(sb, block_group);
	ext4_fsblk_t end = start + EXT4_BLOCKS_PER_GROUP(sb) - 1;
	ext4_fsblk_t itbl_blk_start, itbl_blk_end;
	struct ext4_sb_info *sbi = EXT4_SB(sb);

	/* This is the number of clusters used by the superblock,
	 * block group descriptors, and reserved block group
	 * descriptor blocks */
	base_clusters = ext4_num_base_meta_clusters(sb, block_group);
	num_clusters = base_clusters;

	/*
	 * Account and record inode table clusters if any cluster
	 * is in the block group, or inode table cluster range is
	 * [-1, -1] and won't overlap with block/inode bitmap cluster
	 * accounted below.
	 */
	itbl_blk_start = ext4_inode_table(sb, gdp);
	itbl_blk_end = itbl_blk_start + sbi->s_itb_per_group - 1;
	if (itbl_blk_start <= end && itbl_blk_end >= start) {
		itbl_blk_start = max(itbl_blk_start, start);
		itbl_blk_end = min(itbl_blk_end, end);

		itbl_cluster_start = EXT4_B2C(sbi, itbl_blk_start - start);
		itbl_cluster_end = EXT4_B2C(sbi, itbl_blk_end - start);

		num_clusters += itbl_cluster_end - itbl_cluster_start + 1;
		/* check if border cluster is overlapped */
		if (itbl_cluster_start == base_clusters - 1)
			num_clusters--;
	}

	/*
	 * For the allocation bitmaps, we first need to check to see
	 * if the block is in the block group.  If it is, then check
	 * to see if the cluster is already accounted for in the clusters
	 * used for the base metadata cluster and inode tables cluster.
	 * Normally all of these blocks are contiguous, so the special
	 * case handling shouldn't be necessary except for *very*
	 * unusual file system layouts.
	 */
	if (ext4_block_in_group(sb, ext4_block_bitmap(sb, gdp), block_group)) {
		block_cluster = EXT4_B2C(sbi,
					 ext4_block_bitmap(sb, gdp) - start);
		if (block_cluster >= base_clusters &&
		    (block_cluster < itbl_cluster_start ||
		    block_cluster > itbl_cluster_end))
			num_clusters++;
	}

	if (ext4_block_in_group(sb, ext4_inode_bitmap(sb, gdp), block_group)) {
		inode_cluster = EXT4_B2C(sbi,
					 ext4_inode_bitmap(sb, gdp) - start);
		/*
		 * Additional check if inode bitmap is in just accounted
		 * block_cluster
		 */
		if (inode_cluster != block_cluster &&
		    inode_cluster >= base_clusters &&
		    (inode_cluster < itbl_cluster_start ||
		    inode_cluster > itbl_cluster_end))
			num_clusters++;
	}

	return num_clusters;
}
/* …（同文件无关代码省略）… */
static unsigned int num_clusters_in_group(struct super_block *sb,
					  ext4_group_t block_group)
{
	unsigned int blocks;

	if (block_group == ext4_get_groups_count(sb) - 1) {
		/*
		 * Even though mke2fs always initializes the first and
		 * last group, just in case some other tool was used,
		 * we need to make sure we calculate the right free
		 * blocks.
		 */
		blocks = ext4_blocks_count(EXT4_SB(sb)->s_es) -
			ext4_group_first_block_no(sb, block_group);
	} else
		blocks = EXT4_BLOCKS_PER_GROUP(sb);
	return EXT4_NUM_B2C(EXT4_SB(sb), blocks);
}
/* …（同文件无关代码省略）… */
	return num_clusters_in_group(sb, block_group) -
		ext4_num_overhead_clusters(sb, block_group, gdp);
}

/*
 * The free blocks are managed by bitmaps.  A file system contains several
/* …（同文件无关代码省略）… */
					EXT4_GROUP_INFO_BBITMAP_CORRUPT);
		return ERR_PTR(-EFSCORRUPTED);
	}
	bh = sb_getblk(sb, bitmap_blk);
	if (unlikely(!bh)) {
		ext4_warning(sb, "Cannot get buffer for block bitmap - "
			     "block_group = %u, block_bitmap = %llu",
/* …（同文件无关代码省略）… */
	}
	return bh;
}

/**
 * ext4_has_free_clusters()
/* …（同文件无关代码省略）… */
	} else
		return -ENOSPC;
}

/**
 * ext4_should_retry_alloc() - check if a block allocation should be retried
/* …（同文件无关代码省略）… */
	(void) jbd2_journal_force_commit_nested(sbi->s_journal);
	return 1;
}

/*
 * ext4_new_meta_blocks() -- allocate block for meta data (indexing) blocks
/* …（同文件无关代码省略）… */
	}
	return ret;
}

/**
 * ext4_count_free_clusters() -- count filesystem free clusters
/* …（同文件无关代码省略）… */
static inline int test_root(ext4_group_t a, int b)
{
	while (1) {
		if (a < b)
			return 0;
		if (a == b)
			return 1;
		if ((a % b) != 0)
			return 0;
		a = a / b;
	}
}
/* …（同文件无关代码省略）… */
int ext4_bg_has_super(struct super_block *sb, ext4_group_t group)
{
	struct ext4_super_block *es = EXT4_SB(sb)->s_es;

	if (group == 0)
		return 1;
	if (ext4_has_feature_sparse_super2(sb)) {
		if (group == le32_to_cpu(es->s_backup_bgs[0]) ||
		    group == le32_to_cpu(es->s_backup_bgs[1]))
			return 1;
		return 0;
	}
	if ((group <= 1) || !ext4_has_feature_sparse_super(sb))
		return 1;
	if (!(group & 1))
		return 0;
	if (test_root(group, 3) || (tes
