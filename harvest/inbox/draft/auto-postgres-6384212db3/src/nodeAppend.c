// AUTO-DRAFT from postgres/postgres PR #11591f273e6a26fb55728b410538fa2627b19b28
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include "executor/nodeAppend.h"
#include "miscadmin.h"
#include "pgstat.h"
#include "storage/latch.h"
#include "storage/lwlock.h"
#include "utils/wait_event.h"

/* …（同文件无关代码省略）… */
};

#define INVALID_SUBPLAN_INDEX		-1
#define EVENT_BUFFER_SIZE			16

static TupleTableSlot *ExecAppend(PlanState *pstate);
static bool choose_next_subplan_locally(AppendState *node);
/* …（同文件无关代码省略）… */
static TupleTableSlot *
ExecAppend(PlanState *pstate)
{
	AppendState *node = castNode(AppendState, pstate);
	TupleTableSlot *result;

	/*
	 * If this is the first call after Init or ReScan, we need to do the
	 * initialization work.
	 */
	if (!node->as_begun)
	{
		Assert(node->as_whichplan == INVALID_SUBPLAN_INDEX);
		Assert(!node->as_syncdone);

		/* Nothing to do if there are no subplans */
		if (node->as_nplans == 0)
			return ExecClearTuple(node->ps.ps_ResultTupleSlot);

		/* If there are any async subplans, begin executing them. */
		if (node->as_nasyncplans > 0)
			ExecAppendAsyncBegin(node);

		/*
		 * If no sync subplan has been chosen, we must choose one before
		 * proceeding.
		 */
		if (!node->choose_next_subplan(node) && node->as_nasyncremain == 0)
			return ExecClearTuple(node->ps.ps_ResultTupleSlot);

		Assert(node->as_syncdone ||
			   (node->as_whichplan >= 0 &&
				node->as_whichplan < node->as_nplans));

		/* And we're initialized. */
		node->as_begun = true;
	}

	for (;;)
	{
		PlanState  *subnode;

		CHECK_FOR_INTERRUPTS();

		/*
		 * try to get a tuple from an async subplan if any
		 */
		if (node->as_syncdone || !bms_is_empty(node->as_needrequest))
		{
			if (ExecAppendAsyncGetNext(node, &result))
				return result;
			Assert(!node->as_syncdone);
			Assert(bms_is_empty(node->as_needrequest));
		}

		/*
		 * figure out which sync subplan we are currently processing
		 */
		Assert(node->as_whichplan >= 0 && node->as_whichplan < node->as_nplans);
		subnode = node->appendplans[node->as_whichplan];

		/*
		 * get a tuple from the subplan
		 */
		result = ExecProcNode(subnode);

		if (!TupIsNull(result))
		{
			/*
			 * If the subplan gave us something then return it as-is. We do
			 * NOT make use of the result slot that was set up in
			 * ExecInitAppend; there's no need for it.
			 */
			return result;
		}

		/*
		 * wait or poll for async events if any. We do this before checking
		 * for the end of iteration, because it might drain the remaining
		 * async subplans.
		 */
		if (node->as_nasyncremain > 0)
			ExecAppendAsyncEventWait(node);

		/* choose new sync subplan; if no sync/async subplans, we're done */
		if (!node->choose_next_subplan(node) && node->as_nasyncremain == 0)
			return ExecClearTuple(node->ps.ps_ResultTupleSlot);
	}
}
/* …（同文件无关代码省略）… */
static bool
choose_next_subplan_locally(AppendState *node)
{
	int			whichplan = node->as_whichplan;
	int			nextplan;

	/* We should never be called when there are no subplans */
	Assert(node->as_nplans > 0);

	/* Nothing to do if syncdone */
	if (node->as_syncdone)
		return false;

	/*
	 * If first call then have the bms member function choose the first valid
	 * sync subplan by initializing whichplan to -1.  If there happen to be no
	 * valid sync subplans then the bms member function will handle that by
	 * returning a negative number which will allow us to exit returning a
	 * false value.
	 */
	if (whichplan == INVALID_SUBPLAN_INDEX)
	{
		if (node->as_nasyncplans > 0)
		{
			/* We'd have filled as_valid_subplans already */
			Assert(node->as_valid_subplans_identified);
		}
		else if (!node->as_valid_subplans_identified)
		{
			node->as_valid_subplans =
				ExecFindMatchingSubPlans(node->as_prune_state, false, NULL);
			node->as_valid_subplans_identified = true;
		}

		whichplan = -1;
	}

	/* Ensure whichplan is within the expected range */
	Assert(whichplan >= -1 && whichplan <= node->as_nplans);

	if (ScanDirectionIsForward(node->ps.state->es_direction))
		nextplan = bms_next_member(node->as_valid_subplans, whichplan);
	else
		nextplan = bms_prev_member(node->as_valid_subplans, whichplan);

	if (nextplan < 0)
	{
		/* Set as_syncdone if in async mode */
		if (node->as_nasyncplans > 0)
			node->as_syncdone = true;
		return false;
	}

	node->as_whichplan = nextplan;

	return true;
}
/* …（同文件无关代码省略）… */
static void
ExecAppendAsyncBegin(AppendState *node)
{
	int			i;

	/* Backward scan is not supported by async-aware Appends. */
	Assert(ScanDirectionIsForward(node->ps.state->es_direction));

	/* We should never be called when there are no subplans */
	Assert(node->as_nplans > 0);

	/* We should never be called when there are no async subplans. */
	Assert(node->as_nasyncplans > 0);

	/* If we've yet to determine the valid subplans then do so now. */
	if (!node->as_valid_subplans_identified)
	{
		node->as_valid_subplans =
			ExecFindMatchingSubPlans(node->as_prune_state, false, NULL);
		node->as_valid_subplans_identified = true;

		classify_matching_subplans(node);
	}

	/* Initialize state variables. */
	node->as_syncdone = bms_is_empty(node->as_valid_subplans);
	node->as_nasyncremain = bms_num_members(node->as_valid_asyncplans);

	/* Nothing to do if there are no valid async subplans. */
	if (node->as_nasyncremain == 0)
		return;

	/* Make a request for each of the valid async subplans. */
	i = -1;
	while ((i = bms_next_member(node->as_valid_asyncplans, i)) >= 0)
	{
		AsyncRequest *areq = node->as_asyncrequests[i];

		Assert(areq->request_index == i);
		Assert(!areq->callback_pending);

		/* Do the actual work. */
		ExecAsyncRequest(areq);
	}
}
/* …（同文件无关代码省略）… */
static bool
ExecAppendAsyncGetNext(AppendState *node, TupleTableSlot **result)
{
	*result = NULL;

	/* We should never be called when there are no valid async subplans. */
	Assert(node->as_nasyncremain > 0);

	/* Request a tuple asynchronously. */
	if (ExecAppendAsyncRequest(node, result))
		return true;

	while (node->as_nasyncremain > 0)
	{
		CHECK_FOR_INTERRUPTS();

		/* Wait or poll for async events. */
		ExecAppendAsyncEventWait(node);

		/* Request a tuple asynchronously. */
		if (ExecAppendAsyncRequest(node, result))
			return true;

		/* Break from loop if there's any sync subplan that isn't complete. */
		if (!node->as_syncdone)
			break;
	}

	/*
	 * If all sync subplans are complete, we're totally done scanning the
	 * given node.  Otherwise, we're done with the asynchronous stuff but must
	 * continue scanning the sync subplans.
	 */
	if (node->as_syncdone)
	{
		Assert(node->as_nasyncremain == 0);
		*result = ExecClearTuple(node->ps.ps_ResultTupleSlot);
		return true;
	}

	return false;
}
/* …（同文件无关代码省略）… */
static bool
ExecAppendAsyncRequest(AppendState *node, TupleTableSlot **result)
{
	Bitmapset  *needrequest;
	int			i;

	/* Nothing to do if there are no async subplans needing a new request. */
	if (bms_is_empty(node->as_needrequest))
	{
		Assert(node->as_nasyncresults == 0);
		return false;
	}

	/*
	 * If there are any asynchronously-generated results that have not yet
	 * been returned, we have nothing to do; just return one of them.
	 */
	if (node->as_nasyncresults > 0)
	{
		--node->as_nasyncresults;
		*result = node->as_asyncresults[node->as_nasyncresults];
		return true;
	}

	/* Make a new request for each of the async subplans that need it. */
	needrequest = node->as_needrequest;
	node->as_needrequest = NULL;
	i = -1;
	while ((i = bms_next_member(needrequest, i)) >= 0)
	{
		AsyncRequest *areq = node->as_asyncrequests[i];

		/* Do the actual work. */
		ExecAsyncRequest(areq);
	}
	bms_free(needrequest);

	/* Return one of the asynchronously-generated results if any. */
	if (node->as_nasyncresults > 0)
	{
		--node->as_nasyncresults;
		*result = node->as_asyncresults[node->as_nasyncresults];
		return true;
	}

	return false;
}
/* …（同文件无关代码省略）… */
static void
ExecAppendAsyncEventWait(AppendState *node)
{
	int			nevents = node->as_nasyncplans + 2;
	long		timeout = node->as_syncdone ? -1 : 0;
	WaitEvent	occurred_event[EVENT_BUFFER_SIZE];
	int			noccurred;
	int			i;

	/* We should never be called when there are no valid async subplans. */
	Assert(node->as_nasyncremain > 0);

	Assert(node->as_eventset == NULL);
	node->as_eventset = CreateWaitEventSet(CurrentResourceOwner, nevents);
	AddWaitEventToSet(node->as_eventset, WL_EXIT_ON_PM_DEATH, PGINVALID_SOCKET,
					  NULL, NULL);

	/* Give each waiting subplan a chance to add an event. */
	i = -1;
	while ((i = bms_next_member(node->as_asyncplans, i)) >= 0)
	{
		AsyncRequest *areq = node->as_asyncrequests[i];

		if (areq->callback_pending)
			ExecAsyncConfigureWait(areq);
	}

	/*
	 * No need for further processing if none of the subplans configured any
	 * events.
	 */
	if (GetNumRegisteredWaitEvents(node->as_eventset) == 1)
	{
		FreeWaitEventSet(node->as_eventset);
		node->as_eventset = NULL;
		return;
	}

	/*
	 * Add the process latch to the set, so that we wake up to process the
	 * standard interrupts with CHECK_FOR_INTERRUPTS().
	 *
	 * NOTE: For historical reasons, it's important that this is added to the
	 * WaitEventSet after the ExecAsyncConfigureWait() calls.  Namely,
	 * postgres_fdw calls "GetNumRegisteredWaitEvents(set) == 1" to check if
	 * any other events are in the set.  That's a poor design, it's
	 * questionable for postgres_fdw to be doing that in the first place, but
	 * we cannot change it now.  The pattern has possibly been copied to other
	 * extensions too.
	 */
	AddWaitEventToSet(node->as_eventset, WL_LATCH_SET, PGINVALID_SOCKET,
					  MyLatch, NULL);

	/* Return at most EVENT_BUFFER_SIZE events in one call. */
	if (nevents > EVENT_BUFFER_SIZE)
		nevents = EVENT_BUFFER_SIZE;

	/* Wait or poll for file descriptor events. */
	noccurred = WaitEventSetWait(node->as_eventset, timeout, occurred_event,
								 nevents, WAIT_EVENT_APPEND_READY);
	FreeWaitEventSet(node->as_eventset);
	node->as_eventset = NULL;
	if (noccurred == 0)
		return;

	/* Deliver notifications. */
	for (i = 0; i < noccurred; i++)
	{
		WaitEvent  *w = &occurred_event[i];

		/*
		 * Each waiting subplan should have registered its wait event with
		 * user_data pointing back to its AsyncRequest.
		 */
		if ((w->events & WL_SOCKET_READABLE) != 0)
		{
			AsyncRequest *areq = (AsyncRequest *) w->user_data;

			if (areq->callback_pending)
			{
				/*
				 * Mark it as no longer needing a callback.  We must do this
				 * before dispatching the callback in case the callback resets
				 * the flag.
				 */
				areq->callback_pending = false;

				/* Do the actual work. */
				ExecAsyncNotify(areq);
			}
		}

		/* Handle standard interrupts */
		if ((w->events & WL_LATCH_SET) != 0)
		{
			ResetLatch(MyLatch);
			CHECK_FOR_INTERRUPTS();
		}
	}
}

/* ----------------------------------------------------------------
/* …（同文件无关代码省略）… */
static void
classify_matching_subplans(AppendState *node)
{
	Bitmapset  *valid_asyncplans;

	Assert(node->as_valid_subplans_identified);
	Assert(node->as_valid_asyncplans == NULL);

	/* Nothing to do if there are no valid subplans. */
	if (bms_is_empty(node->as_valid_subplans))
	{
		node->as_syncdone = true;
		node->as_nasyncremain = 0;
		return;
	}

	/* Nothing to do if there are no valid async subplans. */
	if (!bms_overlap(node->as_valid_subplans, node->as_asyncplans))
	{
		node->as_nasyncremain = 0;
		return;
	}

	/* Get valid async subplans. */
	valid_asyncplans = bms_intersect(node->as_asyncplans,
									 node->as_valid_subplans);

	/* Adjust the valid subplans to contain sync subplans only. */
	node->as_valid_subplans = bms_del_members(node->as_valid_subplans,
											  valid_asyncplans);

	/* Save valid async subplans. */
	node->as_valid_asyncplans = valid_asyncplans;
}
