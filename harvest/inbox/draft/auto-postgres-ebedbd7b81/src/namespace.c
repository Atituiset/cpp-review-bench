// AUTO-DRAFT from postgres/postgres PR #9872a0de75b1d4b178681008543a0e2dffd5fda4
	int			i;
  // <<< BUG ANCHOR
	/* check for caller error */
	Assert(nargs >= 0 || !(expand_variadic | expand_defaults));

	/* initialize output fgc_flags to empty */
	*fgc_flags = 0;
