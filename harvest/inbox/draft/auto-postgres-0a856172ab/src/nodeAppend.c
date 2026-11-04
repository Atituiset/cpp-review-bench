// AUTO-DRAFT from postgres/postgres PR #73a28d2733ab1a9895f533785901aa2865a5864c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#define EVENT_BUFFER_SIZE			16
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

	/*
	 * If the timeout is -1, wait until at least one event occurs.  If the
	 * timeout is 0, poll for events, but do not wait at all.
	 */
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
/* …（同文件无关代码省略）… */

		CHECK_FOR_INTERRUPTS();

		/* Wait or poll for async events. */
		ExecAppendAsyncEventWait(node);
	}
