// AUTO-DRAFT from postgres/postgres PR #22ff4fc5a21bce3ccf0376e0e91fde8fd2f28655
	PROC_HDR   *ProcGlobal;
	PGPROC	   *AuxiliaryProcs;
	PGPROC	   *PreparedXactProcs;
	volatile PMSignalData *PMSignalState;  // <<< BUG ANCHOR
	ProcSignalHeader *ProcSignal;
	pid_t		PostmasterPid;
	TimestampTz PgStartTime;
