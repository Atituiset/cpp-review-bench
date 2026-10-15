// AUTO-DRAFT from torvalds/linux PR #9a58da80053f992b285b6b7bebc694b0f284c443
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>
  // <<< BUG ANCHOR
	    memcmp(user->passkey, prev_user->passkey, user->passkey_sz))
		goto out;

	ksmbd_all_conn_set_status(prev_sess, KSMBD_SESS_NEED_RECONNECT);
	err = ksmbd_conn_wait_idle_sess(conn, prev_sess);
	if (err) {
		ksmbd_all_conn_set_status(prev_sess, KSMBD_SESS_NEED_SETUP);
		goto out;
	}
