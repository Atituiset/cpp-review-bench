// AUTO-DRAFT from torvalds/linux PR #9a58da80053f992b285b6b7bebc694b0f284c443
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#include "mgmt/ksmbd_ida.h"
#include "mgmt/user_session.h"
#include "connection.h"
#include "compress.h"
#include "transport_tcp.h"
#include "transport_rdma.h"
/* …（同文件无关代码省略）… */
	spin_lock(&conn->request_lock);
	list_for_each_entry_safe(work, tmp, &conn->async_requests,
				 async_request_entry) {
		if (work->state != KSMBD_WORK_ACTIVE)
			continue;

		ksmbd_debug(CONN, "Cancel async request id %d\n",
			    work->async_id);
		work->state = KSMBD_WORK_CANCELLED;
		if (work->cancel_fn)
			work->cancel_fn(work->cancel_argv);
	}
/* …（同文件无关代码省略）… */
static bool ksmbd_session_is_bound_to_conn(struct ksmbd_session *sess,
					   struct ksmbd_conn *conn)
{
	bool found;

	rcu_read_lock();
	found = xa_load(&conn->sessions, sess->id) == sess;
	rcu_read_unlock();
	if (found)
		return true;

	down_read(&sess->chann_lock);
	found = xa_load(&sess->ksmbd_chann_list, (long)conn);
	up_read(&sess->chann_lock);
	return found;
}
/* …（同文件无关代码省略）… */
	if (retry_count >= max_timeout)
		return -EIO;

	down_read(&conn_list_lock);
	hash_for_each(conn_list, bkt, conn, hlist) {
		if (ksmbd_session_is_bound_to_conn(sess, conn)) {
