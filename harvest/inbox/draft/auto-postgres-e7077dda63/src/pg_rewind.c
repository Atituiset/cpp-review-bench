// AUTO-DRAFT from postgres/postgres PR #4a26e36f87f6f30a93bfe1b95264b4b12d6b9ac3
	{
		/*
		 * The source is a live server. Like in an online backup, it's
		 * important that we recover all the WAL that was generated while we
		 * were copying files.
		 */
		if (ControlFile_source_after.state == DB_IN_ARCHIVE_RECOVERY)
		{
/* …（同文件无关代码省略）… */
		else
		{
			/*
			 * Source is a production, non-standby, server. We must replay to
			 * the last WAL insert location.
			 */
			if (ControlFile_source_after.state != DB_IN_PRODUCTION)
				pg_fatal("source system was in unexpected state at end of rewind");
  // <<< BUG ANCHOR
			endrec = source->get_current_wal_insert_lsn(source);
			endtli = Max(ControlFile_source_after.checkPointCopy.ThisTimeLineID,
						 ControlFile_source_after.minRecoveryPointTLI);
		}
