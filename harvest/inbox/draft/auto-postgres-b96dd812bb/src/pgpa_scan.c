// AUTO-DRAFT from postgres/postgres PR #a054b77e40d0579a9ee1218e2447d5af045d1083
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
 * If there is at least one ElidedNode for this plan node, pass the uppermost
 * one as elided_node, else pass NULL.
 *
 * Set the 'beneath_any_gather' node if we are underneath a Gather or
 * Gather Merge node (except for a single-copy Gather node, for which
 * GATHER or GATHER_MERGE advice should not be emitted).
 *
 * Set the 'within_join_problem' flag if we're inside of a join problem and
 * not otherwise.
 */
pgpa_scan *
pgpa_build_scan(pgpa_plan_walker_context *walker, Plan *plan,
				ElidedNode *elided_node,
				bool beneath_any_gather, bool within_join_problem)
{
	pgpa_scan_strategy strategy = PGPA_SCAN_ORDINARY;
	Bitmapset  *relids = NULL;
/* …（同文件无关代码省略）… */
		return NULL;
	}

	/*
	 * Add the appropriate set of RTIs to walker->no_gather_scans.
	 *
	 * Add nothing if we're beneath a Gather or Gather Merge node, since
	 * NO_GATHER advice is clearly inappropriate in that situation.
	 */
	if (!beneath_any_gather)
		walker->no_gather_scans =
			bms_add_members(walker->no_gather_scans, relids);

	/* Caller tells us whether NO_GATHER() advice for this scan is needed. */
	return pgpa_make_scan(walker, plan, strategy, relids);
}

/* …（同文件无关代码省略）… */
static pgpa_scan *
pgpa_make_scan(pgpa_plan_walker_context *walker, Plan *plan,
			   pgpa_scan_strategy strategy, Bitmapset *relids)
{
	pgpa_scan  *scan;

	/* Create the scan object. */
	scan = palloc_object(pgpa_scan);
	scan->plan = plan;
	scan->strategy = strategy;
	scan->relids = relids;

	/* Add it to the appropriate list. */
	walker->scans[scan->strategy] = lappend(walker->scans[scan->strategy],
											scan);

	return scan;
}

/*
 * Determine the unique rtekind of a set of relids.
 */
