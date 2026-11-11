// AUTO-DRAFT from postgres/postgres PR #5bf919736b20ffdc8d646c33ed2d9054609b23ec
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
  // <<< BUG ANCHOR
	int			nelems;
	Datum	   *datum_flags;
	bool		interruptible = PG_GETARG_BOOL(4);

	memset(&worker, 0, sizeof(worker));
	worker.bgw_flags = BGWORKER_SHMEM_ACCESS |
/* …（同文件无关代码省略）… */
		worker.bgw_flags |= BGWORKER_INTERRUPTIBLE;

	worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
	worker.bgw_restart_time = BGW_NEVER_RESTART;
	sprintf(worker.bgw_library_name, "worker_spi");
	sprintf(worker.bgw_function_name, "worker_spi_main");
	snprintf(worker.bgw_name, BGW_MAXLEN, "worker_spi dynamic worker %d", i);
