// AUTO-DRAFT from torvalds/linux PR #a6656aefd328d544420ecec121fc898b7f067827
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
			ret = -EOPNOTSUPP;
		break;

	case SO_PASSRIGHTS:
		if (sk_is_unix(sk))
			sk->sk_scm_rights = valbool;
/* …（同文件无关代码省略）… */
static void cred_to_ucred(struct pid *pid, const struct cred *cred,
			  struct ucred *ucred)
{
	ucred->pid = pid_vnr(pid);
	ucred->uid = ucred->gid = -1;
	if (cred) {
		struct user_namespace *current_ns = current_user_ns();

		ucred->uid = from_kuid_munged(current_ns, cred->euid);
		ucred->gid = from_kgid_munged(current_ns, cred->egid);
	}
}
/* …（同文件无关代码省略）… */
	return 0;
}

int sk_getsockopt(struct sock *sk, int level, int optname,
		  sockptr_t optval, sockptr_t optlen)
{
/* …（同文件无关代码省略）… */
		v.val = sk->sk_scm_pidfd;
		break;

	case SO_PASSRIGHTS:
		if (!sk_is_unix(sk))
			return -EOPNOTSUPP;
/* …（同文件无关代码省略）… */
			len = sizeof(peercred);

		spin_lock(&sk->sk_peer_lock);
		cred_to_ucred(sk->sk_peer_pid, sk->sk_peer_cred, &peercred);
		spin_unlock(&sk->sk_peer_lock);

		if (copy_to_sockptr(optval, &peercred, len))
/* …（同文件无关代码省略）… */
	}

	case SO_PEERPIDFD:
	{
		struct pid *peer_pid;
		struct file *pidfd_file = NULL;
		unsigned int flags = 0;
		int pidfd;

		if (len > sizeof(pidfd))
			len = sizeof(pidfd);

		spin_lock(&sk->sk_peer_lock);
		peer_pid = get_pid(sk->sk_peer_pid);
		spin_unlock(&sk->sk_peer_lock);

		if (!peer_pid)
			return -ENODATA;

		/* The use of PIDFD_STALE requires stashing of struct pid
		 * on pidfs with pidfs_register_pid() and only AF_UNIX
		 * were prepared for this.
		 */
		if (sk->sk_family == AF_UNIX)
			flags = PIDFD_STALE;

		pidfd = pidfd_prepare(peer_pid, flags, &pidfd_file);
		put_pid(peer_pid);
		if (pidfd < 0)
			return pidfd;

		if (copy_to_sockptr(optval, &pidfd, len) ||
		    copy_to_sockptr(optlen, &len, sizeof(int))) {
			put_unused_fd(pidfd);
			fput(pidfd_file);

			return -EFAULT;
		}

		fd_install(pidfd, pidfd_file);
		return 0;
	}

	case SO_PEERGROUPS:
	{
/* …（同文件无关代码省略）… */

	/* We do not need to acquire sk->sk_peer_lock, we are the last user. */
	put_cred(sk->sk_peer_cred);
	put_pid(sk->sk_peer_pid);

	if (likely(sk->sk_net_refcnt)) {
		put_net_track(net, &sk->ns_tracker);
/* …（同文件无关代码省略）… */
	sk->sk_frag.offset	=	0;
	sk->sk_peek_off		=	-1;

	sk->sk_peer_pid 	=	NULL;
	sk->sk_peer_cred	=	NULL;
	spin_lock_init(&sk->sk_peer_lock);
