// AUTO-DRAFT from postgres/postgres PR #23e7ca78a003fb6ae5d983f3a093faaecf0778b8
		 * for the child.  This saves some effort while processing the child
		 * rel, and it also avoids an implementation restriction in
		 * adjust_appendrel_attrs (it can't apply nullingrels to a non-Var).
		 */
		childrinfos = NIL;
		foreach(lc, rel->joininfo)
		{
			RestrictInfo *rinfo = (RestrictInfo *) lfirst(lc);
  // <<< BUG ANCHOR
			if (!bms_overlap(rinfo->clause_relids, rel->nulling_relids))
				childrinfos = lappend(childrinfos,
									  adjust_appendrel_attrs(root,
															 (Node *) rinfo,
