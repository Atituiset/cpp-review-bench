// AUTO-DRAFT from torvalds/linux PR #a6656aefd328d544420ecec121fc898b7f067827
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static inline void unix_set_secdata(struct scm_cookie *scm, struct sk_buff *skb)
{
	scm->secid = UNIXCB(skb).secid;
}
/* …（同文件无关代码省略）… */
static inline bool unix_secdata_eq(struct scm_cookie *scm, struct sk_buff *skb)
{
	return (scm->secid == UNIXCB(skb).secid);
}
/* …（同文件无关代码省略）… */
}

struct unix_peercred {
	struct pid *peer_pid;
	const struct cred *peer_cred;
};

static inline int prepare_peercred(struct unix_peercred *peercred)
{
	struct pid *pid;
	int err;

	pid = task_tgid(current);
	err = pidfs_register_pid(pid);
	if (likely(!err)) {
		peercred->peer_pid = get_pid(pid);
		peercred->peer_cred = get_current_cred();
	}
	return err;
}

static void drop_peercred(struct unix_peercred *peercred)
{
	const struct cred *cred = NULL;
	struct pid *pid = NULL;

	might_sleep();

	swap(peercred->peer_pid, pid);
	swap(peercred->peer_cred, cred);

	put_pid(pid);
	put_cred(cred);
}

static inline void init_peercred(struct sock *sk,
				 const struct unix_peercred *peercred)
{
	sk->sk_peer_pid = peercred->peer_pid;
	sk->sk_peer_cred = peercred->peer_cred;
}

static void update_peercred(struct sock *sk, struct unix_peercred *peercred)
{
	const struct cred *old_cred;
	struct pid *old_pid;

	spin_lock(&sk->sk_peer_lock);
	old_pid = sk->sk_peer_pid;
	old_cred = sk->sk_peer_cred;
	init_peercred(sk, peercred);
	spin_unlock(&sk->sk_peer_lock);

	peercred->peer_pid = old_pid;
	peercred->peer_cred = old_cred;
}

/* …（同文件无关代码省略）… */
	lockdep_assert_held(&unix_sk(peersk)->lock);

	spin_lock(&sk->sk_peer_lock);
	sk->sk_peer_pid = get_pid(peersk->sk_peer_pid);
	sk->sk_peer_cred = get_cred(peersk->sk_peer_cred);
	spin_unlock(&sk->sk_peer_lock);
}

static bool unix_may_passcred(const struct sock *sk)
{
	return sk->sk_scm_credentials || sk->sk_scm_pidfd;
}

static int unix_listen(struct socket *sock, int backlog)
/* …（同文件无关代码省略）… */
	if (level == SOL_SOCKET) {
		switch (optname) {
		case SO_PEERPIDFD:
			return true;
		default:
			return false;
/* …（同文件无关代码省略）… */
static void unix_detach_fds(struct scm_cookie *scm, struct sk_buff *skb)
{
	scm->fp = UNIXCB(skb).fp;
	UNIXCB(skb).fp = NULL;

	unix_destroy_fpl(scm->fp);
}
/* …（同文件无关代码省略）… */
{
	struct scm_cookie scm = {};

	swap(scm.pid, UNIXCB(skb).pid);

	if (UNIXCB(skb).fp)
		unix_detach_fds(&scm, skb);
/* …（同文件无关代码省略）… */
{
	int err = 0;

	UNIXCB(skb).pid = get_pid(scm->pid);
	UNIXCB(skb).uid = scm->creds.uid;
	UNIXCB(skb).gid = scm->creds.gid;
	UNIXCB(skb).fp = NULL;
/* …（同文件无关代码省略）… */

static void unix_skb_to_scm(struct sk_buff *skb, struct scm_cookie *scm)
{
	scm_set_cred(scm, UNIXCB(skb).pid, UNIXCB(skb).uid, UNIXCB(skb).gid);
	unix_set_secdata(scm, skb);
}

/* …（同文件无关代码省略）… */
static int unix_maybe_add_creds(struct sk_buff *skb, const struct sock *sk,
				const struct sock *other)
{
	if (UNIXCB(skb).pid)
		return 0;

	if (unix_may_passcred(sk) || unix_may_passcred(other) ||
	    !other->sk_socket) {
		struct pid *pid;
		int err;

		pid = task_tgid(current);
		err = pidfs_register_pid(pid);
		if (unlikely(err))
			return err;

		UNIXCB(skb).pid = get_pid(pid);
		current_uid_gid(&UNIXCB(skb).uid, &UNIXCB(skb).gid);
	}

	return 0;
}

static bool unix_skb_scm_eq(struct sk_buff *skb,
			    struct scm_cookie *scm)
{
	return UNIXCB(skb).pid == scm->pid &&
	       uid_eq(UNIXCB(skb).uid, scm->creds.uid) &&
	       gid_eq(UNIXCB(skb).gid, scm->creds.gid) &&
	       unix_secdata_eq(scm, skb);
/* …（同文件无关代码省略）… */

		if (check_creds) {
			/* Never glue messages from different writers */
			if (!unix_skb_scm_eq(skb, &scm))
				break;
		} else if (unix_may_passcred(sk)) {
			/* Copy credentials */
