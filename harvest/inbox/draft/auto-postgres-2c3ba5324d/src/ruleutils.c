// AUTO-DRAFT from postgres/postgres PR #1999fe227df70249dce7c9037147b54c1090b4ad
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
typedef struct
{
	List	   *rtable;			/* List of RangeTblEntry nodes */
	List	   *rtable_names;	/* Parallel list of names for RTEs */
	List	   *rtable_columns; /* Parallel list of deparse_columns structs */
	List	   *subplans;		/* List of Plan trees for SubPlans */
	List	   *ctes;			/* List of CommonTableExpr nodes */
	AppendRelInfo **appendrels; /* Array of AppendRelInfo nodes, or NULL */
	char	   *ret_old_alias;	/* alias for OLD in RETURNING list */
	char	   *ret_new_alias;	/* alias for NEW in RETURNING list */
	/* Workspace for column alias assignment: */
	bool		unique_using;	/* Are we making USING names globally unique */
	List	   *using_names;	/* List of assigned names for USING columns */
	/* Remaining fields are used only when deparsing a Plan tree: */
	Plan	   *plan;			/* immediate parent of current expression */
	List	   *ancestors;		/* ancestors of plan */
	Plan	   *outer_plan;		/* outer subnode, or NULL if none */
	Plan	   *inner_plan;		/* inner subnode, or NULL if none */
	List	   *outer_tlist;	/* referent for OUTER_VAR Vars */
	List	   *inner_tlist;	/* referent for INNER_VAR Vars */
	List	   *index_tlist;	/* referent for INDEX_VAR Vars */
	/* Special namespace representing a function signature: */
	char	   *funcname;
	int			numargs;
	char	  **argnames;
} deparse_namespace;
/* …（同文件无关代码省略）… */
	dpns->ancestors = ancestors;
	set_deparse_plan(dpns, plan);

	/* For ModifyTable, set aliases for OLD and NEW in RETURNING */
	if (IsA(plan, ModifyTable))
	{
		dpns->ret_old_alias = ((ModifyTable *) plan)->returningOldAlias;
		dpns->ret_new_alias = ((ModifyTable *) plan)->returningNewAlias;
	}

	return dpcontext;
}

/* …（同文件无关代码省略）… */
 * of a given Plan node
 *
 * This sets the plan, outer_plan, inner_plan, outer_tlist, inner_tlist,
 * and index_tlist fields.  Caller must already have adjusted the ancestors
 * list if necessary.  Note that the rtable, subplans, and ctes fields do
 * not need to change when shifting attention to different plan nodes in a
 * single plan tree.
 */
static void
set_deparse_plan(deparse_namespace *dpns, Plan *plan)
{
	dpns->plan = plan;

	/*
	 * We special-case Append and MergeAppend to pretend that the first child
	 * plan is the OUTER referent; we have to interpret OUTER Vars in their
	 * tlists according to one of the children, and the first one is the most
	 * natural choice.
	 */
	if (IsA(plan, Append))
		dpns->outer_plan = linitial(((Append *) plan)->appendplans);
	else if (IsA(plan, MergeAppend))
		dpns->outer_plan = linitial(((MergeAppend *) plan)->mergeplans);
	else
		dpns->outer_plan = outerPlan(plan);

	if (dpns->outer_plan)
		dpns->outer_tlist = dpns->outer_plan->targetlist;
	else
		dpns->outer_tlist = NIL;

	/*
	 * For a SubqueryScan, pretend the subplan is INNER referent.  (We don't
	 * use OUTER because that could someday conflict with the normal meaning.)
	 * Likewise, for a CteScan, pretend the subquery's plan is INNER referent.
	 * For a WorkTableScan, locate the parent RecursiveUnion plan node and use
	 * that as INNER referent.
	 *
	 * For MERGE, pretend the ModifyTable's source plan (its outer plan) is
	 * INNER referent.  This is the join from the target relation to the data
	 * source, and all INNER_VAR Vars in other parts of the query refer to its
	 * targetlist.
	 *
	 * For ON CONFLICT DO SELECT/UPDATE we just need the inner tlist to point
	 * to the excluded expression's tlist. (Similar to the SubqueryScan we
	 * don't want to reuse OUTER, it's used for RETURNING in some modify table
	 * cases, although not INSERT .. CONFLICT).
	 */
	if (IsA(plan, SubqueryScan))
		dpns->inner_plan = ((SubqueryScan *) plan)->subplan;
	else if (IsA(plan, CteScan))
		dpns->inner_plan = list_nth(dpns->subplans,
									((CteScan *) plan)->ctePlanId - 1);
	else if (IsA(plan, WorkTableScan))
		dpns->inner_plan = find_recursive_union(dpns,
												(WorkTableScan *) plan);
	else if (IsA(plan, ModifyTable))
	{
		if (((ModifyTable *) plan)->operation == CMD_MERGE)
			dpns->inner_plan = outerPlan(plan);
		else
			dpns->inner_plan = plan;
	}
	else
		dpns->inner_plan = innerPlan(plan);

	if (IsA(plan, ModifyTable) && ((ModifyTable *) plan)->operation == CMD_INSERT)
		dpns->inner_tlist = ((ModifyTable *) plan)->exclRelTlist;
	else if (dpns->inner_plan)
		dpns->inner_tlist = dpns->inner_plan->targetlist;
	else
		dpns->inner_tlist = NIL;

	/* Set up referent for INDEX_VAR Vars, if needed */
	if (IsA(plan, IndexOnlyScan))
		dpns->index_tlist = ((IndexOnlyScan *) plan)->indextlist;
	else if (IsA(plan, ForeignScan))
		dpns->index_tlist = ((ForeignScan *) plan)->fdw_scan_tlist;
	else if (IsA(plan, CustomScan))
		dpns->index_tlist = ((CustomScan *) plan)->custom_scan_tlist;
	else
		dpns->index_tlist = NIL;
}

/*
/* …（同文件无关代码省略）… */
static Plan *
find_recursive_union(deparse_namespace *dpns, WorkTableScan *wtscan)
{
	ListCell   *lc;

	foreach(lc, dpns->ancestors)
	{
		Plan	   *ancestor = (Plan *) lfirst(lc);

		if (IsA(ancestor, RecursiveUnion) &&
			((RecursiveUnion *) ancestor)->wtParam == wtscan->wtParam)
			return ancestor;
	}
	elog(ERROR, "could not find RecursiveUnion for WorkTableScan with wtParam %d",
		 wtscan->wtParam);
	return NULL;
}
/* …（同文件无关代码省略）… */

		/* might be returning old/new column value */
		if (var->varreturningtype == VAR_RETURNING_OLD)
			refname = dpns->ret_old_alias;
		else if (var->varreturningtype == VAR_RETURNING_NEW)
			refname = dpns->ret_new_alias;
		else
			refname = (char *) list_nth(dpns->rtable_names, varno - 1);
