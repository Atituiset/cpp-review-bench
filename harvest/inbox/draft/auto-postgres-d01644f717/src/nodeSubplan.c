// AUTO-DRAFT from postgres/postgres PR #303c9b36205d11e41c7d5d61a1802ec50bccbf5a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
{
	bool		result = false;
	SubPlan    *subplan = node->subplan;
	PlanState  *planstate = node->planstate;
	TupleTableSlot *slot;

	/* Shouldn't have any direct correlation Vars */
/* …（同文件无关代码省略）… */
	 * If first time through or we need to rescan the subplan, build the hash
	 * table.
	 */
	if (node->hashtable == NULL || planstate->chgParam != NULL)
		buildSubPlanHash(node, econtext);

	/*
/* …（同文件无关代码省略）… */
static void
buildSubPlanHash(SubPlanState *node, ExprContext *econtext)
{
	SubPlan    *subplan = node->subplan;
	PlanState  *planstate = node->planstate;
	int			ncols = node->numCols;
	ExprContext *innerecontext = node->innerecontext;
	MemoryContext oldcontext;
	double		nentries;
	TupleTableSlot *slot;

	Assert(subplan->subLinkType == ANY_SUBLINK);

	/*
	 * If we already had any hash tables, reset 'em; otherwise create empty
	 * hash table(s).
	 *
	 * If we need to distinguish accurately between FALSE and UNKNOWN (i.e.,
	 * NULL) results of the IN operation, then we have to store subplan output
	 * rows that are partly or wholly NULL.  We store such rows in a separate
	 * hash table that we expect will be much smaller than the main table. (We
	 * can use hashing to eliminate partly-null rows that are not distinct. We
	 * keep them separate to minimize the cost of the inevitable full-table
	 * searches; see findPartialMatch.)
	 *
	 * If it's not necessary to distinguish FALSE and UNKNOWN, then we don't
	 * need to store subplan output rows that contain NULL.
	 *
	 * Because the input slot for each hash table is always the slot resulting
	 * from an ExecProject(), we can use TTSOpsVirtual for the input ops. This
	 * saves a needless fetch inner op step for the hashing ExprState created
	 * in BuildTupleHashTable().
	 */
	node->havehashrows = false;
	node->havenullrows = false;

	nentries = planstate->plan->plan_rows;

	if (node->hashtable)
		ResetTupleHashTable(node->hashtable);
	else
		node->hashtable = BuildTupleHashTable(node->parent,
											  node->descRight,
											  &TTSOpsVirtual,
											  ncols,
											  node->keyColIdx,
											  node->tab_eq_funcoids,
											  node->tab_hash_funcs,
											  node->tab_collations,
											  nentries,
											  0,	/* no additional data */
											  node->planstate->state->es_query_cxt,
											  node->tuplesContext,
											  innerecontext->ecxt_per_tuple_memory,
											  false);

	if (!subplan->unknownEqFalse)
	{
		if (ncols == 1)
			nentries = 1;		/* there can only be one entry */
		else
		{
			nentries /= 16;
			if (nentries < 1)
				nentries = 1;
		}

		if (node->hashnulls)
			ResetTupleHashTable(node->hashnulls);
		else
			node->hashnulls = BuildTupleHashTable(node->parent,
												  node->descRight,
												  &TTSOpsVirtual,
												  ncols,
												  node->keyColIdx,
												  node->tab_eq_funcoids,
												  node->tab_hash_funcs,
												  node->tab_collations,
												  nentries,
												  0,	/* no additional data */
												  node->planstate->state->es_query_cxt,
												  node->tuplesContext,
												  innerecontext->ecxt_per_tuple_memory,
												  false);
	}
	else
		node->hashnulls = NULL;

	/*
	 * We are probably in a short-lived expression-evaluation context. Switch
	 * to the per-query context for manipulating the child plan.
	 */
	oldcontext = MemoryContextSwitchTo(econtext->ecxt_per_query_memory);

	/*
	 * Reset subplan to start.
	 */
	ExecReScan(planstate);

	/*
	 * Scan the subplan and load the hash table(s).  Note that when there are
	 * duplicate rows coming out of the sub-select, only one copy is stored.
	 */
	for (slot = ExecProcNode(planstate);
		 !TupIsNull(slot);
		 slot = ExecProcNode(planstate))
	{
		int			col = 1;
		ListCell   *plst;
		bool		isnew;

		/*
		 * Load up the Params representing the raw sub-select outputs, then
		 * form the projection tuple to store in the hashtable.
		 */
		foreach(plst, subplan->paramIds)
		{
			int			paramid = lfirst_int(plst);
			ParamExecData *prmdata;

			prmdata = &(innerecontext->ecxt_param_exec_vals[paramid]);
			Assert(prmdata->execPlan == NULL);
			prmdata->value = slot_getattr(slot, col,
										  &(prmdata->isnull));
			col++;
		}
		slot = ExecProject(node->projRight);

		/*
		 * If result contains any nulls, store separately or not at all.
		 */
		if (slotNoNulls(slot))
		{
			(void) LookupTupleHashEntry(node->hashtable, slot, &isnew, NULL);
			node->havehashrows = true;
		}
		else if (node->hashnulls)
		{
			(void) LookupTupleHashEntry(node->hashnulls, slot, &isnew, NULL);
			node->havenullrows = true;
		}

		/*
		 * Reset innerecontext after each inner tuple to free any memory used
		 * during ExecProject and hashtable lookup.
		 */
		ResetExprContext(innerecontext);
	}

	/*
	 * Since the projected tuples are in the sub-query's context and not the
	 * main context, we'd better clear the tuple slot before there's any
	 * chance of a reset of the sub-query's context.  Else we will have the
	 * potential for a double free attempt.  (XXX possibly no longer needed,
	 * but can't hurt.)
	 */
	ExecClearTuple(node->projRight->pi_state.resultslot);

	MemoryContextSwitchTo(oldcontext);
}
/* …（同文件无关代码省略）… */
static bool
slotNoNulls(TupleTableSlot *slot)
{
	int			ncols = slot->tts_tupleDescriptor->natts;
	int			i;

	for (i = 1; i <= ncols; i++)
	{
		if (slot_attisnull(slot, i))
			return false;
	}
	return true;
}
/* …（同文件无关代码省略）… */
	sstate->projRight = NULL;
	sstate->hashtable = NULL;
	sstate->hashnulls = NULL;
	sstate->tuplesContext = NULL;
	sstate->innerecontext = NULL;
	sstate->keyColIdx = NULL;
