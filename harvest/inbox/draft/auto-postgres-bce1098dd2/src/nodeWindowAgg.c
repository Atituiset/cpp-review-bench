// AUTO-DRAFT from postgres/postgres PR #46ed1b831fa1e2ebb95f52c939577db560c0d3a6
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

static void
spool_tuples(WindowAggState *winstate, int64 pos)
{
	WindowAgg  *node = (WindowAgg *) winstate->ss.ps.plan;
	PlanState  *outerPlan;
	TupleTableSlot *outerslot;
	MemoryContext oldcontext;

	if (!winstate->buffer)
		return;					/* just a safety check */
	if (winstate->partition_spooled)
		return;					/* whole partition done already */

	/*
	 * When in pass-through mode we can just exhaust all tuples in the current
	 * partition.  We don't need these tuples for any further window function
	 * evaluation, however, we do need to keep them around if we're not the
	 * top-level window as another WindowAgg node above must see these.
	 */
	if (winstate->status != WINDOWAGG_RUN)
	{
		Assert(winstate->status == WINDOWAGG_PASSTHROUGH ||
			   winstate->status == WINDOWAGG_PASSTHROUGH_STRICT);

		pos = -1;
	}

	/*
	 * If the tuplestore has spilled to disk, alternate reading and writing
	 * becomes quite expensive due to frequent buffer flushes.  It's cheaper
	 * to force the entire partition to get spooled in one go.
	 *
	 * XXX this is a horrid kluge --- it'd be better to fix the performance
	 * problem inside tuplestore.  FIXME
	 */
	else if (!tuplestore_in_memory(winstate->buffer))
		pos = -1;

	outerPlan = outerPlanState(winstate);

	/* Must be in query context to call outerplan */
	oldcontext = MemoryContextSwitchTo(winstate->ss.ps.ps_ExprContext->ecxt_per_query_memory);

	while (winstate->spooled_rows <= pos || pos == -1)
	{
		outerslot = ExecProcNode(outerPlan);
		if (TupIsNull(outerslot))
		{
			/* reached the end of the last partition */
			winstate->partition_spooled = true;
			winstate->more_partitions = false;
			break;
		}

		if (node->partNumCols > 0)
		{
			ExprContext *econtext = winstate->tmpcontext;

			econtext->ecxt_innertuple = winstate->first_part_slot;
			econtext->ecxt_outertuple = outerslot;

			/* Check if this tuple still belongs to the current partition */
			if (!ExecQualAndReset(winstate->partEqfunction, econtext))
			{
				/*
				 * end of partition; copy the tuple for the next cycle.
				 */
				ExecCopySlot(winstate->first_part_slot, outerslot);
				winstate->partition_spooled = true;
				winstate->more_partitions = true;
				break;
			}
		}

		/*
		 * Remember the tuple unless we're the top-level window and we're in
		 * pass-through mode.
		 */
		if (winstate->status != WINDOWAGG_PASSTHROUGH_STRICT)
		{
			/* Still in partition, so save it into the tuplestore */
			tuplestore_puttupleslot(winstate->buffer, outerslot);
			winstate->spooled_rows++;
		}
	}

	MemoryContextSwitchTo(oldcontext);
}
/* …（同文件无关代码省略）… */
		 * expects its caller to have checked.  It cannot always detect that
		 * itself: for a frame ending at UNBOUNDED FOLLOWING with EXCLUDE
		 * GROUP or EXCLUDE TIES and no ORDER BY, it reports every row other
		 * than the current one as excluded, wherever it is, so we would
		 * never reach the end of the loop.
		 */
		spool_tuples(winstate, abs_pos);
		if (abs_pos >= winstate->spooled_rows)
