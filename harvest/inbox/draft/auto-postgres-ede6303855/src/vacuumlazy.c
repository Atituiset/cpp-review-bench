// AUTO-DRAFT from postgres/postgres PR #206e71935e1ed86e6116b118d7b77e78efcff8b8
  // <<< BUG ANCHOR
	BlockNumber rel_pages;		/* total number of pages */
	BlockNumber scanned_pages;	/* # pages examined (not skipped via VM) */

	/*
	 * Count of all-visible blocks eagerly scanned (for logging only). This
/* …（同文件无关代码省略）… */

	/* Initialize page counters explicitly (be tidy) */
	vacrel->scanned_pages = 0;
	vacrel->eager_scanned_pages = 0;
	vacrel->removed_pages = 0;
	vacrel->new_frozen_tuple_pages = 0;
/* …（同文件无关代码省略）… */
	/*
	 * Now actually update rel's pg_class entry.
	 *
	 * new_live_tuples can be -1 when VACUUM skipped nearly all pages and had
	 * no prior tuple-density estimate.
	 */
	vac_update_relstats(rel, new_rel_pages, vacrel->new_live_tuples,
						new_rel_allvisible, new_rel_allfrozen,
/* …（同文件无关代码省略）… */
								 rel_pages);

	/* now we can compute the new value for pg_class.reltuples */
	vacrel->new_live_tuples = vac_estimate_reltuples(vacrel->rel, rel_pages,
													 vacrel->scanned_pages,
													 vacrel->live_tuples);

	/*
	 * Also compute the total number of surviving heap entries.  In the
/* …（同文件无关代码省略）… */

	if (PageIsNew(page))
	{
		/*
		 * All-zeroes pages can be left over if either a backend extends the
		 * relation by a single page, but crashes before the newly initialized
