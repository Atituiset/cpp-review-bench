// AUTO-DRAFT from postgres/postgres PR #1378aa13430e264a990a18597e9d8fdae740927e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
typedef enum
{
	VACUUM_ERRCB_PHASE_UNKNOWN,
	VACUUM_ERRCB_PHASE_SCAN_HEAP,
	VACUUM_ERRCB_PHASE_VACUUM_INDEX,
	VACUUM_ERRCB_PHASE_VACUUM_HEAP,
	VACUUM_ERRCB_PHASE_INDEX_CLEANUP,
	VACUUM_ERRCB_PHASE_TRUNCATE,
} VacErrPhase;
/* …（同文件无关代码省略）… */
typedef struct LVSavedErrInfo
{
	BlockNumber blkno;
	OffsetNumber offnum;
	VacErrPhase phase;
} LVSavedErrInfo;
/* …（同文件无关代码省略）… */
{
	IndexVacuumInfo ivinfo;
	LVSavedErrInfo saved_err_info;

	ivinfo.index = indrel;
	ivinfo.heaprel = vacrel->rel;
	ivinfo.analyze_only = false;
	ivinfo.is_autovacuum = AmAutoVacuumWorkerProcess();
	ivinfo.report_progress = false;
	ivinfo.estimated_count = true;
	ivinfo.message_level = DEBUG2;
	ivinfo.num_heap_tuples = reltuples;
	ivinfo.strategy = vacrel->bstrategy;

	/*
	 * Update error traceback information.
	 *
/* …（同文件无关代码省略）… */
	pfree(vacrel->indname);
	vacrel->indname = NULL;

	return istat;
}

/* …（同文件无关代码省略）… */
{
	IndexVacuumInfo ivinfo;
	LVSavedErrInfo saved_err_info;

	ivinfo.index = indrel;
	ivinfo.heaprel = vacrel->rel;
	ivinfo.analyze_only = false;
	ivinfo.is_autovacuum = AmAutoVacuumWorkerProcess();
	ivinfo.report_progress = false;
	ivinfo.estimated_count = estimated_count;
	ivinfo.message_level = DEBUG2;

	ivinfo.num_heap_tuples = reltuples;
	ivinfo.strategy = vacrel->bstrategy;

	/*
	 * Update error traceback information.
	 *
/* …（同文件无关代码省略）… */
	pfree(vacrel->indname);
	vacrel->indname = NULL;

	return istat;
}
