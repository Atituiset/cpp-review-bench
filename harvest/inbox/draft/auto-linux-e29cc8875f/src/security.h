// AUTO-DRAFT from torvalds/linux PR #a804c5742bff224d4cb4d25dd383554d598be32b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <string.h>
  // <<< BUG ANCHOR
struct watch;
struct watch_notification;
struct lsm_ctx;

/* Default (no) options for the capable function */
#define CAP_OPT_NONE 0x0
/* …（同文件无关代码省略）… */

struct ctl_table;
struct audit_krule;
struct user_namespace;
struct timezone;

/* …（同文件无关代码省略）… */
struct lsm_prop {
	struct lsm_prop_selinux selinux;
	struct lsm_prop_smack smack;
	struct lsm_prop_apparmor apparmor;
	struct lsm_prop_bpf bpf;
};
/* …（同文件无关代码省略）… */
static inline void lsmprop_init(struct lsm_prop *prop)
{
	memset(prop, 0, sizeof(*prop));
}
/* …（同文件无关代码省略）… */
int security_inode_init_security_anon(struct inode *inode,
				      const struct qstr *name,
				      const struct inode *context_inode);
int security_inode_create(struct inode *dir, struct dentry *dentry, umode_t mode);
void security_inode_post_create_tmpfile(const struct mnt_idmap *idmap,
					struct inode *inode);
int security_inode_link(struct dentry *old_dentry, struct inode *dir,
			 struct dentry *new_dentry);
int security_inode_unlink(struct inode *dir, struct dentry *dentry);
int security_inode_symlink(struct inode *dir, struct dentry *dentry,
			   const char *old_name);
int security_inode_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode);
int security_inode_rmdir(struct inode *dir, struct dentry *dentry);
int security_inode_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev);
int security_inode_rename(struct inode *old_dir, struct dentry *old_dentry,
			  struct inode *new_dir, struct dentry *new_dentry,
			  unsigned int flags);
int security_inode_readlink(struct dentry *dentry);
int security_inode_follow_link(struct dentry *dentry, struct inode *inode,
			       bool rcu);
int security_inode_permission(struct inode *inode, int mask);
int security_inode_setattr(const struct mnt_idmap *idmap,
			   struct dentry *dentry, struct iattr *attr);
void security_inode_post_setattr(const struct mnt_idmap *idmap, struct dentry *dentry,
/* …（同文件无关代码省略）… */
			unsigned long arg4, unsigned long arg5);
void security_task_to_inode(struct task_struct *p, struct inode *inode);
int security_create_user_ns(const struct cred *cred);
int security_ipc_permission(struct kern_ipc_perm *ipcp, short flag);
void security_ipc_getlsmprop(struct kern_ipc_perm *ipcp, struct lsm_prop *prop);
int security_msg_msg_alloc(struct msg_msg *msg);
/* …（同文件无关代码省略）… */
static inline int security_inode_init_security_anon(struct inode *inode,
						    const struct qstr *name,
						    const struct inode *context_inode)
{
	return 0;
}

static inline int security_inode_create(struct inode *dir,
					 struct dentry *dentry,
					 umode_t mode)
{
	return 0;
}
/* …（同文件无关代码省略）… */
security_inode_post_create_tmpfile(const struct mnt_idmap *idmap, struct inode *inode)
{ }

static inline int security_inode_link(struct dentry *old_dentry,
				       struct inode *dir,
				       struct dentry *new_dentry)
{
	return 0;
}
/* …（同文件无关代码省略）… */
static inline int security_inode_unlink(struct inode *dir,
					 struct dentry *dentry)
{
	return 0;
}

static inline int security_inode_symlink(struct inode *dir,
					  struct dentry *dentry,
					  const char *old_name)
{
	return 0;
}

static inline int security_inode_mkdir(struct inode *dir,
					struct dentry *dentry,
					int mode)
{
	return 0;
}
/* …（同文件无关代码省略）… */
static inline int security_inode_rmdir(struct inode *dir,
					struct dentry *dentry)
{
	return 0;
}

static inline int security_inode_mknod(struct inode *dir,
					struct dentry *dentry,
					int mode, dev_t dev)
{
	return 0;
}
/* …（同文件无关代码省略）… */
static inline int security_inode_rename(struct inode *old_dir,
					 struct dentry *old_dentry,
					 struct inode *new_dir,
					 struct dentry *new_dentry,
					 unsigned int flags)
{
	return 0;
}
/* …（同文件无关代码省略）… */
static inline int security_inode_readlink(struct dentry *dentry)
{
	return 0;
}
/* …（同文件无关代码省略）… */
static inline int security_inode_follow_link(struct dentry *dentry,
					     struct inode *inode,
					     bool rcu)
{
	return 0;
}

static inline int security_inode_permission(struct inode *inode, int mask)
{
	return 0;
}
/* …（同文件无关代码省略）… */
static inline int security_inode_setattr(const struct mnt_idmap *idmap,
					 struct dentry *dentry,
					 struct iattr *attr)
{
	return 0;
}
/* …（同文件无关代码省略）… */
static inline void
security_inode_post_setattr(const struct mnt_idmap *idmap, struct dentry *dentry,
			    int ia_valid)
{ }
/* …（同文件无关代码省略）… */
static inline void security_task_to_inode(struct task_struct *p, struct inode *inode)
{ }
/* …（同文件无关代码省略）… */
static inline int security_create_user_ns(const struct cred *cred)
{
	return 0;
}

static inline int security_ipc_permission(struct kern_ipc_perm *ipcp,
					  short flag)
{
/* …（同文件无关代码省略）… */
static inline void security_ipc_getlsmprop(struct kern_ipc_perm *ipcp,
					   struct lsm_prop *prop)
{
	lsmprop_init(prop);
}
/* …（同文件无关代码省略）… */
static inline int security_msg_msg_alloc(struct msg_msg *msg)
{
	return 0;
}
