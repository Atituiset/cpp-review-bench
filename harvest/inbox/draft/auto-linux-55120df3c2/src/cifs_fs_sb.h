// AUTO-DRAFT from torvalds/linux PR #3db1c74a20f0552fadc2c6886a43feee8894fa19
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

	atomic_t active;
	atomic_t mnt_cifs_flags;
	atomic_t outstanding_rreq;	/* nr of rreqs not yet fully deinitialized */
	struct delayed_work prune_tlinks;
	struct rcu_head rcu;

/* …（同文件无关代码省略）… */
	 * (cifs_autodisable_serverino) in order to match new mounts.
	 */
	bool mnt_cifs_serverino_autodisabled;
	/*
	 * Available once the mount has completed.
	 */
