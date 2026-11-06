// AUTO-DRAFT from postgres/postgres PR #69b20fdd20de68b0f0eaa855669a6657c65c8533
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
static int
row_is_in_frame(WindowObject winobj, int64 pos, TupleTableSlot *slot,
				bool fetch_tuple)
{
	WindowAggState *winstate = winobj->winstate;
	int			frameOptions = winstate->frameOptions;

	Assert(pos >= 0);			/* else caller error */

	/*
	 * First, check frame starting conditions.  We might as well delegate this
	 * to update_frameheadpos always; it doesn't add any notable cost.
	 */
	update_frameheadpos(winstate);
	if (pos < winstate->frameheadpos)
		return 0;

	/*
	 * Okay so far, now check frame ending conditions.  Here, we avoid calling
	 * update_frametailpos in simple cases, so as not to spool tuples further
	 * ahead than necessary.
	 */
	if (frameOptions & FRAMEOPTION_END_CURRENT_ROW)
	{
		if (frameOptions & FRAMEOPTION_ROWS)
		{
			/* rows after current row are out of frame */
			if (pos > winstate->currentpos)
				return -1;
		}
		else if (frameOptions & (FRAMEOPTION_RANGE | FRAMEOPTION_GROUPS))
		{
			/* following row that is not peer is out of frame */
			if (pos > winstate->currentpos)
			{
				if (fetch_tuple)	/* need to fetch tuple? */
					if (!window_gettupleslot(winobj, pos, slot))
						return -1;
				if (!are_peers(winstate, slot, winstate->ss.ss_ScanTupleSlot))
					return -1;
			}
		}
		else
			Assert(false);
	}
	else if (frameOptions & FRAMEOPTION_END_OFFSET)
	{
		if (frameOptions & FRAMEOPTION_ROWS)
		{
			int64		offset = DatumGetInt64(winstate->endOffsetValue);
			int64		frameendpos = 0;

			/* rows after current row + offset are out of frame */
			if (frameOptions & FRAMEOPTION_END_OFFSET_PRECEDING)
				offset = -offset;

			/*
			 * If we have an overflow, it means the frame end is beyond the
			 * range of int64.  Since currentpos >= 0, this can only be a
			 * positive overflow.  We treat this as meaning that the frame
			 * extends to end of partition.
			 */
			if (!pg_add_s64_overflow(winstate->currentpos, offset,
									 &frameendpos) &&
				pos > frameendpos)
				return -1;
		}
		else if (frameOptions & (FRAMEOPTION_RANGE | FRAMEOPTION_GROUPS))
		{
			/* hard cases, so delegate to update_frametailpos */
			update_frametailpos(winstate);
			if (pos >= winstate->frametailpos)
				return -1;
		}
		else
			Assert(false);
	}

	/* Check exclusion clause */
	if (frameOptions & FRAMEOPTION_EXCLUDE_CURRENT_ROW)
	{
		if (pos == winstate->currentpos)
			return 0;
	}
	else if ((frameOptions & FRAMEOPTION_EXCLUDE_GROUP) ||
			 ((frameOptions & FRAMEOPTION_EXCLUDE_TIES) &&
			  pos != winstate->currentpos))
	{
		WindowAgg  *node = (WindowAgg *) winstate->ss.ps.plan;

		/* If no ORDER BY, all rows are peers with each other */
		if (node->ordNumCols == 0)
			return 0;
		/* Otherwise, check the group boundaries */
		if (pos >= winstate->groupheadpos)
		{
			update_grouptailpos(winstate);
			if (pos < winstate->grouptailpos)
				return 0;
		}
	}

	/* If we get here, it's in frame */
	return 1;
}
/* …（同文件无关代码省略）… */
static void
update_frameheadpos(WindowAggState *winstate)
{
	WindowAgg  *node = (WindowAgg *) winstate->ss.ps.plan;
	int			frameOptions = winstate->frameOptions;
	MemoryContext oldcontext;

	if (winstate->framehead_valid)
		return;					/* already known for current row */

	/* We may be called in a short-lived context */
	oldcontext = MemoryContextSwitchTo(winstate->ss.ps.ps_ExprContext->ecxt_per_query_memory);

	if (frameOptions & FRAMEOPTION_START_UNBOUND
