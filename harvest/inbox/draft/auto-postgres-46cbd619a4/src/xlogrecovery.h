// AUTO-DRAFT from postgres/postgres PR #c2c696c1a4827cde5fdaf8c8f03f9991d43ebfad
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

	 */
	bool		SharedPromoteIsTriggered;

	/*
	 * recoveryWakeupLatch is used to wake up the startup process to continue
	 * WAL replay, if it is waiting for WAL to arrive or promotion to be
/* …（同文件无关代码省略）… */

extern bool PromoteIsTriggered(void);
extern bool CheckPromoteSignal(void);
extern void WakeupRecovery(void);

extern void StartupRequestWalReceiverRestart(void);
