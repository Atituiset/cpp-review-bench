// AUTO-DRAFT from postgres/postgres PR #8becd1123c98cd9338678c6c2d4ae6b4d56a6fb9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR

static bool CheckForStandbyTrigger(void);
static void SetPromoteIsTriggered(void);
static bool HotStandbyActiveInReplay(void);

static void SetCurrentChunkStartTime(TimestampTz xtime);
static void SetLatestXTime(TimestampTz xtime);
/* …（同文件无关代码省略）… */
void
SetRecoveryPause(bool recoveryPause)
{
	SpinLockAcquire(&XLogRecoveryCtl->info_lck);

	if (!recoveryPause)
		XLogRecoveryCtl->recoveryPauseState = RECOVERY_NOT_PAUSED;
	else if (XLogRecoveryCtl->recoveryPauseState == RECOVERY_NOT_PAUSED)
		XLogRecoveryCtl->recoveryPauseState = RECOVERY_PAUSE_REQUESTED;

	SpinLockRelease(&XLogRecoveryCtl->info_lck);

	if (!recoveryPause)
		ConditionVariableBroadcast(&XLogRecoveryCtl->recoveryNotPausedCV);
}
/* …（同文件无关代码省略）… */
static void
SetPromoteIsTriggered(void)
{
	SpinLockAcquire(&XLogRecoveryCtl->info_lck);
	XLogRecoveryCtl->SharedPromoteIsTriggered = true;
	SpinLockRelease(&XLogRecoveryCtl->info_lck);

	/*
	 * Mark the recovery pause state as 'not paused' because the paused state
	 * ends and promotion continues if a promotion is triggered while recovery
	 * is paused. Otherwise pg_get_wal_replay_pause_state() can mistakenly
	 * return 'paused' while a promotion is ongoing.
	 */
	SetRecoveryPause(false);

	LocalPromoteIsTriggered = true;
}
/* …（同文件无关代码省略）… */
static bool
CheckForStandbyTrigger(void)
{
	if (LocalPromoteIsTriggered)
		return true;

	if (IsPromoteSignaled() && CheckPromoteSignal())
	{
		ereport(LOG, (errmsg("received promote request")));
		RemovePromoteSignalFiles();
		ResetPromoteSignaled();
		SetPromoteIsTriggered();
		return true;
	}

	return false;
}
/* …（同文件无关代码省略）… */
void
RemovePromoteSignalFiles(void)
{
	unlink(PROMOTE_SIGNAL_FILE);
}
/* …（同文件无关代码省略）… */
bool
CheckPromoteSignal(void)
{
	struct stat stat_buf;

	if (stat(PROMOTE_SIGNAL_FILE, &stat_buf) == 0)
		return true;

	return false;
}
/* …（同文件无关代码省略）… */
 * Like HotStandbyActive(), but to be used only in WAL replay code,
 * where we don't need to ask any other process what the state is.
 */
static bool
HotStandbyActiveInReplay(void)
{
	Assert(AmStartupProcess() || !IsPostmasterEnvironment);
/* …（同文件无关代码省略）… */
static void
SetLatestXTime(TimestampTz xtime)
{
	SpinLockAcquire(&XLogRecoveryCtl->info_lck);
	XLogRecoveryCtl->recoveryLastXTime = xtime;
	SpinLockRelease(&XLogRecoveryCtl->info_lck);
}
/* …（同文件无关代码省略）… */
static void
SetCurrentChunkStartTime(TimestampTz xtime)
{
	SpinLockAcquire(&XLogRecoveryCtl->info_lck);
	XLogRecoveryCtl->currentChunkStartTime = xtime;
	SpinLockRelease(&XLogRecoveryCtl->info_lck);
}
