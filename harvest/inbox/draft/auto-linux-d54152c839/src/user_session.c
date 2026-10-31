// AUTO-DRAFT from torvalds/linux PR #36aad21d50d1af3f6954bb0c988257a908202477
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <string.h>
  // <<< BUG ANCHOR
 */

#include <linux/atomic.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/rwsem.h>
/* …（同文件无关代码省略）… */
	atomic64_dec(&conn->session_count);
}

struct ksmbd_session_rpc {
	int			id;
	unsigned int		method;
};

#ifdef CONFIG_PROC_FS

static const struct ksmbd_const_name ksmbd_sess_cap_const_names[] = {
/* …（同文件无关代码省略）… */
	up_write(&sess->chann_lock);
}

static void __session_rpc_close(struct ksmbd_session *sess,
				struct ksmbd_session_rpc *entry)
{
	struct ksmbd_rpc_command *resp;

	resp = ksmbd_rpc_close(sess, entry->id);
	if (!resp)
		pr_err("Unable to close RPC pipe %d\n", entry->id);

	kvfree(resp);
	ksmbd_rpc_id_free(entry->id);
	kfree(entry);
}

static void ksmbd_session_rpc_clear_list(struct ksmbd_session *sess)
{
	struct ksmbd_session_rpc *entry;
	long index;

	down_write(&sess->rpc_lock);
	xa_for_each(&sess->rpc_handle_list, index, entry) {
		xa_erase(&sess->rpc_handle_list, index);
		__session_rpc_close(sess, entry);
	}
	up_write(&sess->rpc_lock);

	xa_destroy(&sess->rpc_handle_list);
}

static int __rpc_method(char *rpc_name)
{
	if (!strcmp(rpc_name, "\\srvsvc") || !strcmp(rpc_name, "srvsvc"))
		return KSMBD_RPC_SRVSVC_METHOD_INVOKE;

	if (!strcmp(rpc_name, "\\wkssvc") || !strcmp(rpc_name, "wkssvc"))
		return KSMBD_RPC_WKSSVC_METHOD_INVOKE;

	if (!strcmp(rpc_name, "LANMAN") || !strcmp(rpc_name, "lanman"))
		return KSMBD_RPC_RAP_METHOD;

	if (!strcmp(rpc_name, "\\samr") || !strcmp(rpc_name, "samr"))
		return KSMBD_RPC_SAMR_METHOD_INVOKE;

	if (!strcmp(rpc_name, "\\lsarpc") || !strcmp(rpc_name, "lsarpc"))
		return KSMBD_RPC_LSARPC_METHOD_INVOKE;

	if (!strcmp(rpc_name, "\\mdssvc") || !strcmp(rpc_name, "mdssvc"))
		return -ENOENT;

	pr_err("Unsupported RPC: %s\n", rpc_name);
	return -ENOENT;
}

int ksmbd_session_rpc_open(struct ksmbd_session *sess, char *rpc_name)
{
	struct ksmbd_session_rpc *entry, *old;
	struct ksmbd_rpc_command *resp;
	int method, id;

	method = __rpc_method(rpc_name);
	if (method < 0)
/* …（同文件无关代码省略）… */

	entry->method = method;
	entry->id = id = ksmbd_ipc_id_alloc();
	if (id < 0)
		goto free_entry;

	down_write(&sess->rpc_lock);
	old = xa_store(&sess->rpc_handle_list, id, entry, KSMBD_DEFAULT_GFP);
	if (xa_is_err(old)) {
		up_write(&sess->rpc_lock);
		goto free_id;
	}

	resp = ksmbd_rpc_open(sess, id);
	if (!resp) {
		xa_erase(&sess->rpc_handle_list, entry->id);
		up_write(&sess->rpc_lock);
		goto free_id;
	}

	up_write(&sess->rpc_lock);
	kvfree(resp);
	return id;
free_id:
	ksmbd_rpc_id_free(entry->id);
free_entry:
	kfree(entry);
	return -EINVAL;
}

void ksmbd_session_rpc_close(struct ksmbd_session *sess, int id)
{
	struct ksmbd_session_rpc *entry;

	down_write(&sess->rpc_lock);
	entry = xa_erase(&sess->rpc_handle_list, id);
	if (entry)
		__session_rpc_close(sess, entry);
	up_write(&sess->rpc_lock);
}

int ksmbd_session_rpc_method(struct ksmbd_session *sess, int id)
{
	struct ksmbd_session_rpc *entry;

	lockdep_assert_held(&sess->rpc_lock);
	entry = xa_load(&sess->rpc_handle_list, id);

	return entry ? entry->method : 0;
}
/* …（同文件无关代码省略）… */
void ksmbd_user_session_get(struct ksmbd_session *sess)
{
	atomic_inc(&sess->refcnt);
}
/* …（同文件无关代码省略）… */
		up_write(&prev_sess->chann_lock);
		goto out;
	}
	prev_sess->tearing_down = true;
	up_write(&prev_sess->chann_lock);
	ksmbd_user_session_get(prev_sess);
	pinned = true;
/* …（同文件无关代码省略）… */
	down_write(&conn->session_lock);
	if (err) {
		down_write(&prev_sess->chann_lock);
		prev_sess->tearing_down = false;
		up_write(&prev_sess->chann_lock);
		ksmbd_all_conn_set_status(prev_sess, KSMBD_SESS_GOOD);
		goto out;
