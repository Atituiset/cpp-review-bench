// AUTO-DRAFT from torvalds/linux PR #62b13f41be6284250c06ab4d79a7fb13fe93f4d7
 */
void cifs_queue_oplock_break(struct cifsFileInfo *cfile)
{
	/*
	 * Bump the handle refcount now while we hold the
	 * open_file_lock to enforce the validity of it for the oplock
/* …（同文件无关代码省略）… */
	 *
	 * Only take a reference if the work is actually queued.
	 */
	if (queue_work(cifsoplockd_wq, &cfile->oplock_break))  // <<< BUG ANCHOR
		cifsFileInfo_get(cfile);
}

void cifs_done_oplock_break(struct cifsInodeInfo *cinode)
{
	clear_bit(CIFS_INODE_PENDING_OPLOCK_BREAK, &cinode->flags);
	wake_up_bit(&cinode->flags, CIFS_INODE_PENDING_OPLOCK_BREAK);
}
