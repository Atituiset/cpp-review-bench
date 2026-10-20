// AUTO-DRAFT from torvalds/linux PR #f47d223627f80cb7a1eb14c585fe6cb35de5e984
  // <<< BUG ANCHOR
/* syscalls implemented in spufs */
struct file;
struct coredump_params;
struct spufs_calls {
	long (*create_thread)(const char __user *name,
					unsigned int flags, umode_t mode,
					struct file *neighbor);
	long (*spu_run)(struct file *filp, __u32 __user *unpc,
						__u32 __user *ustatus);
	int (*coredump_extra_notes_size)(void);
	int (*coredump_extra_notes_write)(struct coredump_params *cprm);
	void (*notify_spus_active)(void);
	struct module *owner;
};
