// AUTO-DRAFT from postgres/postgres PR #8becd1123c98cd9338678c6c2d4ae6b4d56a6fb9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

typedef enum RecoveryPauseState
{
	RECOVERY_NOT_PAUSED,		/* pause not requested */
	RECOVERY_PAUSE_REQUESTED,	/* pause requested, but not yet paused */
	RECOVERY_PAUSED,			/* recovery is paused */
} RecoveryPauseState;
/* …（同文件无关代码省略）… */
extern void RemovePromoteSignalFiles(void);

extern bool HotStandbyActive(void);
extern XLogRecPtr GetXLogReplayRecPtr(TimeLineID *replayTLI);
extern RecoveryPauseState GetRecoveryPauseState(void);
extern void SetRecoveryPause(bool recoveryPause);
