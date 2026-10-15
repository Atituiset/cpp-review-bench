// AUTO-DRAFT from postgres/postgres PR #91b1872ff2de16cd353598edbc650a185e649226
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

	foreach(lc, root->join_info_list)
	{
		SpecialJoinInfo *sjinfo = (SpecialJoinInfo *) lfirst(lc);
		int			innerrelid;
		int			nremoved;
		RangeTblEntry *rte;

		/* Skip if not removable */
		if (!join_is_removable(root, sjinfo))
			continue;

		/*
		 * join_is_removable insists that the join's syntactic righthand side
		 * be a single baserel, so we can implement the removal by dropping
		 * the JoinExpr and everything below its righthand side.
		 */
		innerrelid = bms_singleton_member(sjinfo->syn_righthand);

		/* We verify that exactly one JoinExpr gets removed */
		nremoved = 0;
		root->parse->jointree = (FromExpr *)
			remove_join_from_jointree((Node *) root->parse->jointree,
									  sjinfo->ojrelid, &nremoved);
		if (nremoved != 1)
			elog(ERROR, "failed to find join %d in jointree", sjinfo->ojrelid);

		/* Track all the relids we've removed, for use below */
		removed_relids = bms_add_member(removed_relids, innerrelid);
		removed_relids = bms_add_member(removed_relids, sjinfo->ojrelid);

		/*
		 * As in pull_up_simple_subquery, discard no-longer-needed subqueries.
		 * This is not just an optimization, but is necessary to prevent
		 * subsequent processing from descending into stale subtrees and
		 * seeing inconsistent data.  Likewise discard any securityQuals of
		 * the removed rel.  (Although simple_rte_array[] will be rebuilt
		 * shortly, we can still use it to find the RTE in the parse tree.)
		 */
		rte = root->simple_rte_array[innerrelid];
		if (rte->rtekind == RTE_SUBQUERY)
			rte->subquery = NULL;
		rte->securityQuals = NIL;

		/*
		 * It's okay to keep scanning join_info_list for more removable joins,
		 * even though the data that join_is_removable consults is now
		 * slightly out of date.  Removing a join can only delete attr_needed
		 * bits and join clauses, and any attr_needed bit or join clause that
		 * mentions the removed rel above its own join level would have
		 * prevented that rel from being removable.  So what remains to be
		 * examined is unchanged by what we just did.
		 *
		 * The converse doesn't hold: dropping a join can make some other join
		 * removable that didn't look so before.  That's why our caller loops
		 * until we report finding nothing more to remove.
		 */
	}
/* …（同文件无关代码省略）… */
static bool
join_is_removable(PlannerInfo *root, SpecialJoinInfo *sjinfo)
{
	int			innerrelid;
	RelOptInfo *innerrel;
	Relids		inputrelids;
	Relids		joinrelids;
	List	   *clause_list = NIL;
	ListCell   *l;
	int			attroff;

	/*
	 * Must be a left join to a single baserel, else we aren't going to be
	 * able to do anything with it.
	 */
	if (sjinfo->jointype != JOIN_LEFT)
		return false;

	/*
	 * We test the syntactic righthand side, not min_righthand, because the
	 * removal is done by deleting the whole righthand subtree of the join.
	 * (min_righthand can be a singleton when syn_righthand is not, but in
	 * such a case the attr_needed tests below would reject the join anyway.)
	 */
	if (!bms_get_singleton_member(sjinfo->syn_righthand, &innerrelid))
		return false;
	Assert(bms_equal(sjinfo->min_righthand, sjinfo->syn_righthand));

	/*
	 * Never try to eliminate a left join to the query result rel.  Although
	 * the case is syntactically impossible in standard SQL, MERGE will build
	 * a join tree that looks exactly like that.
	 */
	if (innerrelid == root->parse->resultRelation)
		return false;

	innerrel = find_base_rel(root, innerrelid);

	/*
	 * Before we go to the effort of checking whether any innerrel variables
	 * are needed above the join, make a quick check to eliminate cases in
	 * which we will surely be unable to prove uniqueness of the innerrel.
	 */
	if (!rel_supports_distinctness(root, innerrel))
		return false;

	/* Compute the relid set for the join we are considering */
	inputrelids = bms_union(sjinfo->min_lefthand, sjinfo->min_righthand);
	Assert(sjinfo->ojrelid != 0);
	joinrelids = bms_copy(inputrelids);
	joinrelids = bms_add_member(joinrelids, sjinfo->ojrelid);

	/*
	 * We can't remove the join if any inner-rel attributes are used above the
	 * join.  Here, "above" the join includes pushed-down conditions, so we
	 * should reject if attr_needed includes the OJ's own relid; therefore,
	 * compare to inputrelids not joinrelids.
	 *
	 * As a micro-optimization, it seems better to start with max_attr and
	 * count down rather than starting with min_attr and counting up, on the
	 * theory that the system attributes are somewhat less likely to be wanted
	 * and should be tested last.
	 */
	for (attroff = innerrel->max_attr - innerrel->min_attr;
		 attroff >= 0;
		 attroff--)
	{
		if (!bms_is_subset(innerrel->attr_needed[attroff], inputrelids))
			return false;
	}

	/*
	 * Similarly check that the inner rel isn't needed by any PlaceHolderVars
	 * that will be used above the join.  The PHV case is a little bit more
	 * complicated, because PHVs may have been assigned a ph_eval_at location
	 * that includes the innerrel, yet their contained expression might not
	 * actually reference the innerrel (it could be just a constant, for
	 * instance).  If such a PHV is due to be evaluated above the join then it
	 * needn't prevent join removal.
	 */
	foreach(l, root->placeholder_list)
	{
		PlaceHolderInfo *phinfo = (PlaceHolderInfo *) lfirst(l);

		if (bms_overlap(phinfo->ph_lateral, innerrel->relids))
			return false;		/* it references innerrel laterally */
		if (!bms_overlap(phinfo->ph_eval_at, innerrel->relids))
			continue;			/* it definitely doesn't reference innerrel */
		if (bms_is_subset(phinfo->ph_needed, inputrelids))
			continue;			/* PHV is not used above the join */
		if (!bms_is_member(sjinfo->ojrelid, phinfo->ph_eval_at))
			return false;		/* it has to be evaluated below the join */

		/*
		 * We need to be sure there will still be a place to evaluate the PHV
		 * if we remove the join, ie that ph_eval_at wouldn't become empty.
		 */
		if (!bms_ov
