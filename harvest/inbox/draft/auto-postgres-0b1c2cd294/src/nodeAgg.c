// AUTO-DRAFT from postgres/postgres PR #93ec486803b703fbc7fe7ea332da92e30cefc381
 * read as null in the context of the current grouping set.  Also stash the
 * current group bitmap where GroupingExpr can get at it.
 *
 * This relies on three conditions:
 *
 * 1) Nothing is ever going to try and extract the whole tuple from this slot,
 * only reference it in evaluations, which will only access individual
/* …（同文件无关代码省略）… */
 * 3) Within a given phase, we never need to recover the value of an attribute
 * once it has been set to null.
 *
 * Poking into the slot this way is a bit ugly, but the consensus is that the
 * alternative was worse.
 */
/* …（同文件无关代码省略）… */
		{
			ListCell   *lc;
  // <<< BUG ANCHOR
			/* all_grouped_cols is arranged in desc order */
			slot_getsomeattrs(slot, linitial_int(aggstate->all_grouped_cols));

			foreach(lc, aggstate->all_grouped_cols)
			{
/* …（同文件无关代码省略）… */
		/* Initialize this to 1, meaning nothing spilled, yet */
		aggstate->hash_batches_used = 1;
	}

	/*
	 * Initialize current phase-dependent values to initial phase. The initial
