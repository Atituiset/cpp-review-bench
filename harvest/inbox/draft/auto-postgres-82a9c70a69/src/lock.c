// AUTO-DRAFT from postgres/postgres PR #5bd2e236e21b0e158dda43c184260b9395ede4f4
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

	proc->waitLock = NULL;
	proc->waitProcLock = NULL;
	proc->waitStatus = PROC_WAIT_STATUS_ERROR;

	/*
	 * Delete the proclock immediately if it represents no already-held locks.
