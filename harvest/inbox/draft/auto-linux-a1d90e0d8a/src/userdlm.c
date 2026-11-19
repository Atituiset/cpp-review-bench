// AUTO-DRAFT from torvalds/linux PR #4405c18c95d2d0f99f6c581ef0ee8591a1aa6ccd
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdlib.h>
  // <<< BUG ANCHOR
static inline struct ocfs2_cluster_connection *
cluster_connection_from_user_lockres(struct user_lock_res *lockres)
{
	struct dlmfs_inode_private *ip;

	ip = container_of(lockres,
			  struct dlmfs_inode_private,
			  ip_lockres);
	return ip->ip_conn;
}
/* …（同文件无关代码省略）… */
static struct inode *
user_dlm_inode_from_user_lockres(struct user_lock_res *lockres)
{
	struct dlmfs_inode_private *ip;

	ip = container_of(lockres,
			  struct dlmfs_inode_private,
			  ip_lockres);
	return &ip->ip_vfs_inode;
}
/* …（同文件无关代码省略）… */
static inline void user_recover_from_dlm_error(struct user_lock_res *lockres)
{
	spin_lock(&lockres->l_lock);
	lockres->l_flags &= ~USER_LOCK_BUSY;
	spin_unlock(&lockres->l_lock);
}
/* …（同文件无关代码省略）… */
#define user_log_dlm_error(_func, _stat, _lockres) do {			\
	mlog(ML_ERROR, "Dlm error %d while calling %s on "		\
		"resource %.*s\n", _stat, _func,			\
		_lockres->l_namelen, _lockres->l_name); 		\
} while (0)
/* …（同文件无关代码省略）… */
static inline int user_highest_compat_lock_level(int level)
{
	int new_level = DLM_LOCK_EX;

	if (level == DLM_LOCK_EX)
		new_level = DLM_LOCK_NL;
	else if (level == DLM_LOCK_PR)
		new_level = DLM_LOCK_PR;
	return new_level;
}
/* …（同文件无关代码省略）… */
static inline void user_dlm_grab_inode_ref(struct user_lock_res *lockres)
{
	struct inode *inode;
	inode = user_dlm_inode_from_user_lockres(lockres);
	if (!igrab(inode))
		BUG();
}
/* …（同文件无关代码省略）… */

static void __user_dlm_queue_lockres(struct user_lock_res *lockres)
{
	if (!(lockres->l_flags & USER_LOCK_QUEUED)) {
		user_dlm_grab_inode_ref(lockres);

		INIT_WORK(&lockres->l_work, user_dlm_unblock_lock);
/* …（同文件无关代码省略）… */

static inline void user_dlm_drop_inode_ref(struct user_lock_res *lockres)
{
	struct inode *inode;
	inode = user_dlm_inode_from_user_lockres(lockres);
	iput(inode);
}

static void user_dlm_unblock_lock(struct work_struct *work)
{
	int new_level, status;
	struct user_lock_res *lockres =
		container_of(work, struct user_lock_res, l_work);
	struct ocfs2_cluster_connection *conn =
		cluster_connection_from_user_lockres(lockres);

	mlog(0, "lockres %.*s\n", lockres->l_namelen, lockres->l_name);

	spin_lock(&lockres->l_lock);

	mlog_bug_on_msg(!(lockres->l_flags & USER_LOCK_QUEUED),
			"Lockres %.*s, flags 0x%x\n",
			lockres->l_namelen, lockres->l_name, lockres->l_flags);

	/* notice that we don't clear USER_LOCK_BLOCKED here. If it's
	 * set, we want user_ast clear it. */
	lockres->l_flags &= ~USER_LOCK_QUEUED;

	/* It's valid to get here and no longer be blocked - if we get
	 * several basts in a row, we might be queued by the first
	 * one, the unblock thread might run and clear the queued
	 * flag, and finally we might get another bast which re-queues
	 * us before our ast for the downconvert is called. */
	if (!(lockres->l_flags & USER_LOCK_BLOCKED)) {
		mlog(ML_BASTS, "lockres %.*s USER_LOCK_BLOCKED\n",
		     lockres->l_namelen, lockres->l_name);
		spin_unlock(&lockres->l_lock);
		goto drop_ref;
	}

	if (lockres->l_flags & USER_LOCK_IN_TEARDOWN) {
		mlog(ML_BASTS, "lockres %.*s USER_LOCK_IN_TEARDOWN\n",
		     lockres->l_namelen, lockres->l_name);
		spin_unlock(&lockres->l_lock);
		goto drop_ref;
	}

	if (lockres->l_flags & USER_LOCK_BUSY) {
		if (lockres->l_flags & USER_LOCK_IN_CANCEL) {
			mlog(ML_BASTS, "lockres %.*s USER_LOCK_IN_CANCEL\n",
			     lockres->l_namelen, lockres->l_name);
			spin_unlock(&lockres->l_lock);
			goto drop_ref;
		}

		lockres->l_flags |= USER_LOCK_IN_CANCEL;
		spin_unlock(&lockres->l_lock);

		status = ocfs2_dlm_unlock(conn, &lockres->l_lksb,
					  DLM_LKF_CANCEL);
		if (status)
			user_log_dlm_error("ocfs2_dlm_unlock", status, lockres);
		goto drop_ref;
	}

	/* If there are still incompat holders, we can exit safely
	 * without worrying about re-queueing this lock as that will
	 * happen on the last call to user_cluster_unlock. */
	if ((lockres->l_blocking == DLM_LOCK_EX)
	    && (lockres->l_ex_holders || lockres->l_ro_holders)) {
		spin_unlock(&lockres->l_lock);
		mlog(ML_BASTS, "lockres %.*s, EX/PR Holders %u,%u\n",
		     lockres->l_namelen, lockres->l_name,
		     lockres->l_ex_holders, lockres->l_ro_holders);
		goto drop_ref;
	}

	if ((lockres->l_blocking == DLM_LOCK_PR)
	    && lockres->l_ex_holders) {
		spin_unlock(&lockres->l_lock);
		mlog(ML_BASTS, "lockres %.*s, EX Holders %u\n",
		     lockres->l_namelen, lockres->l_name,
		     lockres->l_ex_holders);
		goto drop_ref;
	}

	/* yay, we can downconvert now. */
	new_level = user_highest_compat_lock_level(lockres->l_blocking);
	lockres->l_requested = new_level;
	lockres->l_flags |= USER_LOCK_BUSY;
	mlog(ML_BASTS, "lockres %.*s, downconvert %d => %d\n",
	     lockres->l_namelen, lockres->l_name, lockres->l_level, new_level);
	spin_unlock(&lockres->l_lock);

	/* need lock downconvert request now... */
	status = ocfs2_dlm_lock(conn, new_level, &lockres->l_lksb,
				DLM_LKF_CONVERT|DLM_LKF_VALBLK,
				lockres->l_name,
				lockres->l_namelen);
	if (status) {
		user_log_dlm_error("ocfs2_dlm_lock", status, lockres);
		user_recover_from_dlm_error(lockres);
	}

drop_ref:
	user_dlm_drop_inode_ref(lockres);
}
