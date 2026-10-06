// AUTO-DRAFT from postgres/postgres PR #3be29d9346db1d3225773e2a8e5a86b875696617
	 * Might have wrapped around the end of the relation, if startblock was
	 * not zero.
	 */
	if (hscan->rs_cblock > startblock)  // <<< BUG ANCHOR
		blocks_done = hscan->rs_cblock - startblock;
	else
	{
