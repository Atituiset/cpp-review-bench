// AUTO-DRAFT from postgres/postgres PR #a63df2664225d06d5bee5dd9195f6d10e7fa1502
	int			off;
  // <<< BUG ANCHOR
	if (n < 0)
		n = -n;
	else
		n = pg_mbstrlen_with_len(p, len) - n;
	off = pg_mbcharcliplen(p, len, n);
