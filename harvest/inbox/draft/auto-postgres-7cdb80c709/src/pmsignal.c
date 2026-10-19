// AUTO-DRAFT from postgres/postgres PR #22ff4fc5a21bce3ccf0376e0e91fde8fd2f28655
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
struct PMSignalData
{
	/* per-reason flags for signaling the postmaster */
	sig_atomic_t PMSignalFlags[NUM_PMSIGNALS];
	/* global flags for signals from postmaster to children */
	QuitSignalReason sigquit_reason;	/* why SIGQUIT was sent */
	/* per-child-process flags */
	int			num_child_flags;	/* # of entries in PMChildFlags[] */
	sig_atomic_t PMChildFlags[FLEXIBLE_ARRAY_MEMBER];
};

/* PMSignalState pointer is valid in both postmaster and child processes */
NON_EXEC_STATIC volatile PMSignalData *PMSignalState = NULL;

static void PMSignalShmemRequest(void *);
static void PMSignalShmemInit(void *);
/* …（同文件无关代码省略）… */
static void
PMSignalShmemRequest(void *arg)
{
	size_t		size;

	num_child_flags = MaxLivePostmasterChildren();

	size = add_size(offsetof(PMSignalData, PMChildFlags),
					mul_size(num_child_flags, sizeof(sig_atomic_t)));
	ShmemRequestStruct(.name = "PMSignalState",
					   .size = size,
					   .ptr = (void **) &PMSignalState,
		);
}
/* …（同文件无关代码省略）… */
static void
PMSignalShmemInit(void *arg)
{
	Assert(PMSignalState);
	Assert(num_child_flags > 0);
	PMSignalState->num_child_flags = num_child_flags;
}
