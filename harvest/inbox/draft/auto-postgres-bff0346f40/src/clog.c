// AUTO-DRAFT from postgres/postgres PR #8303f2e13c5c2fab539e11054073a58942cb0f30

	/*
	 * Update the group LSN if the transaction completion LSN is higher.
	 *
	 * Note: lsn will be invalid when supplied during InRecovery processing,
	 * so we don't need to do anything special to avoid LSN updates during
	 * recovery. After recovery completes the next clog change will set the
	 * LSN correctly.
	 */
	if (XLogRecPtrIsValid(lsn))
	{
