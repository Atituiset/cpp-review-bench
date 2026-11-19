// AUTO-DRAFT from torvalds/linux PR #4405c18c95d2d0f99f6c581ef0ee8591a1aa6ccd
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
static int ocfs2_truncate_for_delete(struct ocfs2_super *osb,
				     struct inode *inode,
				     struct buffer_head *fe_bh)
{
	int status = 0;
	struct ocfs2_dinode *fe;
	handle_t *handle = NULL;

	fe = (struct ocfs2_dinode *) fe_bh->b_data;

	/*
	 * This check will also skip truncate of inodes with inline
	 * data and fast symlinks.
	 */
	if (fe->i_clusters) {
		if (ocfs2_should_order_data(inode))
			ocfs2_begin_ordered_truncate(inode, 0);

		handle = ocfs2_start_trans(osb, OCFS2_INODE_UPDATE_CREDITS);
		if (IS_ERR(handle)) {
			status = PTR_ERR(handle);
			handle = NULL;
			mlog_errno(status);
			goto out;
		}

		status = ocfs2_journal_access_di(handle, INODE_CACHE(inode),
						 fe_bh,
						 OCFS2_JOURNAL_ACCESS_WRITE);
		if (status < 0) {
			mlog_errno(status);
			goto out;
		}

		i_size_write(inode, 0);

		status = ocfs2_mark_inode_dirty(handle, inode, fe_bh);
		if (status < 0) {
			mlog_errno(status);
			goto out;
		}

		ocfs2_commit_trans(osb, handle);
		handle = NULL;

		status = ocfs2_commit_truncate(osb, inode, fe_bh);
		if (status < 0)
			mlog_errno(status);
	}

out:
	if (handle)
		ocfs2_commit_trans(osb, handle);
	return status;
}
/* …（同文件无关代码省略）… */
static int ocfs2_remove_inode(struct inode *inode,
			      struct buffer_head *di_bh,
			      struct inode *orphan_dir_inode,
			      struct buffer_head *orphan_dir_bh)
{
	int status;
	struct inode *inode_alloc_inode = NULL;
	struct buffer_head *inode_alloc_bh = NULL;
	handle_t *handle;
	struct ocfs2_super *osb = OCFS2_SB(inode->i_sb);
	struct ocfs2_dinode *di = (struct ocfs2_dinode *) di_bh->b_data;

	inode_alloc_inode =
		ocfs2_get_system_file_inode(osb, INODE_ALLOC_SYSTEM_INODE,
					    le16_to_cpu(di->i_suballoc_slot));
	if (!inode_alloc_inode) {
		status = -ENOENT;
		mlog_errno(status);
		goto bail;
	}

	inode_lock(inode_alloc_inode);
	status = ocfs2_inode_lock(inode_alloc_inode, &inode_alloc_bh, 1);
	if (status < 0) {
		inode_unlock(inode_alloc_inode);

		mlog_errno(status);
		goto bail;
	}

	handle = ocfs2_start_trans(osb, OCFS2_DELETE_INODE_CREDITS +
				   ocfs2_quota_trans_credits(inode->i_sb));
	if (IS_ERR(handle)) {
		status = PTR_ERR(handle);
		mlog_errno(status);
		goto bail_unlock;
	}

	if (!(OCFS2_I(inode)->ip_flags & OCFS2_INODE_SKIP_ORPHAN_DIR)) {
		status = ocfs2_orphan_del(osb, handle, orphan_dir_inode, inode,
					  orphan_dir_bh, false);
		if (status < 0) {
			mlog_errno(status);
			goto bail_commit;
		}
	}

	/* set the inodes dtime */
	status = ocfs2_journal_access_di(handle, INODE_CACHE(inode), di_bh,
					 OCFS2_JOURNAL_ACCESS_WRITE);
	if (status < 0) {
		mlog_errno(status);
		goto bail_commit;
	}

	di->i_dtime = cpu_to_le64(ktime_get_real_seconds());
	di->i_flags &= cpu_to_le32(~(OCFS2_VALID_FL | OCFS2_ORPHANED_FL));
	ocfs2_journal_dirty(handle, di_bh);

	ocfs2_remove_from_cache(INODE_CACHE(inode), di_bh);
	dquot_free_inode(inode);

	status = ocfs2_free_dinode(handle, inode_alloc_inode,
				   inode_alloc_bh, di);
	if (status < 0)
		mlog_errno(status);

bail_commit:
	ocfs2_commit_trans(osb, handle);
bail_unlock:
	ocfs2_inode_unlock(inode_alloc_inode, 1);
	inode_unlock(inode_alloc_inode);
	brelse(inode_alloc_bh);
bail:
	iput(inode_alloc_inode);

	return status;
}
/* …（同文件无关代码省略）… */
static int ocfs2_check_orphan_recovery_state(struct ocfs2_super *osb,
					     int slot)
{
	int ret = 0;

	spin_lock(&osb->osb_lock);
	if (ocfs2_node_map_test_bit(osb, &osb->osb_recovering_orphan_dirs, slot)) {
		ret = -EDEADLK;
		goto out;
	}
	/* This signals to the orphan recovery process that it should
	 * wait for us to handle the wipe. */
	osb->osb_orphan_wipes[slot]++;
out:
	spin_unlock(&osb->osb_lock);
	trace_ocfs2_check_orphan_recovery_state(slot, ret);
	return ret;
}
/* …（同文件无关代码省略）… */
static void ocfs2_signal_wipe_completion(struct ocfs2_super *osb,
					 int slot)
{
	spin_lock(&osb->osb_lock);
	osb->osb_orphan_wipes[slot]--;
	spin_unlock(&osb->osb_lock);

	wake_up(&osb->osb_wipe_event);
}
/* …（同文件无关代码省略）… */
static int ocfs2_wipe_inode(struct inode *inode,
			    struct buffer_head *di_bh)
{
	int status, orphaned_slot = -1;
	struct inode *orphan_dir_inode = NULL;
	struct buffer_head *orphan_dir_bh = NULL;
	struct ocfs2_super *osb = OCFS2_SB(inode->i_sb);
	struct ocfs2_dinode *di = (struct ocfs2_dinode *) di_bh->b_data;

	if (!(OCFS2_I(inode)->ip_flags & OCFS2_INODE_SKIP_ORPHAN_DIR)) {
		orphaned_slot = le16_to_cpu(di->i_orphaned_slot);

		status = ocfs2_check_orphan_recovery_state(osb, orphaned_slot);
		if (status)
			return status;

		orphan_dir_inode = ocfs2_get_system_file_inode(osb,
							       ORPHAN_DIR_SYSTEM_INODE,
							       orphaned_slot);
		if (!orphan_dir_inode) {
			status = -ENOENT;
			mlog_errno(status);
			goto bail;
		}

		/* Lock the orphan dir. The lock will be held for the entire
		 * delete_inode operation. We do this now to avoid races with
		 * recovery completion on other nodes. */
		inode_lock(orphan_dir_inode);
		status = ocfs2_inode_lock(orphan_dir_inode, &orphan_dir_bh, 1);
		if (status < 0) {
			inode_unlock(orphan_dir_inode);

			mlog_errno(status);
			goto bail;
		}
	}

	/* we do this while holding the orphan dir lock because we
	 * don't want recovery being run from another node to try an
	 * inode delete underneath us -- this will result in two nodes
	 * truncating the same file! */
	status = ocfs2_truncate_for_delete(osb, inode, di_bh);
	if (status < 0) {
		mlog_errno(status);
		goto bail_unlock_dir;
	}

	/* Remove any dir index tree */
	if (S_ISDIR(inode->i_mode)) {
		status = ocfs2_dx_dir_truncate(inode, di_bh);
		if (status) {
			mlog_errno(status);
			goto bail_unlock_dir;
		}
	}

	/*Free extended attribute resources associated with this inode.*/
	status = ocfs2_xattr_remove(inode, di_bh);
	if (status < 0) {
		mlog_errno(status);
		goto bail_unlock_dir;
	}

	status = ocfs2_remove_refcount_tree(inode, di_bh);
	if (status < 0) {
		mlog_errno(status);
		goto bail_unlock_dir;
	}

	status = ocfs2_remove_inode(inode, di_bh, orphan_dir_inode,
				    orphan_dir_bh);
	if (status < 0)
		mlog_errno(status);

bail_unlock_dir:
	if (OCFS2_I(inode)->ip_flags & OCFS2_INODE_SKIP_ORPHAN_DIR)
		return status;

	ocfs2_inode_unlock(orphan_dir_inode, 1);
	inode_unlock(orphan_dir_inode);
	brelse(orphan_dir_bh);
bail:
	iput(orphan_dir_inode);
	ocfs2_signal_wipe_completion(osb, orphaned_slot);

	return status;
}
/* …（同文件无关代码省略）… */
static int ocfs2_inode_is_valid_to_delete(struct inode *inode)
{
	int ret = 0;
	struct ocfs2_inode_info *oi = OCFS2_I(inode);
	struct ocfs2_super *osb = OCFS2_SB(inode->i_sb);

	trace_ocfs2_inode_is_valid_to_delete(current, osb->dc_task,
					     (unsigned long long)oi->ip_blkno,
					     oi->ip_flags);

	/* We shouldn't be getting here for the root directory
	 * inode.. */
	if (inode == osb->root_inode) {
		mlog(ML_ERROR, "Skipping delete of root inode.\n");
		goto bail;
	}

	/*
	 * If we're coming from downconvert_thread we can't go into our own
	 * voting [hello, deadlock city!] so we cannot delete the inode. But
	 * since we dropped last inode ref when downconverting dentry lock,
	 * we cannot have the file open and thus the node doing unlink will
	 * take care of deleting the inode.
	 */
	if (current == osb->dc_task)
		goto bail;

	spin_lock(&oi->ip_lock);
	/* OCFS2 *never* deletes system files. This should technically
	 * never get here as system file inodes should always have a
	 * positive link count. */
	if (oi->ip_flags & OCFS2_INODE_SYSTEM_FILE) {
		mlog(ML_ERROR, "Skipping delete of system file %llu\n",
		     (unsigned long long)oi->ip_blkno);
		goto bail_unlock;
	}

	ret = 1;
bail_unlock:
	spin_unlock(&oi->ip_lock);
bail:
	return ret;
}
/* …（同文件无关代码省略）… */
static int ocfs2_query_inode_wipe(struct inode *inode,
				  struct buffer_head *di_bh,
				  int *wipe)
{
	int status = 0, reason = 0;
	struct ocfs2_inode_info *oi = OCFS2_I(inode);
	struct ocfs2_dinode *di;

	*wipe = 0;

	trace_ocfs2_query_inode_wipe_begin((unsigned long long)oi->ip_blkno,
					   inode->i_nlink);

	/* While we were waiting for the cluster lock in
	 * ocfs2_delete_inode, another node might have asked to delete
	 * the inode. Recheck our flags to catch this. */
	if (!ocfs2_inode_is_valid_to_delete(inode)) {
		reason = 1;
		goto bail;
	}

	/* Now that we have an up to date inode, we can double check
	 * the link count. */
	if (inode->i_nlink)
		goto bail;

	/* Do some basic inode verification... */
	di = (struct ocfs2_dinode *) di_bh->b_data;
	if (!(di->i_flags & cpu_to_le32(OCFS2_ORPHANED_FL)) &&
	    !(oi->ip_flags & OCFS2_INODE_SKIP_ORPHAN_DIR)) {
		/*
		 * Inodes in the orphan dir must have ORPHANED_FL.  The only
		 * inodes that come back out of the orphan dir are reflink
		 * targets. A reflink target may be moved out of the orphan
		 * dir between the time we scan the directory and the time we
		 * process it. This would lead to HAS_REFCOUNT_FL being set but
		 * ORPHANED_FL not.
		 */
		if (di->i_dyn_features & cpu_to_le16(OCFS2_HAS_REFCOUNT_FL)) {
			reason = 2;
			goto bail;
		}

		/* for lack of a better error? */
		status = -EEXIST;
		mlog(ML_ERROR,
		     "Inode %llu (on-disk %llu) not orphaned! "
		     "Disk flags  0x%x, inode flags 0x%x\n",
		     (unsigned long long)oi->ip_blkno,
		     (unsigned long long)le64_to_cpu(di->i_blkno),
		     le32_to_cpu(di->i_flags), oi->ip_flags);
		goto bail;
	}

	/* has someone already deleted us?! baaad... */
	if (di->i_dtime) {
		status = -EEXIST;
		mlog_errno(status);
		goto bail;
	}

	/*
	 * This is how ocfs2 determines whether an inode is still live
	 * within the cluster. Every node takes a shared read lock on
	 * the inode open lock in ocfs2_read_locked_inode(). When we
	 * get to ->delete_inode(), each node tries to convert it's
	 * lock to an exclusive. Trylocks are serialized by the inode
	 * meta data lock. If the upconvert succeeds, we know the inode
	 * is no longer live and can be deleted.
	 *
	 * Though we call this with the meta data lock held, the
	 * trylock keeps us from ABBA deadlock.
	 */
	status = ocfs2_try_open_lock(inode, 1);
	if (status == -EAGAIN) {
		status = 0;
		reason = 3;
		goto bail;
	}
	if (status < 0) {
		mlog_errno(status);
		goto bail;
	}

	*wipe = 1;
	trace_ocfs2_query_inode_wipe_succ(le16_to_cpu(di->i_orphaned_slot));

bail:
	trace_ocfs2_query_inode_wipe_end(status, reason);
	return status;
}
/* …（同文件无关代码省略）… */
static void ocfs2_cleanup_delete_inode(struct inode *inode,
				       int sync_data)
{
	trace_ocfs2_cleanup_delete_inode(
		(unsigned long long)OCFS2_I(inode)->ip_blkno, sync_data);
	if (sync_data)
		filemap_write_and_wait(inode->i_mapping);
	truncate_inode_pages_final(&inode->i_data);
}
/* …（同文件无关代码省略）… */
static void ocfs2_delete_inode(struct inode *inode)
{
	int wipe, status;
	sigset_t oldset;
	unsigned int nofs_flag = 0;
	struct buffer_head *di_bh = NULL;
	struct ocfs2_dinode *di = NULL;

	trace_ocfs2_delete_inode(inode->i_ino,
				 (unsigned long long)OCFS2_I(inode)->ip_blkno,
				 is_bad_inode(inode));

	/* When we fail in read_inode() we mark inode as bad. The second test
	 * catches the case when inode allocation fails before allocating
	 * a block for inode. */
	if (is_bad_inode(inode) || !OCFS2_I(inode)->ip_blkno)
		goto bail;

	if (!ocfs2_inode_is_valid_to_delete(inode)) {
		/* It's probably not necessary to truncate_inode_pages
		 * here but we do it for safety anyway (it will most
		 * likely be a no-op anyway) */
		ocfs2_cleanup_delete_inode(inode, 0);
		goto bail;
	}

	dquot_initialize(inode);

	/* We want to block signals in delete_inode as the lock and
	 * messaging paths may return us -ERESTARTSYS. Which would
	 * cause us to exit early, resulting in inodes being orphaned
	 * forever. */
	ocfs2_block_signals(&oldset);

	/*
	 * Synchronize us against ocfs2_get_dentry. We take this in
	 * shared mode so that all nodes can still concurrently
	 * process deletes.
	 */
	status = ocfs2_nfs_sync_lock(OCFS2_SB(inode->i_sb), 0, &nofs_flag);
	if (status < 0) {
		mlog(ML_ERROR, "getting nfs sync lock(PR) failed %d\n", status);
		ocfs2_cleanup_delete_inode(inode, 0);
		goto bail_unblock;
	}
	/* Lock down the inode. This gives us an up to date view of
	 * it's metadata (for verification), and allows us to
	 * serialize delete_inode on multiple nodes.
	 *
	 * Even though we might be doing a truncate, we don't take the
	 * allocation lock here as it won't be needed - nobody will
	 * have the file open.
	 */
	status = ocfs2_inode_lock(inode, &di_bh, 1);
	if (status < 0) {
		if (status != -ENOENT)
			mlog_errno(status);
		ocfs2_cleanup_delete_inode(inode, 0);
		goto bail_unlock_nfs_sync;
	}

	di = (struct ocfs2_dinode *)di_bh->b_data;
	/* Skip inode deletion and wait for dio orphan entry recovered
	 * first */
	if (unlikely(di->i_flags & cpu_to_le32(OCFS2_DIO_ORPHANED_FL))) {
		ocfs2_cleanup_delete_inode(inode, 0);
		goto bail_unlock_inode;
	}

	/* Query the cluster. This will be the final decision made
	 * before we go ahead and wipe the inode. */
	status = ocfs2_query_inode_wipe(inode, di_bh, &wipe);
	if (!wipe || status < 0) {
		/* Error and remote inode busy both mean we won't be
		 * removing the inode, so they take almost the same
		 * path. */
		if (status < 0)
			mlog_errno(status);

		/* Someone in the cluster has disallowed a wipe of
		 * this inode, or it was never completely
		 * orphaned. Write out the pages and exit now. */
		ocfs2_cleanup_delete_inode(inode, 1);
		goto bail_unlock_inode;
	}

	ocfs2_cleanup_delete_inode(inode, 0);

	status = ocfs2_wipe_inode(inode, di_bh);
	if (status < 0) {
		if (status != -EDEADLK)
			mlog_errno(status);
		goto bail_unlock_inode;
	}

	/*
	 * Mark the inode as successfully deleted.
	 *
	 * This is important for ocfs2_clear_inode() as it will check
	 * this flag and skip any checkpointing work
	 *
	 * ocfs2_stuff_meta_lvb() also uses this flag to invalidate
	 * the LVB for other nodes.
	 */
	OCFS2_I(inode)->ip_flags |= OCFS2_INODE_DELETED;

bail_unlock_inode:
	ocfs2_inode_unlock(inode, 1);
	brelse(di_bh);

bail_unlock_nfs_sync:
	ocfs2_nfs_sync_unlock(OCFS2_SB(inode->i_sb), 0, nofs_flag);

bail_unblock:
	ocfs2_unblock_signals(&oldset);
bail:
	return;
}
/* …（同文件无关代码省略）… */
static void ocfs2_clear_inode(struct inode *inode)
{
	int status;
	struct ocfs2_inode_info *oi = OCFS2_I(inode);
	struct ocfs2_super *osb = OCFS2_SB(inode->i_sb);

	clear_inode(inode);
	trace_ocfs2_clear_inode((unsigned long long)oi->ip_blkno,
				inode->i_nlink);

	mlog_bug_on_msg(osb == NULL,
			"Inode=%llu\n", inode->i_ino);

	dquot_drop(inode);

	/* To prevent remote deletes we hold open lock before, now it
	 * is time to unlock PR and EX open locks. */
	ocfs2_open_unlock(inode);

	/* Do these before all the other work so that we don't bounce
	 * the downconvert thread while waiting to destroy the locks. */
	ocfs2_mark_lockres_freeing(osb, &oi->ip_rw_lockres);
	ocfs2_mark_lockres_freeing(osb, &oi->ip_inode_lockres);
	ocfs2_mark_lockres_freeing(osb, &oi->ip_open_lockres);

	ocfs2_resv_discard(&osb->osb_la_resmap,
			   &oi->ip_la_data_resv);
	ocfs2_resv_init_once(&oi->ip_la_data_resv);

	/* We very well may get a clear_inode before all an inodes
	 * metadata has hit disk. Of course, we can't drop any cluster
	 * locks until the journal has finished with it. The only
	 * exception here are successfully wiped inodes - their
	 * metadata can now be considered to be part of the system
	 * inodes from which it came. */
	if (!(oi->ip_flags & OCFS2_INODE_DELETED))
		ocfs2_checkpoint_inode(inode);

	mlog_bug_on_msg(!list_empty(&oi->ip_io_markers),
			"Clear inode of %llu, inode has io markers\n",
			(unsigned long long)oi->ip_blkno);
	mlog_bug_on_msg(!list_empty(&oi->ip_unwritten_list),
			"Clear inode of %llu, inode has unwritten extents\n",
			(unsigned long long)oi->ip_blkno);

	ocfs2_extent_map_trunc(inode, 0);

	status = ocfs2_drop_inode_locks(inode);
	if (status < 0)
		mlog_errno(status);

	ocfs2_lock_res_free(&oi->ip_rw_lockres);
	ocfs2_lock_res_free(&oi->ip_inode_lockres);
	ocfs2_lock_res_free(&oi->ip_open_lockres);

	ocfs2_metadata_cache_exit(INODE_CACHE(inode));

	mlog_bug_on_msg(INODE_CACHE(inode)->ci_num_cached,
			"Clear inode of %llu, inode has %u cache items\n",
			(unsigned long long)oi->ip_blkno,
			INODE_CACHE(inode)->ci_num_cached);

	mlog_bug_on_msg(!(INODE_CACHE(inode)->ci_flags & OCFS2_CACHE_FL_INLINE),
			"Clear inode of %llu, inode has a bad flag\n",
			(unsigned long long)oi->ip_blkno);

	mlog_bug_on_msg(spin_is_locked(&oi->ip_lock),
			"Clear inode of %llu, inode is locked\n",
			(unsigned long long)oi->ip_blkno);

	mlog_bug_on_msg(!mutex_trylock(&oi->ip_io_mutex),
			"Clear inode of %llu, io_mutex is locked\n",
			(unsigned long long)oi->ip_blkno);
	mutex_unlock(&oi->ip_io_mutex);

	/*
	 * down_trylock() returns 0, down_write_trylock() returns 1
	 * kernel 1, world 0
	 */
	mlog_bug_on_msg(!down_write_trylock(&oi->ip_alloc_sem),
			"Clear inode of %llu, alloc_sem is locked\n",
			(unsigned long long)oi->ip_blkno);
	up_write(&oi->ip_alloc_sem);

	mlog_bug_on_msg(oi->ip_open_count,
			"Clear inode of %llu has open count %d\n",
			(unsigned long long)oi->ip_blkno, oi->ip_open_count);

	/* Clear all other flags. */
	oi->ip_flags = 0;
	oi->ip_dir_start_lookup = 0;
	oi->ip_blkno = 0ULL;

	/*
	 * ip_jinode is used to track txns against this inode. We ensure that
	 * the journal is flushed before journal shutdown. Thus it is safe to
	 * have inodes get cleaned up after journal shutdown.
	 */
	if (!osb->journal)
		return;

	jbd2_journal_release_jbd_inode(osb->journal->j_journal,
				       &oi->ip_jinode);
}

void ocfs2_evict_inode(struct inode *inode)
{
	write_inode_now(inode, 1);

	if (!inode->i_nlink ||
	    (OCFS2_I(inode)->ip_flags & OCFS2_INODE_MAYBE_ORPHANED)) {
		ocfs2_delete_inode(inode);
	} else {
		truncate_inode_pages_final(&inode->i_data);
	}
	ocfs2_clear_inode(inode);
}
/* …（同文件无关代码省略）… */
int ocfs2_mark_inode_dirty(handle_t *handle,
			   struct inode *inode,
			   struct buffer_head *bh)
{
	int status;
	struct ocfs2_dinode *fe = (struct ocfs2_dinode *) bh->b_data;

	trace_ocfs2_mark_inode_dirty((unsigned long long)OCFS2_I(inode)->ip_blkno);

	status = ocfs2_journal_access_di(handle, INODE_CACHE(inode), bh,
					 OCFS2_JOURNAL_ACCESS_WRITE);
	if (status < 0) {
		mlog_errno(status);
		goto leave;
	}

	spin_lock(&OCFS2_I(inode)->ip_lock);
	fe->i_clusters = cpu_to_le32(OCFS2_I(inode)->ip_clusters);
	ocfs2_get_inode_flags(OCFS2_I(inode));
	fe->i_attr = cpu_to_le32(OCFS2_I(inode)->ip_attr);
	fe->i_dyn_features = cpu_to_le16(OCFS2_I(inode)->ip_dyn_features);
	spin_unlock(&OCFS2_I(inode)->ip_lock);

	fe->i_size = cpu_to_le64(i_size_read(inode));
	ocfs2_set_links_count(fe, inode->i_nlink);
	fe->i_uid = cpu_to_le32(i_uid_read(inode));
	fe->i_gid = cpu_to_le32(i_gid_read(inode));
	fe->i_mode = cpu_to_le16(inode->i_mode);
	fe->i_atime = cpu_to_le64(inode_get_atime_sec(inode));
	fe->i_atime_nsec = cpu_to_le32(inode_get_atime_nsec(inode));
	fe->i_ctime = cpu_to_le64(inode_get_ctime_sec(inode));
	fe->i_ctime_nsec = cpu_to_le32(inode_get_ctime_nsec(inode));
	fe->i_mtime = cpu_to_le64(inode_get_mtime_sec(inode));
	fe->i_mtime_nsec = cpu_to_le32(inode_get_mtime_nsec(inode));

	ocfs2_journal_dirty(handle, bh);
	ocfs2_update_inode_fsync_trans(handle, inode, 1);
leave:
	return status;
}
