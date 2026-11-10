// AUTO-DRAFT from postgres/postgres PR #58aecdd064c5f156e04e96c3fa4074f4ae3e19ca
#define EVENT_BUFFER_SIZE			16
/* …（同文件无关代码省略）… */
 *		ExecAppendAsyncEventWait
 *
 *		Wait or poll for file descriptor events and fire callbacks.
 * ----------------------------------------------------------------
 */
static void
/* …（同文件无关代码省略）… */
	if (nevents > EVENT_BUFFER_SIZE)
		nevents = EVENT_BUFFER_SIZE;

	/*
	 * If the timeout is -1, wait until at least one event occurs.  If the
	 * timeout is 0, poll for events, but do not wait at all.
	 */
	noccurred = WaitEventSetWait(node->as_eventset, timeout, occurred_event,
								 nevents, WAIT_EVENT_APPEND_READY);
	FreeWaitEventSet(node->as_eventset);
