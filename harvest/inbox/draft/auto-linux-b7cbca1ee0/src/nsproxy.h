// AUTO-DRAFT from torvalds/linux PR #f99005fbed4e8071110faa350cc5fa453afa1455
struct nsproxy {
	refcount_t count;
	struct uts_namespace *uts_ns;
	struct ipc_namespace *ipc_ns;
	struct mnt_namespace *mnt_ns;
	struct pid_namespace *pid_ns_for_children;
	struct net 	     *net_ns;
	struct time_namespace *time_ns;
	struct time_namespace *time_ns_for_children;
	struct cgroup_namespace *cgroup_ns;
};
/* …（同文件无关代码省略）… */
void switch_task_namespaces(struct task_struct *tsk, struct nsproxy *new);
int exec_task_namespaces(void);
void deactivate_nsproxy(struct nsproxy *ns);
int unshare_nsproxy_namespaces(unsigned long, struct nsproxy **,
	struct cred *, struct fs_struct *);
int __init nsproxy_cache_init(void);
