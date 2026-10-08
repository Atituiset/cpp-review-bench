// AUTO-DRAFT from postgres/postgres PR #1f2245371e31f0def347546f5ad250552a4eb8a4
		 * subsequent WAL records, which may not contain logical information.
		 */
		if (status)
			EnableLogicalDecoding();  // <<< BUG ANCHOR
		else
			DisableLogicalDecoding();
