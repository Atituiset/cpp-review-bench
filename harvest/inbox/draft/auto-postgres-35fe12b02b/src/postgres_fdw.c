// AUTO-DRAFT from postgres/postgres PR #11591f273e6a26fb55728b410538fa2627b19b28
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct PgFdwScanState
{
	Relation	rel;			/* relcache entry for the foreign table. NULL
								 * for a foreign join scan. */
	TupleDesc	tupdesc;		/* tuple descriptor of scan */
	AttInMetadata *attinmeta;	/* attribute datatype conversion metadata */

	/* extracted fdw_private data */
	char	   *query;			/* text of SELECT command */
	List	   *retrieved_attrs;	/* list of retrieved attribute numbers */

	/* for remote query execution */
	PGconn	   *conn;			/* connection for the scan */
	PgFdwConnState *conn_state; /* extra per-connection state */
	unsigned int cursor_number; /* quasi-unique ID for my cursor */
	bool		cursor_exists;	/* have we created the cursor? */
	int			numParams;		/* number of parameters passed to query */
	FmgrInfo   *param_flinfo;	/* output conversion functions for them */
	List	   *param_exprs;	/* executable expressions for param values */
	const char **param_values;	/* textual values of query parameters */

	/* for storing result tuples */
	HeapTuple  *tuples;			/* array of currently-retrieved tuples */
	int			num_tuples;		/* # of tuples in array */
	int			next_tuple;		/* index of next one to return */

	/* batch-level state, for optimizing rewinds and avoiding useless fetch */
	int			fetch_ct_2;		/* Min(# of fetches done, 2) */
	bool		eof_reached;	/* true if last fetch reached EOF */

	/* for asynchronous execution */
	bool		async_capable;	/* engage asynchronous-capable logic? */

	/* working memory contexts */
	MemoryContext batch_cxt;	/* context holding current batch of tuples */
	MemoryContext temp_cxt;		/* context for per-tuple temporary data */

	int			fetch_size;		/* number of tuples per fetch */
} PgFdwScanState;
/* …（同文件无关代码省略）… */
typedef struct ConversionLocation
{
	AttrNumber	cur_attno;		/* attribute number being processed, or 0 */
	Relation	rel;			/* foreign table being processed, or NULL */
	ForeignScanState *fsstate;	/* plan node being processed, or NULL */
} ConversionLocation;
/* …（同文件无关代码省略）… */

	/*
	 * If the node is async-capable, any asynchronous fetch made for it should
	 * have been processed before we get here (see ExecAppendAsyncReset()).
	 */
	Assert(!fsstate->async_capable || !fsstate->conn_state->pendingAreq ||
		   fsstate->conn_state->pendingAreq->requestee != (PlanState *) node);
/* …（同文件无关代码省略）… */
static void
create_cursor(ForeignScanState *node)
{
	PgFdwScanState *fsstate = (PgFdwScanState *) node->fdw_state;
	ExprContext *econtext = node->ss.ps.ps_ExprContext;
	int			numParams = fsstate->numParams;
	const char **values = fsstate->param_values;
	PGconn	   *conn = fsstate->conn;
	StringInfoData buf;
	PGresult   *res;

	/* First, process a pending asynchronous request, if any. */
	if (fsstate->conn_state->pendingAreq)
		process_pending_request(fsstate->conn_state->pendingAreq);

	/*
	 * Construct array of query parameter values in text format.  We do the
	 * conversions in the short-lived per-tuple context, so as not to cause a
	 * memory leak over repeated scans.
	 */
	if (numParams > 0)
	{
		MemoryContext oldcontext;

		oldcontext = MemoryContextSwitchTo(econtext->ecxt_per_tuple_memory);

		process_query_params(econtext,
							 fsstate->param_flinfo,
							 fsstate->param_exprs,
							 values);

		MemoryContextSwitchTo(oldcontext);
	}

	/* Construct the DECLARE CURSOR command */
	initStringInfo(&buf);
	appendStringInfo(&buf, "DECLARE c%u CURSOR FOR\n%s",
					 fsstate->cursor_number, fsstate->query);

	/*
	 * Notice that we pass NULL for paramTypes, thus forcing the remote server
	 * to infer types for all parameters.  Since we explicitly cast every
	 * parameter (see deparse.c), the "inference" is trivial and will produce
	 * the desired result.  This allows us to avoid assuming that the remote
	 * server has the same OIDs we do for the parameters' types.
	 */
	if (!PQsendQueryParams(conn, buf.data, numParams,
						   NULL, values, NULL, NULL, 0))
		pgfdw_report_error(NULL, conn, buf.data);

	/*
	 * Get the result, and check for success.
	 */
	res = pgfdw_get_result(conn);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
		pgfdw_report_error(res, conn, fsstate->query);
	PQclear(res);

	/* Mark the cursor as created, and show no tuples have been retrieved */
	fsstate->cursor_exists = true;
	fsstate->tuples = NULL;
	fsstate->num_tuples = 0;
	fsstate->next_tuple = 0;
	fsstate->fetch_ct_2 = 0;
	fsstate->eof_reached = false;

	/* Clean up */
	pfree(buf.data);
}
/* …（同文件无关代码省略）… */
static void
fetch_more_data(ForeignScanState *node)
{
	PgFdwScanState *fsstate = (PgFdwScanState *) node->fdw_state;
	PGconn	   *conn = fsstate->conn;
	PGresult   *res;
	int			numrows;
	int			i;
	MemoryContext oldcontext;

	/*
	 * We'll store the tuples in the batch_cxt.  First, flush the previous
	 * batch.
	 */
	fsstate->tuples = NULL;
	MemoryContextReset(fsstate->batch_cxt);
	oldcontext = MemoryContextSwitchTo(fsstate->batch_cxt);

	if (fsstate->async_capable)
	{
		Assert(fsstate->conn_state->pendingAreq);

		/*
		 * The query was already sent by an earlier call to
		 * fetch_more_data_begin.  So now we just fetch the result.
		 */
		res = pgfdw_get_result(conn);
		/* On error, report the original query, not the FETCH. */
		if (PQresultStatus(res) != PGRES_TUPLES_OK)
			pgfdw_report_error(res, conn, fsstate->query);

		/* Reset per-connection state */
		fsstate->conn_state->pendingAreq = NULL;
	}
	else
	{
		char		sql[64];

		/* This is a regular synchronous fetch. */
		snprintf(sql, sizeof(sql), "FETCH %d FROM c%u",
				 fsstate->fetch_size, fsstate->cursor_number);

		res = pgfdw_exec_query(conn, sql, fsstate->conn_state);
		/* On error, report the original query, not the FETCH. */
		if (PQresultStatus(res) != PGRES_TUPLES_OK)
			pgfdw_report_error(res, conn, fsstate->query);
	}

	/* Convert the data into HeapTuples */
	numrows = PQntuples(res);
	fsstate->tuples = palloc0_array(HeapTuple, numrows);
	fsstate->num_tuples = numrows;
	fsstate->next_tuple = 0;

	for (i = 0; i < numrows; i++)
	{
		Assert(IsA(node->ss.ps.plan, ForeignScan));

		fsstate->tuples[i] =
			make_tuple_from_result_row(res, i,
									   fsstate->rel,
									   fsstate->attinmeta,
									   fsstate->retrieved_attrs,
									   node,
									   fsstate->temp_cxt);
	}

	/* Update fetch_ct_2 */
	if (fsstate->fetch_ct_2 < 2)
		fsstate->fetch_ct_2++;

	/* Must be EOF if we didn't get as many tuples as we asked for. */
	fsstate->eof_reached = (numrows < fsstate->fetch_size);

	PQclear(res);

	MemoryContextSwitchTo(oldcontext);
}
/* …（同文件无关代码省略）… */
int
set_transmission_modes(void)
{
	int			nestlevel = NewGUCNestLevel();

	/*
	 * The values set here should match what pg_dump does.  See also
	 * configure_remote_session in connection.c.
	 */
	if (DateStyle != USE_ISO_DATES)
		(void) set_config_option("datestyle", "ISO",
								 PGC_USERSET, PGC_S_SESSION,
								 GUC_ACTION_SAVE, true, 0, false);
	if (IntervalStyle != INTSTYLE_POSTGRES)
		(void) set_config_option("intervalstyle", "postgres",
								 PGC_USERSET, PGC_S_SESSION,
								 GUC_ACTION_SAVE, true, 0, false);
	if (extra_float_digits < 3)
		(void) set_config_option("extra_float_digits", "3",
								 PGC_USERSET, PGC_S_SESSION,
								 GUC_ACTION_SAVE, true, 0, false);

	/*
	 * In addition force restrictive search_path, in case there are any
	 * regproc or similar constants to be printed.
	 */
	(void) set_config_option("search_path", "pg_catalog",
							 PGC_USERSET, PGC_S_SESSION,
							 GUC_ACTION_SAVE, true, 0, false);

	return nestlevel;
}
/* …（同文件无关代码省略）… */
void
reset_transmission_modes(int nestlevel)
{
	AtEOXact_GUC(true, nestlevel);
}
/* …（同文件无关代码省略）… */
static void
process_query_params(ExprContext *econtext,
					 FmgrInfo *param_flinfo,
					 List *param_exprs,
					 const char **param_values)
{
	int			nestlevel;
	int			i;
	ListCell   *lc;

	nestlevel = set_transmission_modes();

	i = 0;
	foreach(lc, param_exprs)
	{
		ExprState  *expr_state = (ExprState *) lfirst(lc);
		Datum		expr_value;
		bool		isNull;

		/* Evaluate the parameter expression */
		expr_value = ExecEvalExpr(expr_state, econtext, &isNull);

		/*
		 * Get string representation of each parameter value by invoking
		 * type-specific output function, unless the value is null.
		 */
		if (isNull)
			param_values[i] = NULL;
		else
			param_values[i] = OutputFunctionCall(&param_flinfo[i], expr_value);

		i++;
	}

	reset_transmission_modes(nestlevel);
}
/* …（同文件无关代码省略）… */
	ForeignScanState *node = (ForeignScanState *) areq->requestee;
	PgFdwScanState *fsstate = (PgFdwScanState *) node->fdw_state;
	AsyncRequest *pendingAreq = fsstate->conn_state->pendingAreq;
	AppendState *requestor = (AppendState *) areq->requestor;
	WaitEventSet *set = requestor->as_eventset;

	/* This should not be called unless callback_pending */
	Assert(areq->callback_pending);

	/*
	 * If process_pending_request() has been invoked on the given request
/* …（同文件无关代码省略）… */
	/* The core code would have registered postmaster death event */
	Assert(GetNumRegisteredWaitEvents(set) >= 1);

	/* Begin an asynchronous data fetch if not already done */
	if (!pendingAreq)
		fetch_more_data_begin(areq);
	else if (pendingAreq->requestor != areq->requestor)
	{
		/*
		 * This is the case when the in-process request was made by another
		 * Append.  Note that it might be useless to process the request made
		 * by that Append, because the query might not need tuples from that
		 * Append anymore; so we avoid processing it to begin a fetch for the
		 * given request if possible.  If there are any child subplans of the
		 * same parent that are ready for new requests, skip the given
		 * request.  Likewise, if there are any configured events other than
		 * the postmaster death event, skip it.  Otherwise, process the
		 * in-process request, then begin a fetch to configure the event
		 * below, because we might otherwise end up with no configured events
		 * other than the postmaster death event.
		 */
		if (!bms_is_empty(requestor->as_needrequest))
			return;
		if (GetNumRegisteredWaitEvents(set) > 1)
			return;
		process_pending_request(pendingAreq);
		fetch_more_data_begin(areq);
	}
	else if (pendingAreq->requestee != areq->requestee)
	{
		/*
		 * This is the case when the in-process request was made by the same
		 * parent but for a different child.  Since we configure only the
		 * event for the request made for that child, skip the given request.
		 */
		return;
	}
/* …（同文件无关代码省略）… */
static void
fetch_more_data_begin(AsyncRequest *areq)
{
	ForeignScanState *node = (ForeignScanState *) areq->requestee;
	PgFdwScanState *fsstate = (PgFdwScanState *) node->fdw_state;
	char		sql[64];

	Assert(!fsstate->conn_state->pendingAreq);

	/* Create the cursor synchronously. */
	if (!fsstate->cursor_exists)
		create_cursor(node);

	/* We will send this query, but not wait for the response. */
	snprintf(sql, sizeof(sql), "FETCH %d FROM c%u",
			 fsstate->fetch_size, fsstate->cursor_number);

	if (!PQsendQuery(fsstate->conn, sql))
		pgfdw_report_error(NULL, fsstate->conn, fsstate->query);

	/* Remember that the request is in process */
	fsstate->conn_state->pendingAreq = areq;
}
/* …（同文件无关代码省略）… */
void
process_pending_request(AsyncRequest *areq)
{
	ForeignScanState *node = (ForeignScanState *) areq->requestee;
	PgFdwScanState *fsstate = (PgFdwScanState *) node->fdw_state;

	/* The request would have been pending for a callback */
	Assert(areq->callback_pending);

	/* The request should be currently in-process */
	Assert(fsstate->conn_state->pendingAreq == areq);

	fetch_more_data(node);

	/*
	 * If we didn't get any tuples, must be end of data; complete the request
	 * now.  Otherwise, we postpone completing the request until we are called
	 * from postgresForeignAsyncConfigureWait()/postgresForeignAsyncNotify().
	 */
	if (fsstate->next_tuple >= fsstate->num_tuples)
	{
		/* Unlike AsyncNotify, we unset callback_pending ourselves */
		areq->callback_pending = false;
		/* Mark the request as complete */
		ExecAsyncRequestDone(areq, NULL);
		/* Unlike AsyncNotify, we call ExecAsyncResponse ourselves */
		ExecAsyncResponse(areq);
	}
}
/* …（同文件无关代码省略）… */
static HeapTuple
make_tuple_from_result_row(PGresult *res,
						   int row,
						   Relation rel,
						   AttInMetadata *attinmeta,
						   List *retrieved_attrs,
						   ForeignScanState *fsstate,
						   MemoryContext temp_context)
{
	HeapTuple	tuple;
	TupleDesc	tupdesc;
	Datum	   *values;
	bool	   *nulls;
	ItemPointer ctid = NULL;
	ConversionLocation errpos;
	ErrorContextCallback errcallback;
	MemoryContext oldcontext;
	ListCell   *lc;
	int			j;

	Assert(row < PQntuples(res));

	/*
	 * Do the following work in a temp context that we reset after each tuple.
	 * This cleans up not only the data we have direct access to, but any
	 * cruft the I/O functions might leak.
	 */
	oldcontext = MemoryContextSwitchTo(temp_context);

	/*
	 * Get the tuple descriptor for the row.  Use the rel's tupdesc if rel is
	 * provided, otherwise look to the scan node's ScanTupleSlot.
	 */
	if (rel)
		tupdesc = RelationGetDescr(rel);
	else
	{
		Assert(fsstate);
		tupdesc = fsstate->ss.ss_ScanTupleSlot->tts_tupleDescriptor;
	}

	values = palloc0_array(Datum, tupdesc->natts);
	nulls = palloc_array(bool, tupdesc->natts);
	/* Initialize to nulls for any columns not present in result */
	memset(nulls, true, tupdesc->natts * sizeof(bool));

	/*
	 * Set up and install callback to report where conversion error occurs.
	 */
	errpos.cur_attno = 0;
	errpos.rel = rel;
	errpos.fsstate = fsstate;
	errcallback.callback = conversion_error_callback;
	errcallback.arg = &errpos;
	errcallback.previous = error_context_stack;
	error_context_stack = &errcallback;

	/*
	 * i indexes columns in the relation, j indexes columns in the PGresult.
	 */
	j = 0;
	foreach(lc, retrieved_attrs)
	{
		int			i = lfirst_int(lc);
		const char *valstr;

		/* fetch next column's textual value */
		if (PQgetisnull(res, row, j))
			valstr = NULL;
		else
			valstr = PQgetvalue(res, row, j);

		/*
		 * convert value to internal representation
		 *
		 * Note: we ignore system columns other than ctid and oid in result
		 */
		errpos.cur_attno = i;
		if (i > 0)
		{
			/* ordinary column */
			Assert(i <= tupdesc->natts);
			nulls[i - 1] = (valstr == NULL);
			/* Apply the input function even to nulls, to support domains */
			values[i - 1] = InputFunctionCall(&attinmeta->attinfuncs[i - 1],
											  valstr,
											  attinmeta->attioparams[i - 1],
											  attinmeta->atttypmods[i - 1]);
		}
		else if (i == SelfItemPointerAttributeNumber)
		{
			/* ctid */
			if (valstr != NULL)
			{
				Datum		datum;

				datum = DirectFunctionCall1(tidin, CStringGetDatum(valstr));
				ctid = (ItemPointer) DatumGetPointer(datum);
			}
		}
		errpos.cur_attno = 0;

		j++;
	}

	/* Uninstall error context callback. */
	error_context_stack = errcallback.previous;

	/*
	 * Check we got the expected number of columns.  Note: j == 0 and
	 * PQnfields == 1 is expected, since deparse emits a NULL if no columns.
	 */
	if (j > 0 && j != PQnfields(res))
		elog(ERROR, "remote query result does not match the foreign table");

	/*
	 * Build the result tuple in caller's memory context.
	 */
	MemoryContextSwitchTo(oldcontext);

	tuple = heap_form_tuple(tupdesc, values, nulls);

	/*
	 * If we have a CTID to return, install it in both t_self and t_ctid.
	 * t_self is the normal place, but if the tuple is converted to a
	 * composite Datum, t_self will be lost; setting t_ctid allows CTID to be
	 * preserved during EvalPlanQual re-evaluations (see ROW_MARK_COPY code).
	 */
	if (ctid)
		tuple->t_self = tuple->t_data->t_ctid = *ctid;

	/*
	 * Stomp on the xmin, xmax, and cmin fields from the tuple created by
	 * heap_form_tuple.  heap_form_tuple actually creates the tuple with
	 * DatumTupleFields, not HeapTupleFields, but the executor expects
	 * HeapTupleFields and will happily extract system columns on that
	 * assumption.  If we don't do this then, for example, the tuple length
	 * ends up in the xmin field, which isn't what we want.
	 */
	HeapTupleHeaderSetXmax(tuple->t_data, InvalidTransactionId);
	HeapTupleHeaderSetXmin(tuple->t_data, InvalidTransactionId);
	HeapTupleHeaderSetCmin(tuple->t_data, InvalidTransactionId);

	/* Clean up */
	MemoryContextReset(temp_context);

	return tuple;
}
/* …（同文件无关代码省略）… */
static void
conversion_error_callback(void *arg)
{
	ConversionLocation *errpos = (ConversionLocation *) arg;
	Relation	rel = errpos->rel;
	ForeignScanState *fsstate = errpos->fsstate;
	const char *attname = NULL;
	const char *relname = NULL;
	bool		is_wholerow = false;

	/*
	 * If we're in a scan node, always use aliases from the rangetable, for
	 * consistency between the simple-relation and remote-join cases.  Look at
	 * the relation's tupdesc only if we're not in a scan node.
	 */
	if (fsstate)
	{
		/* ForeignScan case */
		ForeignScan *fsplan = castNode(ForeignScan, fsstate->ss.ps.plan);
		int			varno = 0;
		AttrNumber	colno = 0;

		if (fsplan->scan.scanrelid > 0)
		{
			/* error occurred in a scan against a foreign table */
			varno = fsplan->scan.scanrelid;
			colno = errpos->cur_attno;
		}
		else
		{
			/* error occurred in a scan against a foreign join */
			TargetEntry *tle;

			tle = list_nth_node(TargetEntry, fsplan->fdw_scan_tlist,
								errpos->cur_attno - 1);

			/*
			 * Target list can have Vars and expressions.  For Vars, we can
			 * get some information, however for expressions we can't.  Thus
			 * for expressions, just show generic context message.
			 */
			if (IsA(tle->expr, Var))
			{
				Var		   *var = (Var *) tle->expr;

				varno = var->varno;
				colno = var->varattno;
			}
		}

		if (varno > 0)
		{
			EState	   *estate = fsstate->ss.ps.state;
			RangeTblEntry *rte = exec_rt_fetch(varno, estate);

			relname = rte->eref->aliasname;

			if (colno == 0)
				is_wholerow = true;
			else if (colno > 0 && colno <= list_length(rte->eref->colnames))
				attname = strVal(list_nth(rte->eref->colnames, colno - 1));
			else if (colno == SelfItemPointerAttributeNumber)
				attname = "ctid";
		}
	}
	else if (rel)
	{
		/* Non-ForeignScan case (we should always have a rel here) */
		TupleDesc	tupdesc = RelationGetDescr(rel);

		relname = RelationGetRelationName(rel);
		if (errpos->cur_attno > 0 && errpos->cur_attno <= tupdesc->natts)
		{
			Form_pg_attribute attr = TupleDescAttr(tupdesc,
												   errpos->cur_attno - 1);

			attname = NameStr(attr->attname);
		}
		else if (errpos->cur_attno == SelfItemPointerAttributeNumber)
			attname = "ctid";
	}

	if (relname && is_wholerow)
		errcontext("whole-row reference to foreign table \"%s\"", relname);
	else if (relname && attname)
		errcontext("column \"%s\" of foreign table \"%s\"", attname, relname);
	else
		errcontext("processing expression at position %d in select list",
				   errpos->cur_attno);
}
