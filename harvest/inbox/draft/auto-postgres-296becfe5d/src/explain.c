// AUTO-DRAFT from postgres/postgres PR #c6b8af429b3592b00d3240a24c5a27077ff0dd82
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define BYTES_TO_KILOBYTES(b) (((b) + 1023) / 1024)
/* …（同文件无关代码省略）… */
static void show_window_keys(StringInfo buf, PlanState *planstate,
							 int nkeys, AttrNumber *keycols,
							 List *ancestors, ExplainState *es);
static void show_storage_info(char *maxStorageType, int64 maxSpaceUsed,
							  ExplainState *es);
static void show_tablesample(TableSampleClause *tsc, PlanState *planstate,
							 List *ancestors, ExplainState *es);
/* …（同文件无关代码省略）… */
static void
show_window_keys(StringInfo buf, PlanState *planstate,
				 int nkeys, AttrNumber *keycols,
				 List *ancestors, ExplainState *es)
{
	Plan	   *plan = planstate->plan;
	List	   *context;
	bool		useprefix;

	/* Set up deparsing context */
	context = set_deparse_context_plan(es->deparse_cxt,
									   plan,
									   ancestors);
	useprefix = (es->rtable_size > 1 || es->verbose);

	for (int keyno = 0; keyno < nkeys; keyno++)
	{
		/* find key expression in tlist */
		AttrNumber	keyresno = keycols[keyno];
		TargetEntry *target = get_tle_by_resno(plan->targetlist,
											   keyresno);
		char	   *exprstr;

		if (!target)
			elog(ERROR, "no tlist entry for key %d", keyresno);
		/* Deparse the expression, showing any top-level cast */
		exprstr = deparse_expression((Node *) target->expr, context,
									 useprefix, true);
		if (keyno > 0)
			appendStringInfoString(buf, ", ");
		appendStringInfoString(buf, exprstr);
		pfree(exprstr);

		/*
		 * We don't attempt to provide sort order information because
		 * WindowAgg carries equality operators not comparison operators;
		 * compare show_agg_keys.
		 */
	}
}
/* …（同文件无关代码省略）… */
 * Show information on storage method and maximum memory/disk space used.
 */
static void
show_storage_info(char *maxStorageType, int64 maxSpaceUsed, ExplainState *es)
{
	int64		maxSpaceUsedKB = BYTES_TO_KILOBYTES(maxSpaceUsed);

	if (es->format != EXPLAIN_FORMAT_TEXT)
	{
		ExplainPropertyText("Storage", maxStorageType, es);
		ExplainPropertyInteger("Maximum Storage", "kB", maxSpaceUsedKB, es);
	}
	else
	{
		ExplainIndentText(es);
		appendStringInfo(es->str,
						 "Storage: %s  Maximum Storage: " INT64_FORMAT "kB\n",
						 maxStorageType,
						 maxSpaceUsedKB);
	}
}
/* …（同文件无关代码省略）… */
static void
show_tablesample(TableSampleClause *tsc, PlanState *planstate,
				 List *ancestors, ExplainState *es)
{
	List	   *context;
	bool		useprefix;
	char	   *method_name;
	List	   *params = NIL;
	char	   *repeatable;
	ListCell   *lc;

	/* Set up deparsing context */
	context = set_deparse_context_plan(es->deparse_cxt,
									   planstate->plan,
									   ancestors);
	useprefix = es->rtable_size > 1;

	/* Get the tablesample method name */
	method_name = get_func_name(tsc->tsmhandler);

	/* Deparse parameter expressions */
	foreach(lc, tsc->args)
	{
		Node	   *arg = (Node *) lfirst(lc);

		params = lappend(params,
						 deparse_expression(arg, context,
											useprefix, false));
	}
	if (tsc->repeatable)
		repeatable = deparse_expression((Node *) tsc->repeatable, context,
										useprefix, false);
	else
		repeatable = NULL;

	/* Print results */
	if (es->format == EXPLAIN_FORMAT_TEXT)
	{
		bool		first = true;

		ExplainIndentText(es);
		appendStringInfo(es->str, "Sampling: %s (", method_name);
		foreach(lc, params)
		{
			if (!first)
				appendStringInfoString(es->str, ", ");
			appendStringInfoString(es->str, (const char *) lfirst(lc));
			first = false;
		}
		appendStringInfoChar(es->str, ')');
		if (repeatable)
			appendStringInfo(es->str, " REPEATABLE (%s)", repeatable);
		appendStringInfoChar(es->str, '\n');
	}
	else
	{
		ExplainPropertyText("Sampling Method", method_name, es);
		ExplainPropertyList("Sampling Parameters", params, es);
		if (repeatable)
			ExplainPropertyText("Repeatable Seed", repeatable, es);
	}
}
/* …（同文件无关代码省略）… */
static void
show_material_info(MaterialState *mstate, ExplainState *es)
{
	char	   *maxStorageType;
	int64		maxSpaceUsed;

	Tuplestorestate *tupstore = mstate->tuplestorestate;
/* …（同文件无关代码省略）… */
	if (!es->analyze || tupstore == NULL)
		return;

	tuplestore_get_stats(tupstore, &maxStorageType, &maxSpaceUsed);
	show_storage_info(maxStorageType, maxSpaceUsed, es);
}

/*
/* …（同文件无关代码省略）… */
static void
show_windowagg_info(WindowAggState *winstate, ExplainState *es)
{
	char	   *maxStorageType;
	int64		maxSpaceUsed;

	Tuplestorestate *tupstore = winstate->buffer;
/* …（同文件无关代码省略）… */
	if (!es->analyze || tupstore == NULL)
		return;

	tuplestore_get_stats(tupstore, &maxStorageType, &maxSpaceUsed);
	show_storage_info(maxStorageType, maxSpaceUsed, es);
}

/*
/* …（同文件无关代码省略）… */
static void
show_ctescan_info(CteScanState *ctescanstate, ExplainState *es)
{
	char	   *maxStorageType;
	int64		maxSpaceUsed;

	Tuplestorestate *tupstore = ctescanstate->leader->cte_table;

	if (!es->analyze || tupstore == NULL)
		return;

	tuplestore_get_stats(tupstore, &maxStorageType, &maxSpaceUsed);
	show_storage_info(maxStorageType, maxSpaceUsed, es);
}

/*
/* …（同文件无关代码省略）… */
static void
show_table_func_scan_info(TableFuncScanState *tscanstate, ExplainState *es)
{
	char	   *maxStorageType;
	int64		maxSpaceUsed;

	Tuplestorestate *tupstore = tscanstate->tupstore;

	if (!es->analyze || tupstore == NULL)
		return;

	tuplestore_get_stats(tupstore, &maxStorageType, &maxSpaceUsed);
	show_storage_info(maxStorageType, maxSpaceUsed, es);
}

/*
/* …（同文件无关代码省略）… */
static void
show_recursive_union_info(RecursiveUnionState *rstate, ExplainState *es)
{
	char	   *maxStorageType,
			   *tempStorageType;
	int64		maxSpaceUsed,
				tempSpaceUsed;

/* …（同文件无关代码省略）… */
	 * from one of them which consumed more memory/disk than the other.  The
	 * storage size is sum of the two.
	 */
	tuplestore_get_stats(rstate->working_table, &tempStorageType,
						 &tempSpaceUsed);
	tuplestore_get_stats(rstate->intermediate_table, &maxStorageType,
						 &maxSpaceUsed);

	if (tempSpaceUsed > maxSpaceUsed)
		maxStorageType = tempStorage
