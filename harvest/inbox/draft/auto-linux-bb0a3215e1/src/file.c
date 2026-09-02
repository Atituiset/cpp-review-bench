// AUTO-DRAFT from torvalds/linux PR #89a312991dc6e638a36adc43ccb91dbc25504c04
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
void
cifs_down_write(struct rw_semaphore *sem)
{
	while (!down_write_trylock(sem))
		msleep(10);
}
/* …（同文件无关代码省略）… */
static void cifsFileInfo_put_final(struct cifsFileInfo *cifs_file)
{
	struct inode *inode = d_inode(cifs_file->dentry);
	struct cifsInodeInfo *cifsi = CIFS_I(inode);
	struct cifsLockInfo *li, *tmp;

	/*
	 * Delete any outstanding lock records. We'll lose them when the file
	 * is closed anyway.
	 */
	cifs_down_write(&cifsi->lock_sem);
	list_for_each_entry_safe(li, tmp, &cifs_file->llist->locks, llist) {
		list_del(&li->llist);
		cifs_del_lock_waiters(li);
		kfree(li);
	}
	list_del(&cifs_file->llist->llist);
	kfree(cifs_file->llist);
	up_write(&cifsi->lock_sem);

	cifs_put_tlink(cifs_file->tlink);
	dput(cifs_file->dentry);
	kfree(cifs_file->symlink_target);
	kfree(cifs_file);
}
/* …（同文件无关代码省略）… */
void cifsFileInfo_put(struct cifsFileInfo *cifs_file)
{
	_cifsFileInfo_put(cifs_file, true, true);
}
/* …（同文件无关代码省略）… */
void _cifsFileInfo_put(struct cifsFileInfo *cifs_file,
		       bool wait_oplock_handler, bool offload)
{
	struct inode *inode = d_inode(cifs_file->dentry);
	struct cifs_tcon *tcon = tlink_tcon(cifs_file->tlink);
	struct TCP_Server_Info *server = tcon->ses->server;
	struct cifsInodeInfo *cifsi = CIFS_I(inode);
	struct super_block *sb = inode->i_sb;
	struct cifs_sb_info *cifs_sb = CIFS_SB(sb);
	struct cifs_fid fid = {};
	struct cifs_pending_open open;
	bool oplock_break_cancelled;
	bool serverclose_offloaded = false;

	spin_lock(&tcon->open_file_lock);
	spin_lock(&cifsi->open_file_lock);
	spin_lock(&cifs_file->file_info_lock);

	cifs_file->offload = offload;
	if (--cifs_file->count > 0) {
		spin_unlock(&cifs_file->file_info_lock);
		spin_unlock(&cifsi->open_file_lock);
		spin_unlock(&tcon->open_file_lock);
		return;
	}
	spin_unlock(&cifs_file->file_info_lock);

	if (server->ops->get_lease_key)
		server->ops->get_lease_key(inode, &fid);

	/* store open in pending opens to make sure we don't miss lease break */
	cifs_add_pending_open_locked(&fid, cifs_file->tlink, &open);

	/* remove it from the lists */
	list_del(&cifs_file->flist);
	list_del(&cifs_file->tlist);
	atomic_dec(&tcon->num_local_opens);

	if (list_empty(&cifsi->openFileList)) {
		cifs_dbg(FYI, "closing last open instance for inode %p\n",
			 d_inode(cifs_file->dentry));
		/*
		 * In strict cache mode we need invalidate mapping on the last
		 * close  because it may cause a error when we open this file
		 * again and get at least level II oplock.
		 */
		if (cifs_sb_flags(cifs_sb) & CIFS_MOUNT_STRICT_IO)
			set_bit(CIFS_INO_INVALID_MAPPING, &cifsi->flags);
		cifs_set_oplock_level(cifsi, 0);
	}

	if (OPEN_FMODE(cifs_file->f_flags) & FMODE_WRITE) {
		/* Stamp while open_file_lock is held; covers all close paths
		 * including background I/O. Pairs with smp_load_acquire() in
		 * is_size_safe_to_change().
		 */
		smp_store_release(&cifsi->time_last_write, jiffies);
	}

	spin_unlock(&cifsi->open_file_lock);
	spin_unlock(&tcon->open_file_lock);

	oplock_break_cancelled = wait_oplock_handler ?
		cancel_work_sync(&cifs_file->oplock_break) : false;

	if (!tcon->need_reconnect && !cifs_file->invalidHandle) {
		struct TCP_Server_Info *server = tcon->ses->server;
		unsigned int xid;
		int rc = 0;

		xid = get_xid();
		if (server->ops->close_getattr)
			rc = server->ops->close_getattr(xid, tcon, cifs_file);
		else if (server->ops->close)
			rc = server->ops->close(xid, tcon, &cifs_file->fid);
		_free_xid(xid);

		if (rc == -EBUSY || rc == -EAGAIN) {
			// Server close failed, hence offloading it as an async op
			queue_work(serverclose_wq, &cifs_file->serverclose);
			serverclose_offloaded = true;
		}
	}

	if (oplock_break_cancelled)
		cifs_done_oplock_break(cifsi);

	cifs_del_pending_open(&open);

	// if serverclose has been offloaded to wq (on failure), it will
	// handle offloading put as well. If serverclose not offloaded,
	// we need to handle offloading put here.
	if (!serverclose_offloaded) {
		if (offload)
			queue_work(fileinfo_put_wq, &cifs_file->put);
		else
			cifsFileInfo_put_final(cifs_file);
	}
}
/* …（同文件无关代码省略）… */
int cifs_file_flush(const unsigned int xid, struct inode *inode,
		    struct cifsFileInfo *cfile)
{
	struct cifs_sb_info *cifs_sb = CIFS_SB(inode);
	struct cifs_tcon *tcon;
	int rc;

	if (cifs_sb_flags(cifs_sb) & CIFS_MOUNT_NOSSYNC)
		return 0;

	if (cfile && (OPEN_FMODE(cfile->f_flags) & FMODE_WRITE)) {
		tcon = tlink_tcon(cfile->tlink);
		return tcon->ses->server->ops->flush(xid, tcon,
						     &cfile->fid);
	}
	rc = cifs_get_writable_file(CIFS_I(inode), FIND_ANY, &cfile);
	if (!rc) {
		tcon = tlink_tcon(cfile->tlink);
		rc = tcon->ses->server->ops->flush(xid, tcon, &cfile->fid);
		cifsFileInfo_put(cfile);
	} else if (rc == -EBADF) {
		rc = 0;
	}
	return rc;
}
/* …（同文件无关代码省略）… */
	struct cifs_tcon *tcon;
	int rc;

	rc = filemap_write_and_wait(inode->i_mapping);
	if (is_interrupt_error(rc))
		return -ERESTARTSYS;
	mapping_set_error(inode->i_mapping, rc);

	cfile = find_writable_file(cinode, FIND_FSUID_ONLY);
	rc = cifs_file_flush(xid, inode, cfile);
	if (!rc) {
		if (cfile) {
			tcon = tlink_tcon(cfile->tlink);
			server = tcon->ses->server;
			rc = server->ops->set_file_size(xid, tcon,
							cfile, 0, false);
		}
		if (!rc) {
			netfs_resize_file(&cinode->netfs, 0, true);
			cifs_setsize(inode, 0);
			cifs_invalidate_cache(inode, 0);
		}
	}
	if (cfile)
		cifsFileInfo_put(cfile);
	return rc;
/* …（同文件无关代码省略）… */
void
cifs_del_lock_waiters(struct cifsLockInfo *lock)
{
	struct cifsLockInfo *li, *tmp;
	list_for_each_entry_safe(li, tmp, &lock->blist, blist) {
		list_del_init(&li->blist);
		wake_up(&li->block_q);
	}
}
/* …（同文件无关代码省略）… */
struct cifsFileInfo *
find_writable_file(struct cifsInodeInfo *cifs_inode, int flags)
{
	struct cifsFileInfo *cfile;
	int rc;

	rc = cifs_get_writable_file(cifs_inode, flags, &cfile);
	if (rc)
		cifs_dbg(FYI, "Couldn't find writable handle rc=%d\n", rc);

	return cfile;
}
