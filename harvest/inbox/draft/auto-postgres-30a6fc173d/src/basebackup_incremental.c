// AUTO-DRAFT from postgres/postgres PR #316861101e8c5ca48149d368e08ff865dd3ea306

		if (range->tli == latest_wal_range_tli)
		{
			if (range->end_lsn > backup_state->startpoint)
				ereport(ERROR,
						(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
