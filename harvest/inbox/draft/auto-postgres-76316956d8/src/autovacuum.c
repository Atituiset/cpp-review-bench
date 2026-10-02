// AUTO-DRAFT from postgres/postgres PR #42e96cf2fe095c822f76bc94669ea875cb1e3351
	}

	if (nworkers_for_balance != orig_nworkers_for_balance)
		pg_atomic_write_u32(&AutoVacuumShmem->av_nworkersForBalance,
							nworkers_for_balance);
}

/*
