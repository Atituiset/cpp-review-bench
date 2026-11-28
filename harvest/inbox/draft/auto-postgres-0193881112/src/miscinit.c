// AUTO-DRAFT from postgres/postgres PR #b59ec01d944d5e6f054824ca19c116bf995d484f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

	 * But we don't unblock SIGQUIT or provide a default handler for it.
	 */
	pqinitmask();
	sigprocmask(SIG_SETMASK, &BlockSig, NULL);

	/* Compute paths, no postmaster to inherit from */
