// AUTO-DRAFT from postgres/postgres PR #a464d31ff6213fe47505c750972b404e4517ebd9
										ec->ec_min_security);
  // <<< BUG ANCHOR
	/*
	 * If either EM is a child, force the clause's clause_relids to include
	 * the relid(s) of the child rel.  In normal cases it would already, but
	 * not if we are considering appendrel child relations with pseudoconstant
	 * translated variables (i.e., UNION ALL sub-selects with constant output
	 * items).  We must do this so that join_clause_is_movable_into() will
	 * think that the clause should be evaluated at the correct place.
	 */
	if (leftem->em_is_child)
		rinfo->clause_relids = bms_add_members(rinfo->clause_relids,
											   leftem->em_relids);
	if (rightem->em_is_child)
		rinfo->clause_relids = bms_add_members(rinfo->clause_relids,
											   rightem->em_relids);

	/* If it's a child clause, copy the parent's rinfo_serial */
	if (parent_rinfo)
