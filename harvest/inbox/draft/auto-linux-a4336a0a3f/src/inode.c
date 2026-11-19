// AUTO-DRAFT from torvalds/linux PR #d866d5bafc5e1417032255002e95f8ec3121754b
static void hugetlbfs_inc_free_inodes(struct hugetlbfs_sb_info *sbinfo)
{
	if (sbinfo->free_inodes >= 0) {
		spin_lock(&sbinfo->stat_lock);
		sbinfo->free_inodes++;
		spin_unlock(&sbinfo->stat_lock);
	}
}
/* …（同文件无关代码省略）… */
  // <<< BUG ANCHOR
static void hugetlbfs_free_inode(struct inode *inode)
{
	trace_hugetlbfs_free_inode(inode);
	kmem_cache_free(hugetlbfs_inode_cachep, HUGETLBFS_I(inode));
}

static void hugetlbfs_destroy_inode(struct inode *inode)
{
	hugetlbfs_inc_free_inodes(HUGETLBFS_SB(inode->i_sb));
}
