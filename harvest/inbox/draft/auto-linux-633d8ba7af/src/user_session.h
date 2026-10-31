// AUTO-DRAFT from torvalds/linux PR #36aad21d50d1af3f6954bb0c988257a908202477
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

#define __USER_SESSION_MANAGEMENT_H__

#include <linux/hashtable.h>
#include <linux/xarray.h>

#include "../smb_common.h"
/* …（同文件无关代码省略）… */
#define PREAUTH_HASHVALUE_SIZE		64

struct ksmbd_file_table;

struct channel {
	char			sess_key[CIFS_KEY_SIZE];
/* …（同文件无关代码省略）… */
struct ksmbd_session {
	u64				id;

	__u16				dialect;
	char				ClientGUID[SMB2_CLIENT_GUID_SIZE];

	struct ksmbd_user		*user;
	/* Holds the connection slot until an SMB3 channel takes it over. */
	struct ksmbd_conn		*setup_conn;
	unsigned int			sequence_number;
	unsigned int			flags;

	bool				sign;
	bool				enc;
	bool				tearing_down;

	int				state;
	__u8				*Preauth_HashValue;

	char				sess_key[CIFS_KEY_SIZE];
	u64				kerberos_expiry;

	struct hlist_node		hlist;
	struct list_head		dispose_entry;
	struct rw_semaphore		chann_lock;
	struct xarray			ksmbd_chann_list;
	struct xarray			tree_conns;
	struct ida			tree_conn_ida;
	struct xarray			rpc_handle_list;

	__u8				smb3encryptionkey[SMB3_ENC_DEC_KEY_SIZE];
	__u8				smb3decryptionkey[SMB3_ENC_DEC_KEY_SIZE];
	__u8				smb3signingkey[SMB3_SIGN_KEY_SIZE];

	struct ksmbd_file_table		file_table;
	unsigned long			last_active;
	struct rw_semaphore		tree_conns_lock;

#ifdef CONFIG_PROC_FS
	struct proc_dir_entry		*proc_entry;
#endif
	atomic_t			refcnt;
	struct rw_semaphore		rpc_lock;
};
/* …（同文件无关代码省略）… */

int ksmbd_session_rpc_open(struct ksmbd_session *sess, char *rpc_name);
void ksmbd_session_rpc_close(struct ksmbd_session *sess, int id);
int ksmbd_session_rpc_method(struct ksmbd_session *sess, int id);
void ksmbd_user_session_get(struct ksmbd_session *sess);
void ksmbd_user_session_put(struct ksmbd_session *sess);
int create_proc_sessions(void);
