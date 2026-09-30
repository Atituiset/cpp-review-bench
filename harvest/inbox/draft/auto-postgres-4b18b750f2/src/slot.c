// AUTO-DRAFT from postgres/postgres PR #9510a826e4add77280e0a7a65f429563a4c4a2a8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
		MyReplicationSlot = NULL;
	}

	/* might not have been set when we've been a plain slot */
	LWLockAcquire(ProcArrayLock, LW_EXCLUSIVE);
	MyProc->statusFlags &= ~PROC_IN_LOGICAL_DECODING;
	ProcGlobal->statusFlags[MyProc->pgxactoff] = MyProc->statusFlags;
	LWLockRelease(ProcArrayLock);

	if (am_walsender)
	{
