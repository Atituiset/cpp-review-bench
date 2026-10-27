// AUTO-DRAFT from postgres/postgres PR #e5d5d1ad1e3aa495609e19fa716dcea6a39db1bb
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "executor/tstoreReceiver.h"
#include "miscadmin.h"
#include "pg_trace.h"
#include "tcop/pquery.h"
#include "tcop/utility.h"
#include "utils/memutils.h"
#include "utils/snapmgr.h"


/*
 * ActivePortal is the currently executing Portal (the most closely nested,
 * if there are several).
 */
Portal		ActivePortal = NULL;


static void ProcessQuery(PlannedStmt *plan,
						 const char *sourceText,
/* …（同文件无关代码省略）… */
QueryDesc *
CreateQueryDesc(PlannedStmt *plannedstmt,
				const char *sourceText,
				Snapshot snapshot,
				Snapshot crosscheck_snapshot,
				DestReceiver *dest,
				ParamListInfo params,
				QueryEnvironment *queryEnv,
				int instrument_options)
{
	QueryDesc  *qd = palloc_object(QueryDesc);

	qd->operation = plannedstmt->commandType;	/* operation */
	qd->plannedstmt = plannedstmt;	/* plan */
	qd->sourceText = sourceText;	/* query text */
	qd->snapshot = RegisterSnapshot(snapshot);	/* snapshot */
	/* RI check snapshot */
	qd->crosscheck_snapshot = RegisterSnapshot(crosscheck_snapshot);
	qd->dest = dest;			/* output dest */
	qd->params = params;		/* parameter values passed into query */
	qd->queryEnv = queryEnv;
	qd->instrument_options = instrument_options;	/* instrumentation wanted? */
	qd->query_instr_options = 0;

	/* null these fields until set by ExecutorStart */
	qd->tupDesc = NULL;
	qd->estate = NULL;
	qd->planstate = NULL;
	qd->query_instr = NULL;

	/* not yet executed */
	qd->already_executed = false;

	return qd;
}

/* …（同文件无关代码省略）… */
void
FreeQueryDesc(QueryDesc *qdesc)
{
	/* Can't be a live query */
	Assert(qdesc->estate == NULL);

	/* forget our snapshots */
	UnregisterSnapshot(qdesc->snapshot);
	UnregisterSnapshot(qdesc->crosscheck_snapshot);

	/* Only the QueryDesc itself need be freed */
	pfree(qdesc);
}


/*
 * ProcessQuery
/* …（同文件无关代码省略）… */
static void
ProcessQuery(PlannedStmt *plan,
			 const char *sourceText,
			 ParamListInfo params,
			 QueryEnvironment *queryEnv,
			 DestReceiver *dest,
			 QueryCompletion *qc)
{
	QueryDesc  *queryDesc;

	/*
	 * Create the QueryDesc object
	 */
	queryDesc = CreateQueryDesc(plan, sourceText,
								GetActiveSnapshot(), InvalidSnapshot,
								dest, params, queryEnv, 0);

	/*
	 * Call ExecutorStart to prepare the plan for execution
	 */
	ExecutorStart(queryDesc, 0);

	/*
	 * Run the plan to completion.
	 */
	ExecutorRun(queryDesc, ForwardScanDirection, 0);

	/*
	 * Build command completion status data, if caller wants one.
	 */
	if (qc)
	{
		CommandTag	tag;

		if (queryDesc->operation == CMD_SELECT)
			tag = CMDTAG_SELECT;
		else if (queryDesc->operation == CMD_INSERT)
			tag = CMDTAG_INSERT;
		else if (queryDesc->operation == CMD_UPDATE)
			tag = CMDTAG_UPDATE;
		else if (queryDesc->operation == CMD_DELETE)
			tag = CMDTAG_DELETE;
		else if (queryDesc->operation == CMD_MERGE)
			tag = CMDTAG_MERGE;
		else
			tag = CMDTAG_UNKNOWN;

		SetQueryCompletion(qc, tag, queryDesc->estate->es_processed);
	}

	/*
	 * Now, we close down all the scans and free allocated resources.
	 */
	ExecutorFinish(queryDesc);
	ExecutorEnd(queryDesc);

	FreeQueryDesc(queryDesc);
}
