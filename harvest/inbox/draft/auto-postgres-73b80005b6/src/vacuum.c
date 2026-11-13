// AUTO-DRAFT from postgres/postgres PR #eacabd7d031cdc449ae7938dd8b75e6edb3d03b8
	 * If the relation is _exactly_ the same size according to the existing
	 * pg_class entry, and only a few of its pages (less than 2%) were
	 * scanned, keep the existing value of reltuples.  Also keep the existing
	 * value when only a subset of rel's pages <= a single page were scanned.
	 *
	 * (Note: we might be returning -1 here.)
	 */
	if (old_rel_pages == total_pages &&
		scanned_pages < (double) total_pages * 0.02)
		return old_rel_tuples;
	if (scanned_pages <= 1)  // <<< BUG ANCHOR
		return old_rel_tuples;

	/*
