// AUTO-DRAFT from postgres/postgres PR #227de22095c8101905ce41e1ebf425239feed079
	 * clamp the given nelements to fit in that.  We'd also better do
	 * something sane when faced with a zero or NaN estimate.  There are
	 * various reasons why the planner might not be able to provide a good
	 * estimate here; for example, recursive CTEs don't base their estimate
	 * on anything reliable and can yield some rather large estimates.  Here
	 * we cap nbuckets to hash_mem to help prevent insane memory allocations.
	 * The hash table can still grow when the bucket count is insufficient.
	 * That will make the query a little slower, but that's better than
	 * outright failing due to OOM if we were to request an insane bucket
	 * count.  Queries being slightly slower with smaller work_mem values is
	 * also aligned to user expectations.  We purposefully don't include the
	 * tuple width in this calculation, as that would make the cap depend on
	 * planner estimates for the tuple width, which could also be unreliable.
	 */
	if (isnan(nelements) || nelements <= 0)
		nbuckets = 1;
