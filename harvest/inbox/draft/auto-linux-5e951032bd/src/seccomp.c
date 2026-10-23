// AUTO-DRAFT from torvalds/linux PR #79132ddf81d78d9b84d9b8f05c7c5c3a3e912537
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <time.h>
  // <<< BUG ANCHOR
enum notify_state {
	SECCOMP_NOTIFY_INIT,
	SECCOMP_NOTIFY_SENT,
	SECCOMP_NOTIFY_REPLIED,
};
/* …（同文件无关代码省略）… */
struct seccomp_knotif {
	/* The struct pid of the task whose filter triggered the notification */
	struct task_struct *task;

	/* The "cookie" for this request; this is unique for this filter. */
	u64 id;

	/*
	 * The seccomp data. This pointer is valid the entire time this
	 * notification is active, since it comes from __seccomp_filter which
	 * eclipses the entire lifecycle here.
	 */
	const struct seccomp_data *data;

	/*
	 * Notification states. When SECCOMP_RET_USER_NOTIF is returned, a
	 * struct seccomp_knotif is created and starts out in INIT. Once the
	 * handler reads the notification off of an FD, it transitions to SENT.
	 * If a signal is received the state transitions back to INIT and
	 * another message is sent. When the userspace handler replies, state
	 * transitions to REPLIED.
	 */
	enum notify_state state;

	/* The return values, only valid when in SECCOMP_NOTIFY_REPLIED */
	int error;
	long val;
	u32 flags;

	/*
	 * Signals when this has changed states, such as the listener
	 * dying, a new seccomp addfd message, or changing to REPLIED
	 */
	struct completion ready;

	struct list_head list;

	/* outstanding addfd requests */
	struct list_head addfd;
};
/* …（同文件无关代码省略）… */
struct notification {
	atomic_t requests;
	u32 flags;
	u64 next_id;
	struct list_head notifications;
};
/* …（同文件无关代码省略）… */
struct action_cache {
	DECLARE_BITMAP(allow_native, SECCOMP_ARCH_NATIVE_NR);
#ifdef SECCOMP_ARCH_COMPAT
	DECLARE_BITMAP(allow_compat, SECCOMP_ARCH_COMPAT_NR);
#endif
};
/* …（同文件无关代码省略）… */
 * @log: true if all actions except for SECCOMP_RET_ALLOW should be logged
 * @wait_killable_recv: Put notifying process in killable state once the
 *			notification is received by the userspace listener.
 * @prev: points to a previously installed, or inherited, filter
 * @prog: the BPF program to evaluate
 * @notif: the struct that holds all notification related information
/* …（同文件无关代码省略）… */
struct seccomp_filter {
	refcount_t refs;
	refcount_t users;
	bool log;
	bool wait_killable_recv;
	struct action_cache cache;
	struct seccomp_filter *prev;
	struct bpf_prog *prog;
	struct notification *notif;
	struct mutex notify_lock;
	wait_queue_head_t wqh;
};
/* …（同文件无关代码省略）… */
	/* Set wait killable flag, if present. */
	if (flags & SECCOMP_FILTER_FLAG_WAIT_KILLABLE_RECV)
		filter->wait_killable_recv = true;

	/*
	 * If there is an existing filter, make it the prev and don't drop its
/* …（同文件无关代码省略）… */
static bool should_sleep_killable(struct seccomp_filter *match,
				  struct seccomp_knotif *n)
{
	return match->wait_killable_recv && n->state >= SECCOMP_NOTIFY_SENT;
}
/* …（同文件无关代码省略）… */
			 * Check to see whether we should switch to wait
			 * killable. Only return the interrupted error if not.
			 */
			if (!(!wait_killable && should_sleep_killable(match, &n)))
				goto interrupted;
		}

		addfd = list_first_entry_or_null(&n.addfd,
/* …（同文件无关代码省略）… */
	if (addfd.newfd && !(addfd.flags & SECCOMP_ADDFD_FLAG_SETFD))
		return -EINVAL;

	kaddfd.file = fget(addfd.srcfd);
	if (!kaddfd.file)
		return -EBADF;

/* …（同文件无关代码省略）… */
	 * We need to check again if the addfd request has been handled,
	 * and if not, we will remove it from the queue.
	 */
	if (list_empty(&kaddfd.list))
		ret = kaddfd.ret;
	else
		list_del(&kaddfd.list);

out_unlock:
	mutex_unlock(&filter->notify_lock);
/* …（同文件无关代码省略）… */
		return -EINVAL;

	/*
	 * The SECCOMP_FILTER_FLAG_WAIT_KILLABLE_SENT flag doesn't make sense
	 * without the SECCOMP_FILTER_FLAG_NEW_LISTENER flag.
	 */
	if ((flags & SECCOMP_FILTER_FLAG_WAIT_KILLABLE_RECV) &&
	    ((flags & SECCOMP_FILTER_FLAG_NEW_LISTENER) == 0))
		return -EINVAL;
