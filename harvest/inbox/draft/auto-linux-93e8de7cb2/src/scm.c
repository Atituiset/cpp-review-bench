// AUTO-DRAFT from torvalds/linux PR #a6656aefd328d544420ecec121fc898b7f067827
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR

static inline int scm_replace_pid(struct scm_cookie *scm, struct pid *pid)
{
	int err;

	/* drop all previous references */
/* …（同文件无关代码省略）… */
	if (unlikely(err))
		return err;

	scm->pid = pid;
	scm->creds.pid = pid_vnr(pid);
	return 0;
}
/* …（同文件无关代码省略）… */
			if (err)
				goto error;

			if (!p->pid || pid_vnr(p->pid) != creds.pid) {
				struct pid *pid;
				err = -ESRCH;
				pid = find_get_pid(creds.pid);
/* …（同文件无关代码省略）… */
int put_cmsg(struct msghdr * msg, int level, int type, int len, void *data)
{
	int cmlen = CMSG_LEN(len);

	if (msg->msg_flags & MSG_CMSG_COMPAT)
		return put_cmsg_compat(msg, level, type, len, data);

	if (!msg->msg_control || msg->msg_controllen < sizeof(struct cmsghdr)) {
		msg->msg_flags |= MSG_CTRUNC;
		return 0; /* XXX: return error? check spec. */
	}
	if (msg->msg_controllen < cmlen) {
		msg->msg_flags |= MSG_CTRUNC;
		cmlen = msg->msg_controllen;
	}

	if (msg->msg_control_is_user) {
		struct cmsghdr __user *cm = msg->msg_control_user;

		check_object_size(data, cmlen - sizeof(*cm), true);

		scoped_user_write_access_size(cm, cmlen, efault) {
			unsafe_put_user(cmlen, &cm->cmsg_len, efault);
			unsafe_put_user(level, &cm->cmsg_level, efault);
			unsafe_put_user(type, &cm->cmsg_type, efault);
			unsafe_copy_to_user(CMSG_USER_DATA(cm), data,
					    cmlen - sizeof(*cm), efault);
		}
	} else {
		struct cmsghdr *cm = msg->msg_control;

		cm->cmsg_level = level;
		cm->cmsg_type = type;
		cm->cmsg_len = cmlen;
		memcpy(CMSG_DATA(cm), data, cmlen - sizeof(*cm));
	}

	cmlen = min(CMSG_SPACE(len), msg->msg_controllen);
	if (msg->msg_control_is_user)
		msg->msg_control_user += cmlen;
	else
		msg->msg_control += cmlen;
	msg->msg_controllen -= cmlen;
	return 0;

efault:
	return -EFAULT;
}
/* …（同文件无关代码省略）… */
static bool scm_has_secdata(struct sock *sk)
{
	return sk->sk_scm_security;
}
/* …（同文件无关代码省略）… */
}
#endif

static void scm_pidfd_recv(struct msghdr *msg, struct scm_cookie *scm)
{
	struct file *pidfd_file = NULL;
	int len, pidfd;

	/* put_cmsg() doesn't return an error if CMSG is truncated,
	 * that's why we need to opencode these checks here.
	 */
/* …（同文件无关代码省略）… */
		return;
	}

	if (!scm->pid)
		return;

	pidfd = pidfd_prepare(scm->pid, PIDFD_STALE, &pidfd_file);

	if (put_cmsg(msg, SOL_SOCKET, SCM_PIDFD, sizeof(int), &pidfd)) {
		if (pidfd_file) {
			put_unused_fd(pidfd);
			fput(pidfd_file);
/* …（同文件无关代码省略）… */
{
	if (!msg->msg_control) {
		if (sk->sk_scm_credentials || sk->sk_scm_pidfd ||
		    scm->fp || scm_has_secdata(sk))
			msg->msg_flags |= MSG_CTRUNC;

		scm_destroy(scm);
/* …（同文件无关代码省略）… */
	}

	if (sock->sk->sk_scm_pidfd)
		scm_pidfd_recv(msg, scm);

	scm_destroy_cred(scm);
}
