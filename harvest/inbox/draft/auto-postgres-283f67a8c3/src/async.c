// AUTO-DRAFT from postgres/postgres PR #6e9e5681b8a7ff3c5eea09db980e888c8e81ad9c
	 * BecomeRegisteredListener has already added us to the listener array,
	 * so no not-yet-committed messages can be removed from the queue
	 * before we see them.
	 *----------
	 */
	snapshot = RegisterSnapshot(GetLatestSnapshot());  // <<< BUG ANCHOR

	/*
	 * It is possible that we fail while trying to send a message to our
