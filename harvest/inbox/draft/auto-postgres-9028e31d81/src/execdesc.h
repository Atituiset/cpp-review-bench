// AUTO-DRAFT from postgres/postgres PR #e5d5d1ad1e3aa495609e19fa716dcea6a39db1bb
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

typedef struct QueryDesc
{
	/* These fields are provided by CreateQueryDesc */
	CmdType		operation;		/* CMD_SELECT, CMD_UPDATE, etc. */
	PlannedStmt *plannedstmt;	/* planner's output (could be utility, too) */
	const char *sourceText;		/* source text of the query */
	Snapshot	snapshot;		/* snapshot to use for query */
	Snapshot	crosscheck_snapshot;	/* crosscheck for RI update/delete */
	DestReceiver *dest;			/* the destination for tuple output */
	ParamListInfo params;		/* param values being passed in */
	QueryEnvironment *queryEnv; /* query environment passed in */
	int			instrument_options; /* OR of InstrumentOption flags */
	int			query_instr_options;	/* OR of InstrumentOption flags for
										 * query_instr */

	/* These fields are set by ExecutorStart */
	TupleDesc	tupDesc;		/* descriptor for result tuples */
	EState	   *estate;			/* executor's query-wide state */
	PlanState  *planstate;		/* tree of per-plan-node state */

	/* This field is set by ExecutePlan */
	bool		already_executed;	/* true if previously executed */

	/* This field is allocated by ExecutorStart if needed */
	struct Instrumentation *query_instr;	/* query level instrumentation */
} QueryDesc;

/* in pquery.c */
/* …（同文件无关代码省略）… */

extern void FreeQueryDesc(QueryDesc *qdesc);

#endif							/* EXECDESC_H  */
