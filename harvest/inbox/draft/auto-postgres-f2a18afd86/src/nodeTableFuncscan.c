// AUTO-DRAFT from postgres/postgres PR #a7f8cd811d65d1a51be87bdb890d7ebf7d49aa45
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
	scanslot = node->ss.ss_ScanTupleSlot;

	/*
	 * If first time through, read all tuples from function and put them in a
	 * tuplestore. Subsequent calls just fetch tuples from tuplestore.
	 */
	if (node->tupstore == NULL)
		tfuncFetchRows(node, node->ss.ps.ps_ExprContext);

	/*
	 * Get the next tuple from tuplestore.
/* …（同文件无关代码省略）… */
							  "TableFunc per value context",
							  ALLOCSET_DEFAULT_SIZES);
	scanstate->opaque = NULL;	/* initialized at runtime */

	scanstate->ns_names = tf->ns_names;

/* …（同文件无关代码省略）… */
	ExecScanReScan(&node->ss);

	/*
	 * Recompute when parameters are changed.
	 */
	if (chgparam)
	{
		if (node->tupstore != NULL)
		{
			tuplestore_end(node->tupstore);
			node->tupstore = NULL;
		}
	}

	if (node->tupstore != NULL)
		tuplestore_rescan(node->tupstore);
}

/* …（同文件无关代码省略）… */
static void
tfuncFetchRows(TableFuncScanState *tstate, ExprContext *econtext)
{
	const TableFuncRoutine *routine = tstate->routine;
	MemoryContext oldcxt;
	Datum		value;
	bool		isnull;

	Assert(tstate->opaque == NULL);

	/* build tuplestore for the result */
	oldcxt = MemoryContextSwitchTo(econtext->ecxt_per_query_memory);
	tstate->tupstore = tuplestore_begin_heap(false, false, work_mem);

	/*
	 * Each call to fetch a new set of rows - of which there may be very many
	 * if XMLTABLE or JSON_TABLE is being used in a lateral join - will
	 * allocate a possibly substantial amount of memory, so we cannot use the
	 * per-query context here. perTableCxt now serves the same function as
	 * "argcontext" does in FunctionScan - a place to store per-one-call (i.e.
	 * one result table) lifetime data (as opposed to per-query or
	 * per-result-tuple).
	 */
	MemoryContextSwitchTo(tstate->perTableCxt);

	PG_TRY();
	{
		routine->InitOpaque(tstate,
							tstate->ss.ss_ScanTupleSlot->tts_tupleDescriptor->natts);

		/*
		 * If evaluating the document expression returns NULL, the table
		 * expression is empty and we return immediately.
		 */
		value = ExecEvalExpr(tstate->docexpr, econtext, &isnull);

		if (!isnull)
		{
			/* otherwise, pass the document value to the table builder */
			tfuncInitialize(tstate, econtext, value);

			/* initialize ordinality counter */
			tstate->ordinal = 1;

			/* Load all rows into the tuplestore, and we're done */
			tfuncLoadRows(tstate, econtext);
		}
	}
	PG_CATCH();
	{
		if (tstate->opaque != NULL)
			routine->DestroyOpaque(tstate);
		PG_RE_THROW();
	}
	PG_END_TRY();

	/* clean up and return to original memory context */

	if (tstate->opaque != NULL)
	{
		routine->DestroyOpaque(tstate);
		tstate->opaque = NULL;
	}

	MemoryContextSwitchTo(oldcxt);
	MemoryContextReset(tstate->perTableCxt);
}
/* …（同文件无关代码省略）… */
static void
tfuncInitialize(TableFuncScanState *tstate, ExprContext *econtext, Datum doc)
{
	const TableFuncRoutine *routine = tstate->routine;
	TupleDesc	tupdesc;
	ListCell   *lc1,
			   *lc2;
	bool		isnull;
	int			colno;
	Datum		value;
	int			ordinalitycol =
		((TableFuncScan *) (tstate->ss.ps.plan))->tablefunc->ordinalitycol;

	/*
	 * Install the document as a possibly-toasted Datum into the tablefunc
	 * context.
	 */
	routine->SetDocument(tstate, doc);

	/* Evaluate namespace specifications */
	forboth(lc1, tstate->ns_uris, lc2, tstate->ns_names)
	{
		ExprState  *expr = (ExprState *) lfirst(lc1);
		String	   *ns_node = lfirst_node(String, lc2);
		char	   *ns_uri;
		char	   *ns_name;

		value = ExecEvalExpr(expr, econtext, &isnull);
		if (isnull)
			ereport(ERROR,
					(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
					 errmsg("namespace URI must not be null")));
		ns_uri = TextDatumGetCString(value);

		/* DEFAULT is passed down to SetNamespace as NULL */
		ns_name = ns_node ? strVal(ns_node) : NULL;

		routine->SetNamespace(tstate, ns_name, ns_uri);
	}

	/*
	 * Install the row filter expression, if any, into the table builder
	 * context.
	 */
	if (routine->SetRowFilter)
	{
		value = ExecEvalExpr(tstate->rowexpr, econtext, &isnull);
		if (isnull)
			ereport(ERROR,
					(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
					 errmsg("row filter expression must not be null")));

		routine->SetRowFilter(tstate, TextDatumGetCString(value));
	}

	/*
	 * Install the column filter expressions into the table builder context.
	 * If an expression is given, use that; otherwise the column name itself
	 * is the column filter.
	 */
	colno = 0;
	tupdesc = tstate->ss.ss_ScanTupleSlot->tts_tupleDescriptor;
	foreach(lc1, tstate->colexprs)
	{
		char	   *colfilter;
		Form_pg_attribute att = TupleDescAttr(tupdesc, colno);

		if (colno != ordinalitycol)
		{
			ExprState  *colexpr = lfirst(lc1);

			if (colexpr != NULL)
			{
				value = ExecEvalExpr(colexpr, econtext, &isnull);
				if (isnull)
					ereport(ERROR,
							(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
							 errmsg("column filter expression must not be null"),
							 errdetail("Filter for column \"%s\" is null.",
									   NameStr(att->attname))));
				colfilter = TextDatumGetCString(value);
			}
			else
				colfilter = NameStr(att->attname);

			routine->SetColumnFilter(tstate, colfilter, colno);
		}

		colno++;
	}
}
/* …（同文件无关代码省略）… */
static void
tfuncLoadRows(TableFuncScanState *tstate, ExprContext *econtext)
{
	const TableFuncRoutine *routine = tstate->routine;
	TupleTableSlot *slot = tstate->ss.ss_ScanTupleSlot;
	TupleDesc	tupdesc = slot->tts_tupleDescriptor;
	Datum	   *values = slot->tts_values;
	bool	   *nulls = slot->tts_isnull;
	int			natts = tupdesc->natts;
	MemoryContext oldcxt;
	int			ordinalitycol;

	ordinalitycol =
		((TableFuncScan *) (tstate->ss.ps.plan))->tablefunc->ordinalitycol;

	/*
	 * We need a short-lived memory context that we can clean up each time
	 * around the loop, to avoid wasting space. Our default per-tuple context
	 * is fine for the job, since we won't have used it for anything yet in
	 * this tuple cycle.
	 */
	oldcxt = MemoryContextSwitchTo(econtext->ecxt_per_tuple_memory);

	/
