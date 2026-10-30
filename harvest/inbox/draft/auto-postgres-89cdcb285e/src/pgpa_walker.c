// AUTO-DRAFT from postgres/postgres PR #a054b77e40d0579a9ee1218e2447d5af045d1083
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include "utils/lsyscache.h"

static void pgpa_walk_recursively(pgpa_plan_walker_context *walker, Plan *plan,
								  bool within_join_problem,
								  pgpa_join_unroller *join_unroller,
								  List *active_query_features,
/* …（同文件无关代码省略）… */
	walker->pstmt = pstmt;

	/* Walk the main plan tree. */
	pgpa_walk_recursively(walker, pstmt->planTree, false, NULL, NIL, false);

	/* Main plan tree walk won't reach subplans, so walk those. */
	foreach(lc, pstmt->subplans)
	{
		Plan	   *plan = lfirst(lc);

		if (plan != NULL)
			pgpa_walk_recursively(walker, plan, false, NULL, NIL, false);
	}

	/* Adjust RTIs from sj_unique_rels for the flattened range table. */
/* …（同文件无关代码省略）… */
	 * that we won't advise either GATHER or GATHER_MERGE or NO_GATHER in such
	 * cases, which might be something we want to improve in the future.
	 *
	 * (Should the Partial Aggregates in such a case be created in an
	 * UPPERREL_GROUP_AGG with a non-empty relid set? Right now that doesn't
	 * happen, but it seems like it would make life easier for us if it did.)
/* …（同文件无关代码省略）… */
/*
 * Main workhorse for the plan tree walk.
 *
 * If within_join_problem is true, we encountered a join at some higher level
 * of the tree walk and haven't yet descended out of the portion of the plan
 * tree that is part of that same join problem. We're no longer in the same
/* …（同文件无关代码省略）… */
 */
static void
pgpa_walk_recursively(pgpa_plan_walker_context *walker, Plan *plan,
					  bool within_join_problem,
					  pgpa_join_unroller *join_unroller,
					  List *active_query_features,
					  bool beneath_any_gather)
{
	pgpa_join_unroller *outer_join_unroller = NULL;
	pgpa_join_unroller *inner_join_unroller = NULL;
	bool		join_unroller_toplevel = false;
	ListCell   *lc;
	List	   *extraplans = NIL;
	List	   *elided_nodes = NIL;
	int			first_partial_plan = 0;

	Assert(within_join_problem || join_unroller == NULL);

	/*
	 * Check the future_query_features list to see whether this was previously
	 * identified as a plan node that needs to be treated as a query feature.
	 * We must do this before handling elided nodes, because if there's an
	 * elided node associated with a future query feature, the RTIs associated
	 * with the elided node should be the only ones attributed to the query
	 * feature.
	 */
	foreach_ptr(pgpa_query_feature, qf, walker->future_query_features)
	{
		if (qf->plan == plan)
		{
			active_query_features = list_copy(active_query_features);
			active_query_features = lappend(active_query_features, qf);
			walker->future_query_features =
				list_delete_ptr(walker->future_query_features, qf);
			break;
		}
	}

	/*
	 * Find all elided nodes for this Plan node.
	 */
	foreach_node(ElidedNode, n, walker->pstmt->elidedNodes)
	{
		if (n->plan_node_id == plan->plan_node_id)
			elided_nodes = lappend(elided_nodes, n);
	}

	/* If we found any elided_nodes, handle them. */
	if (elided_nodes != NIL)
	{
		int			num_elided_nodes = list_length(elided_nodes);
		ElidedNode *last_elided_node;

		/*
		 * RTIs for the final -- and thus logically uppermost -- elided node
		 * should be collected for query features passed down by the caller.
		 * However, elided nodes act as barriers to query features, which
		 * means that (1) the remaining elided nodes, if any, should be
		 * ignored for purposes of query features and (2) the list of active
		 * query features should be reset to empty so that we do not add RTIs
		 * from the plan node that is logically beneath the elided node to the
		 * query features passed down from the caller.
		 */
		last_elided_node = list_nth(elided_nodes, num_elided_nodes - 1);
		pgpa_qf_add_rtis(active_query_features,
						 pgpa_filter_out_join_relids(last_elided_node->relids,
													 walker->pstmt->rtable));
		active_query_features = NIL;

		/*
		 * If we're within a join problem, the join_unroller is responsible
		 * for building the scan for the final elided node, so throw it out.
		 */
		if (within_join_problem)
			elided_nodes = list_truncate(elided_nodes, num_elided_nodes - 1);

		/* Build scans for all (or the remaining) elided nodes. */
		foreach_node(ElidedNode, elided_node, elided_nodes)
		{
			(void) pgpa_build_scan(walker, plan, elided_node,
								   beneath_any_gather, within_join_problem);
		}

		/*
		 * If there were any elided nodes, then everything beneath those nodes
		 * is not part of the same join problem.
		 *
		 * In more detail, if an Append or MergeAppend was elided, then a
		 * partitionwise join was chosen and only a single child survived; if
		 * a SubqueryScan was elided, the subquery was planned without
		 * flattening it into the parent.
		 */
		within_join_problem = false;
		join_unroller = NULL;
	}

	/*
	 * If this is a Gather or Gather Merge node, directly add it to the list
	 * of currently-active query features. We must do this after handling
	 * elided nodes, since the Gather or Gather Merge node occurs logically
	 * beneath any associated elided nodes.
	 *
	 * Exception: We disregard any single_copy Gather nodes. These are created
	 * by debug_parallel_query, and having them affect the plan advice is
	 * counterproductive, as the result will be to advise the use of a real
	 * Gather node, rather than a single copy one.
	 */
	if (IsA(plan, Gather) && !((Gather *) plan)->single_copy)
	{
		active_query_features =
			lappend(list_copy(active_query_features),
					pgpa_add_feature(walker, PGPAQF_GATHER, plan));
		beneath_any_gather = true;
	}
	else if (IsA(plan, GatherMerge))
	{
		active_query_features =
			lappend(list_copy(active_query_features),
					pgpa_add_feature(walker, PGPAQF_GATHER_MERGE, plan));
		beneath_any_gather = true;
	}

	/*
	 * If we're within a join problem, the join unroller is responsible for
	 * building any required scan for this node. If not, we do it here.
	 */
	if (!within_join_problem)
		(void) pgpa_build_scan(walker, plan, NULL, beneath_any_gather
