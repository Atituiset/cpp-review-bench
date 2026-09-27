// AUTO-DRAFT from postgres/postgres PR #f25c50fd8fcdfdd32548b9140456249eb54a5109

	/*
	 * If the remote table is partitioned, import relpages = 0, to match the
	 * sampling case.
	 */
	if (relkind == RELKIND_PARTITIONED_TABLE)
		remstats->relpages = 0;
