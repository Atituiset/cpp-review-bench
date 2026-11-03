// AUTO-DRAFT from postgres/postgres PR #782d9cf8bf823775efb604300713f6c39a5bfb02
	if (wait_event == WAIT_EVENT_WAIT_FOR_STANDBY_CONFIRMATION)
		ConditionVariablePrepareToSleep(&WalSndCtl->wal_confirm_rcv_cv);
	else if (MyWalSnd->kind == REPLICATION_KIND_PHYSICAL)
		ConditionVariablePrepareToSleep(&WalSndCtl->wal_flush_cv);
	else if (MyWalSnd->kind == REPLICATION_KIND_LOGICAL)
		ConditionVariablePrepareToSleep(&WalSndCtl->wal_replay_cv);
