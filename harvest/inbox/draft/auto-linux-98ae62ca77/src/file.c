// AUTO-DRAFT from torvalds/linux PR #62b13f41be6284250c06ab4d79a7fb13fe93f4d7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#define CIFS_REOPEN_BOUND	BIT(0)	/* caller bounds the retries */
#define CIFS_REOPEN_NOWAIT	BIT(1)	/* fail fast, skip reconnect wait */
/* …（同文件无关代码省略）… */
static inline int cifs_convert_flags(unsigned int oflags, int rdwr_for_fscache)
{
	int flags = 0;

	if (oflags & O_TMPFILE)
		flags |= DELETE;

	if ((oflags & O_ACCMODE) == O_RDONLY)
		return flags | GENERIC_READ;
	if ((oflags & O_ACCMODE) == O_WRONLY) {
		return flags | (rdwr_for_fscache == 1 ?
				(GENERIC_READ | GENERIC_WRITE) : GENERIC_WRITE);
	}
	if ((oflags & O_ACCMODE) == O_RDWR) {
		/* GENERIC_ALL is too much permission to request
		   can cause unnecessary access denied on create */
		/* return GENERIC_ALL; */
		return flags | GENERIC_READ | GENERIC_WRITE;
	}

	return flags | READ_CONTROL | FILE_WRITE_ATTRIBUTES |
		FILE_READ_ATTRIBUTES | FILE_WRITE_EA | FILE_APPEND_DATA |
		FILE_WRITE_DATA | FILE_READ_DATA;
}
/* …（同文件无关代码省略）… */
static u32 cifs_posix_convert_flags(unsigned int flags)
{
	u32 posix_flags = 0;

	if ((flags & O_ACCMODE) == O_RDONLY)
		posix_flags = SMB_O_RDONLY;
	else if ((flags & O_ACCMODE) == O_WRONLY)
		posix_flags = SMB_O_WRONLY;
	else if ((flags & O_ACCMODE) == O_RDWR)
		posix_flags = SMB_O_RDWR;

	if (flags & O_CREAT) {
		posix_flags |= SMB_O_CREAT;
		if (flags & O_EXCL)
			posix_flags |= SMB_O_EXCL;
	} else if (flags & O_EXCL)
		cifs_dbg(FYI, "Application %s pid %d has incorrectly set O_EXCL flag but not O_CREAT on file open. Ignoring O_EXCL\n",
			 current->comm, current->tgid);

	if (flags & O_TRUNC)
		posix_flags |= SMB_O_TRUNC;
	/* be safe and imply O_SYNC for O_DSYNC */
	if (flags & O_DSYNC)
		posix_flags |= SMB_O_SYNC;
	if (flags & O_DIRECTORY)
		posix_flags |= SMB_O_DIRECTORY;
	if (flags & O_NOFOLLOW)
		posix_flags |= SMB_O_NOFOLLOW;
	if (flags & O_DIRECT)
		posix_flags |= SMB_O_DIRECT;

	return posix_flags;
}
/* …（同文件无关代码省略）… */
int cifs_posix_open(const char *full_path, struct inode **pinode,
			struct super_block *sb, int mode, unsigned int f_flags,
			__u32 *poplock, __u16 *pnetfid, unsigned int xid)
{
	int rc;
	FILE_UNIX_BASIC_INFO *presp_data;
	__u32 posix_flags = 0;
	struct cifs_sb_info *cifs_sb = CIFS_SB(sb);
	struct cifs_fattr fattr;
	struct tcon_link *tlink;
	struct cifs_tcon *tcon;

	cifs_dbg(FYI, "posix open %s\n", full_path);

	presp_data = kzalloc_obj(FILE_UNIX_BASIC_INFO);
	if (presp_data == NULL)
		return -ENOMEM;

	tlink = cifs_sb_tlink(cifs_sb);
	if (IS_ERR(tlink)) {
		rc = PTR_ERR(tlink);
		goto posix_open_ret;
	}

	tcon = tlink_tcon(tlink);
	mode &= ~current_umask();

	posix_flags = cifs_posix_convert_flags(f_flags);
	rc = CIFSPOSIXCreate(xid, tcon, posix_flags, mode, pnetfid, presp_data,
			     poplock, full_path, cifs_remap(cifs_sb));
	cifs_put_tlink(tlink);

	if (rc)
		goto posix_open_ret;

	if (presp_data->Type == cpu_to_le32(-1))
		goto posix_open_ret; /* open ok, caller does qpathinfo */

	if (!pinode)
		goto posix_open_ret; /* caller does not need info */

	cifs_unix_basic_to_fattr(&fattr, presp_data, cifs_sb);

	/* get new inode and set it up */
	if (*pinode == NULL) {
		cifs_fill_uniqueid(sb, &fattr);
		*pinode = cifs_iget(sb, &fattr);
		if (!*pinode) {
			rc = -ENOMEM;
			goto posix_open_ret;
		}
	} else {
		cifs_revalidate_mapping(*pinode);
		rc = cifs_fattr_to_inode(*pinode, &fattr, false);
	}

posix_open_ret:
	kfree(presp_data);
	return rc;
}
/* …（同文件无关代码省略）… */
static bool
cifs_has_mand_locks(struct cifsInodeInfo *cinode)
{
	struct cifs_fid_locks *cur;
	bool has_locks = false;

	down_read(&cinode->lock_sem);
	list_for_each_entry(cur, &cinode->llist, llist) {
		if (!list_empty(&cur->locks)) {
			has_locks = true;
			break;
		}
	}
	up_read(&cinode->lock_sem);
	return has_locks;
}
/* …（同文件无关代码省略）… */
void
cifs_down_write(struct rw_semaphore *sem)
{
	while (!down_write_trylock(sem))
		msleep(10);
}
/* …（同文件无关代码省略）… */
struct cifsFileInfo *
cifsFileInfo_get(struct cifsFileInfo *cifs_file)
{
	spin_lock(&cifs_file->file_info_lock);
	cifsFileInfo_get_locked(cifs_file);
	spin_unlock(&cifs_file->file_info_lock);
	return cifs_file;
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
	cifs_add_pend
