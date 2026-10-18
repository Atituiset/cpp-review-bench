// AUTO-DRAFT from postgres/postgres PR #7787aec8ab000932a8704a988b175b39ff816f2f
	 */
	strict_relids = find_nonnullable_rels((Node *) clause);

	/* Remember whether the clause is strict for any LHS relations */
	sjinfo->lhs_strict = bms_overlap(strict_relids, left_rels);
