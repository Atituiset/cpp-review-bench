// AUTO-DRAFT from torvalds/linux PR #79132ddf81d78d9b84d9b8f05c7c5c3a3e912537
#define SECCOMP_FILTER_FLAG_TSYNC_ESRCH		(1UL << 4)
/* Received notifications wait in killable state (only respond to fatal signals) */
#define SECCOMP_FILTER_FLAG_WAIT_KILLABLE_RECV	(1UL << 5)

/*
 * All BPF programs must return a 32-bit value.
