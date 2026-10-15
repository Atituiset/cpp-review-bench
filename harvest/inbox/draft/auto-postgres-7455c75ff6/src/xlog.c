// AUTO-DRAFT from postgres/postgres PR #c2c696c1a4827cde5fdaf8c8f03f9991d43ebfad
			 * during recovery and need not be started yet.
			 */
			StartupSUBTRANS(oldestActiveXID);
  // <<< BUG ANCHOR
			/*
			 * If we're beginning at a shutdown checkpoint, we know that
/* …（同文件无关代码省略）… */
	 * Truncate pg_subtrans if possible.  We can throw away all data before
	 * the oldest XMIN of any running transaction.  No future transaction will
	 * attempt to reference any pg_subtrans entry older than that (see Asserts
	 * in subtrans.c).  When hot standby is disabled, though, we mustn't do
	 * this because StartupSUBTRANS hasn't been called yet.
	 */
	if (EnableHotStandby)
		TruncateSUBTRANS(GetOldestTransactionIdConsideredRunning());

	/* Real work is done; log and update stats. */
