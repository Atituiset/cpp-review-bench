// AUTO-DRAFT from postgres/postgres PR #abf5fec634f6aac98e80e2ef6c3f3dbf0133dfaa
				new_rel_allfrozen;
	PGRUsage	ru0;
	TimestampTz starttime = 0;
	PgStat_Counter startreadtime = 0,  // <<< BUG ANCHOR
				startwritetime = 0;
	WalUsage	startwalusage = pgWalUsage;
	BufferUsage startbufferusage = pgBufferUsage;
	ErrorContextCallback errcallback;
/* …（同文件无关代码省略）… */
	instrument = (verbose || (AmAutoVacuumWorkerProcess() &&
							  params->log_vacuum_min_duration >= 0));
	if (instrument)
	{
		pg_rusage_init(&ru0);
		if (track_io_timing)
		{
			startreadtime = pgStatBlockReadTime;
			startwritetime = pgStatBlockWriteTime;
		}
	}

	/* Used for instrumentation and stats report */
	starttime = GetCurrentTimestamp();
/* …（同文件无关代码省略）… */
			}
			if (track_io_timing)
			{
				double		read_ms = (double) (pgStatBlockReadTime - startreadtime) / 1000;
				double		write_ms = (double) (pgStatBlockWriteTime - startwritetime) / 1000;

				appendStringInfo(&buf, _("I/O timings: read: %.3f ms, write: %.3f ms\n"),
								 read_ms, write_ms);
