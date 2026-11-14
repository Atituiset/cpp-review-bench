// AUTO-DRAFT from postgres/postgres PR #206e71935e1ed86e6116b118d7b77e78efcff8b8
 *		the old value of pg_class.reltuples/pg_class.relpages as a measurement
 *		of the tuple density in the unscanned pages.
 *
 *		Note: scanned_tuples should count only *live* tuples, since
 *		pg_class.reltuples is defined that way.
 */
double
vac_estimate_reltuples(Relation relation,
					   BlockNumber total_pages,
					   BlockNumber scanned_pages,
					   double scanned_tuples)
{
	BlockNumber old_rel_pages = relation->rd_rel->relpages;
	double		old_rel_tuples = relation->rd_rel->reltuples;
	double		old_density;
	double		unscanned_pages;
	double		total_tuples;
  // <<< BUG ANCHOR
	/* If we did scan the whole table, just use the count as-is */
	if (scanned_pages >= total_pages)
		return scanned_tuples;

	/*
	 * With no prior density, a tiny non-random sample is not sufficient to
	 * estimate the relation's tuple count. This can happen when on-access
	 * pruning marks most pages all-visible before the first VACUUM.
	 */
	if (old_rel_tuples < 0 || old_rel_pages == 0)
	{
		if (scanned_pages <= 1 ||
			scanned_pages < (double) total_pages * 0.02)
			return -1;

		return floor((scanned_tuples / scanned_pages) * total_pages + 0.5);
	}

	/*
/* …（同文件无关代码省略）… */
	 * value when at most one page was scanned.  When the relation size has
	 * changed, use the old density to account for the added or removed pages
	 * instead of preserving a stale tuple count.
	 *
	 * (Note: we might be returning -1 here.)
	 */
	if (old_rel_pages == total_pages &&
		(scanned_pages <= 1 ||
		 scanned_pages < (double) total_pages * 0.02))
		return old_rel_tuples;

	/*
/* …（同文件无关代码省略）… */
	 * and finally add on the number of tuples in the scanned pages.
	 */
	old_density = old_rel_tuples / old_rel_pages;
	unscanned_pages = (double) total_pages - (double) scanned_pages;
	total_tuples = old_density * unscanned_pages + scanned_tuples;
	return floor(total_tuples + 0.5);
}
