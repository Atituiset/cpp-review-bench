// AUTO-DRAFT from postgres/postgres PR #6b858a36ddb80bfa4f478afebe3e84de72600e02
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
typedef struct convert_testexpr_context
{
	PlannerInfo *root;
	List	   *subst_nodes;	/* Nodes to substitute for Params */
} convert_testexpr_context;
/* …（同文件无关代码省略）… */
typedef struct process_sublinks_context
{
	PlannerInfo *root;
	bool		isTopQual;
} process_sublinks_context;

typedef struct finalize_primnode_context
{
	PlannerInfo *root;
	Bitmapset  *paramids;		/* Non-local PARAM_EXEC paramids found */
} finalize_primnode_context;
/* …（同文件无关代码省略）… */
static void
get_first_col_type(Plan *plan, Oid *coltype, int32 *coltypmod,
				   Oid *colcollation)
{
	/* In cases such as EXISTS, tlist might be empty; arbitrarily use VOID */
	if (plan->targetlist)
	{
		TargetEntry *tent = linitial_node(TargetEntry, plan->targetlist);

		if (!tent->resjunk)
		{
			*coltype = exprType((Node *) tent->expr);
			*coltypmod = exprTypmod((Node *) tent->expr);
			*colcollation = exprCollation((Node *) tent->expr);
			return;
		}
	}
	*coltype = VOIDOID;
	*coltypmod = -1;
	*colcollation = InvalidOid;
}
/* …（同文件无关代码省略）… */
static Node *
make_subplan(PlannerInfo *root, Query *orig_subquery,
			 SubLinkType subLinkType, int subLinkId,
			 Node *testexpr, bool isTopQual)
{
	Query	   *subquery;
	bool		simple_exists = false;
	double		tuple_fraction;
	PlannerInfo *subroot;
	RelOptInfo *final_rel;
	Path	   *best_path;
	Plan	   *plan;
	List	   *plan_params;
	Node	   *result;
	const char *sublinkstr = sublinktype_to_string(subLinkType);

	/*
	 * Copy the source Query node.  This is a quick and dirty kluge to resolve
	 * the fact that the parser can generate trees with multiple links to the
	 * same sub-Query node, but the planner wants to scribble on the Query.
	 * Try to clean this up when we do querytree redesign...
	 */
	subquery = copyObject(orig_subquery);

	/*
	 * If it's an EXISTS subplan, we might be able to simplify it.
	 */
	if (subLinkType == EXISTS_SUBLINK)
		simple_exists = simplify_EXISTS_query(root, subquery);

	/*
	 * For an EXISTS subplan, tell lower-level planner to expect that only the
	 * first tuple will be retrieved.  For ALL and ANY subplans, we will be
	 * able to stop evaluating if the test condition fails or matches, so very
	 * often not all the tuples will be retrieved; for lack of a better idea,
	 * specify 50% retrieval.  For EXPR, MULTIEXPR, and ROWCOMPARE subplans,
	 * use default behavior (we're only expecting one row out, anyway).
	 *
	 * NOTE: if you change these numbers, also change cost_subplan() in
	 * path/costsize.c.
	 *
	 * XXX If an ANY subplan is uncorrelated, build_subplan may decide to hash
	 * its output.  In that case it would've been better to specify full
	 * retrieval.  At present, however, we can only check hashability after
	 * we've made the subplan :-(.  (Determining whether it'll fit in hash_mem
	 * is the really hard part.)  Therefore, we don't want to be too
	 * optimistic about the percentage of tuples retrieved, for fear of
	 * selecting a plan that's bad for the materialization case.
	 */
	if (subLinkType == EXISTS_SUBLINK)
		tuple_fraction = 1.0;	/* just like a LIMIT 1 */
	else if (subLinkType == ALL_SUBLINK ||
			 subLinkType == ANY_SUBLINK)
		tuple_fraction = 0.5;	/* 50% */
	else
		tuple_fraction = 0.0;	/* default behavior */

	/* plan_params should not be in use in current query level */
	Assert(root->plan_params == NIL);

	/* Generate Paths for the subquery */
	subroot = subquery_planner(root->glob, subquery,
							   choose_plan_name(root->glob, sublinkstr, true),
							   root, NULL, false, tuple_fraction, NULL);

	/* Isolate the params needed by this specific subplan */
	plan_params = root->plan_params;
	root->plan_params = NIL;

	/*
	 * Select best Path and turn it into a Plan.  At least for now, there
	 * seems no reason to postpone doing that.
	 */
	final_rel = fetch_upper_rel(subroot, UPPERREL_FINAL, NULL);
	best_path = get_cheapest_fractional_path(final_rel, tuple_fraction);

	plan = create_plan(subroot, best_path);

	/* And convert to SubPlan or InitPlan format. */
	result = build_subplan(root, plan, best_path,
						   subroot, plan_params,
						   subLinkType, subLinkId,
						   testexpr, NIL, isTopQual);

	/*
	 * If it's a correlated EXISTS with an unimportant targetlist, we might be
	 * able to transform it to the equivalent of an IN and then implement it
	 * by hashing.  We don't have enough information yet to tell which way is
	 * likely to be better (it depends on the expected number of executions of
	 * the EXISTS qual, and we are much too early in planning the outer query
	 * to be able to guess that).  So we generate both plans, if possible, and
	 * leave it to setrefs.c to decide which to use.
	 */
	if (simple_exists && IsA(result, SubPlan))
	{
		Node	   *newtestexpr;
		List	   *paramIds;

		/* Make a second copy of the original subquery */
		subquery = copyObject(orig_subquery);
		/* and re-simplify */
		simple_exists = simplify_EXISTS_query(root, subquery);
		Assert(simple_exists);
		/* See if it can be converted to an ANY query */
		subquery = convert_EXISTS_to_ANY(root, subquery,
										 &newtestexpr, &paramIds);
		if (subquery)
		{
			char	   *plan_name;

			/*
			 * Generate Paths for the ANY subquery; we'll need all rows. Use a
			 * distinct prefix for this user-visible name, since this is an
			 * ANY implementation of the original EXISTS subplan.
			 */
			plan_name = choose_plan_name(root->glob, "exists_to_any", true);
			subroot = subquery_planner(root->glob, subquery, plan_name,
									   root, subroot, false, 0.0, NULL);

			/* Isolate the params needed by this specific subplan */
			plan_params = root->plan_params;
			root->plan_params = NIL;

			/* Select best Path */
			final_rel = fetch_upper_rel(subroot, UPPERREL_FINAL, NULL);
			best_path = final_rel->cheapest_total_path;

			/* Now we can check if it'll fit in hash_mem */
			if (subpath_is_hashable(best_path, true))
			{
				SubPlan    *hashplan;
				AlternativeSubPlan *asplan;

				/* OK, finish planning the ANY subquery */
				plan = create_plan(subroot, best_path);

				/* ... and convert to SubPlan format */
				hashplan = castNode(SubPlan,
									build_subplan(root, plan, best_path,
												  subroot, plan_params,
												  ANY_SUBLINK, 0,
												  newtestexpr,
												  paramIds,
												  true));
				/* Check we got what we expected */
				Assert(hashplan->parParam == NIL);
				Assert(hashplan->useHashTable);

				/* Leave it to setrefs.c to decide which plan to use */
				asplan = makeNode(AlternativeSubPlan);
				asplan->subplans = list_make2(result, hashplan);
				result = (Node *) asplan;
				root->hasAlternativeSubPlans = true;
			}
		}
	}

	return result;
}
/* …（同文件无关代码省略）… */
static Node *
build_subplan(PlannerInfo *root, Plan *plan, Path *path,
			  PlannerInfo *subroot, List *plan_params,
			  SubLinkType subLinkType, int subLinkId,
			  Node *testexpr, List *testexpr_paramids,
			  bool unknownEqFalse)
{
	Node	   *result;
	SubPlan    *splan;
	ListCell   *lc;

	/*
	 * Initialize the SubPlan node.
	 *
	 * Note: plan_id and cost fields are set further down.
	 */
	splan = makeNode(SubPlan);
	splan->subLinkType = subLinkType;
	splan->plan_name = subroot->plan_name;
	splan->testexpr = NULL;
	splan->paramIds = NIL;
	get_first_col_type(plan, &splan->firstColType, &splan->firstColTypmod,
					   &splan->firstColCollation);
	splan->useHashTable = false;
	splan->unknownEqFalse = unknownEqFalse;
	splan->parallel_safe = plan->parallel_safe;
	splan->setParam = NIL;
	splan->parParam = NIL;
	splan->args = NIL;

	/*
	 * Make parParam and args lists of param IDs and expressions that current
	 * query level will pass to this child plan.
	 */
	foreach(lc, plan_params)
	{
		PlannerParamItem *pitem = (PlannerParamItem *) lfirst(lc);
		Node	   *arg = pitem->item;

		/*
		 * The Var, PlaceHolderVar, Aggref, GroupingFunc, or ReturningExpr has
		 * already been adjusted to have the correct varlevelsup, phlevelsup,
		 * agglevelsup, or retlevelsup.
		 *
		 * If it's an Aggref, GroupingFunc, or ReturningExpr, its arguments
		 * might contain SubLinks, which have not yet been processed (see the
		 * comments for SS_replace_correlation_vars).  Do that now.  A
		 * PlaceHolderVar needs no such treatment: subquery_planner already
		 * preprocessed the PHVs of its owning level, so its expression is
		 * fully processed and may already contain SubPlans.
		 */
		if (IsA(arg, Aggref) ||
			IsA(arg, GroupingFunc) ||
			IsA(arg, ReturningExpr))
			arg = SS_process_sublinks(root, arg, false);

		splan->parParam = lappend_int(splan->parParam, pitem->paramId);
		splan->args = lappend(splan->args, arg);
	}

	/*
	 * Un-correlated or undirect correlated plans of EXISTS, EXPR, ARRAY,
	 * ROWCOMPARE, or MULTIEXPR types can be used as initPlans.  For EXISTS,
	 * EXPR, or ARRAY, we return a Param referring to the result of evaluating
	 * the initPlan.  For ROWCOMPARE, we must modify the testexpr tree to
	 * contain PARAM_EXEC Params instead of the PARAM_SUBLINK Params emitted
	 * by the parser, and then return that tree.  For MULTIEXPR, we return a
	 * null constant: the resjunk targetlist item containing the SubLink does
	 * not need to return anything useful, since the referencing Params are
	 * elsewhere.
	 */
	if (splan->parParam == NIL && subLinkType == EXISTS_SUBLINK)
	{
		Param	   *prm;

		Assert(testexpr == NULL);
		prm = generate_new_exec_param(root, BOOLOID, -1, InvalidOid);
		splan->setParam = list_make1_int(prm->paramid);
		splan->isInitPlan = true;
		result = (Node *) prm;
	}
	else if (splan->parParam == NIL && subLinkType == EXPR_SUBLINK)
	{
		TargetEntry *te = linitial(plan->targetlist);
		Param	   *prm;

		Assert(!te->resjunk);
		Assert(testexpr == NULL);
		prm = generate_new_exec_param(root,
									  exprType((Node *) te->expr),
									  exprTypmod((Node *) te->expr),
									  exprCollation((Node *) te->expr));
		splan->setParam = list_make1_int(prm->paramid);
		splan->isInitPlan = true;
		result = (Node *) prm;
	}
	else if (splan->parParam == NIL && subLinkType == ARRAY_SUBLINK)
	{
		TargetEntry *te = linitial(plan->targetlist);
		Oid			arraytype;
		Param	   *prm;

		Assert(!te->resjunk);
		Assert(testexpr == NULL);
		arraytype = get_promoted_array_type(exprType((Node *) te->expr));
		if (!OidIsValid(arraytype))
			elog(ERROR, "could not find array type for datatype %s",
				 format_type_be(exprType((Node *) te->expr)));
		prm = generate_new_exec_param(root,
									  arraytype,
									  exprTypmod((Node *) te->expr),
									  exprCollation((Node *) te->expr));
		splan->setParam = list_make1_int(prm->paramid);
		splan->isInitPlan = true;
		result = (Node *) prm;
	}
	else if (splan->parParam == NIL && subLinkType == ROWCOMPARE_SUBLINK)
	{
		/* Adjust the Params */
		List	   *params;

		Assert(testexpr != NULL);
		params = generate_subquery_params(root,
										  plan->targetlist,
										  &splan->paramIds);
		result = convert_testexpr(root,
								  testexpr,
								  params);
		splan->setParam = list_copy(splan->paramIds);
		splan->isInitPlan = true;

		/*
		 * The executable expression is returned to become part of the outer
		 * plan's expression tree; it is not kept in the initplan node.
		 */
	}
	else if (subLinkType == MULTIEXPR_SUBLINK)
	{
		/*
		 * Whether it's an initplan or not, it needs to set a PARAM_EXEC Param
		 * for each output column.
		 */
		List	   *params;

		Assert(testexpr == NULL);
		params = generate_subquery_params(root,
										  plan->targetlist,
										  &splan->setParam);

		/*
		 * Save the list of replacement Params in the n'th cell of
		 * root->multiexpr_params; setrefs.c will use it to replace
		 * PARAM_MULTIEXPR Params.
		 */
		while (list_length(root->multiexpr_params) < subLinkId)
			root->multiexpr_params = lappend(root->multiexpr_params, NIL);
		lc = list_nth_cell(root->multiexpr_params, subLinkId - 1);
		Assert(lfirst(lc) == NIL);
		lfirst(lc) = params;

		/* It can be an initplan if there are no parParams. */
		if (splan->parParam == NIL)
		{
			splan->isInitPlan = true;
			result = (Node *) makeNullConst(RECORDOID, -1, InvalidOid);
		}
		else
		{
			splan->isInitPlan = false;
			result = (Node *) splan;
		}
	}
	else
	{
		/*
		 * Adjust the Params in the testexpr, unless caller already took care
		 * of it (as indicated by passing a list of Param IDs).
		 */
		if (testexpr && testexpr_paramids == NIL)
		{
			List	   *params;

			params = generate_subquery_params(root,
											  plan->targetlist,
											  &splan->paramIds);
			splan->testexpr = convert_testexpr(root,
											   testexpr,
											   params);
		}
		else
		{
			splan->testexpr = testexpr;
			splan->paramIds = testexpr_paramids;
		}

		/*
		 * We can't convert subplans of ALL_SUBLINK or ANY_SUBLINK types to
		 * initPlans, even when they are uncorrelated or undirect correlated,
		 * because we need to scan the output of the subplan for each outer
		 * tuple.  But if it's a not-direct-correlated IN (= ANY) test, we
		 * might be able to use a hashtable to avoid comparing all the tuples.
		 */
		if (subLinkType == ANY_SUBLINK &&
			splan->parParam == NIL &&
			subplan_is_hashable(plan, unknownEqFalse) &&
			testexpr_is_hashable(splan->testexpr, splan->paramIds))
			splan->useHashTable = true;

		/*
		 * Otherwise, we have the option to tack a Material node onto the top
		 * of the subplan, to reduce the cost of reading it repeatedly.  This
		 * is pointless for a direct-correlated subplan, since we'd have to
		 * recompute its results each time anyway.  For uncorrelated/undirect
		 * correlated subplans, we add Material unless the subplan's top plan
		 * node would materialize its output anyway.  Also, if enable_material
		 * is false, then the user does not want us to materialize anything
		 * unnecessarily, so we don't.
		 */
		else if (splan->parParam == NIL && enable_material &&
				 !ExecMaterializesOutput(nodeTag(plan)))
			plan = materialize_finished_plan(plan);

		result = (Node *) splan;
		splan->isInitPlan = false;
	}

	/*
	 * Add the subplan, its path, and its PlannerInfo to the global lists.
	 */
	root->glob->subplans = lappend(root->glob->subplans, plan);
	root->glob->subpaths = lappend(root->glob->subpaths, path);
	root->glob->subroots = lappend(root->glob->subroots, subroot);
	splan->plan_id = list_length(root->glob->subplans);

	if (splan->isInitPlan)
		root->init_plans = lappend(root->init_plans, splan);

	/*
	 * A parameterless subplan (not initplan) should be prepared to handle
	 * REWIND efficiently.  If it has direct parameters then there's no point
	 * since it'll be reset on each scan anyway; and if it's an initplan then
	 * there's no point since it won't get re-run without parameter changes
	 * anyway.  The input of a hashed subplan doesn't need REWIND either.
	 */
	if (splan->parParam == NIL && !splan->isInitPlan && !splan->useHashTable)
		root->glob->rewindPlanIDs = bms_add_member(root->glob->rewindPlanIDs,
												   splan->plan_id);

	/* Lastly, fill in the cost estimates for use later */
	cost_subplan(root, splan, plan);

	return result;
}
/* …（同文件无关代码省略）… */
static List *
generate_subquery_params(PlannerInfo *root, List *tlist, List **paramIds)
{
	List	   *result;
	List	   *ids;
	ListCell   *lc;

	result = ids = NIL;
	foreach(lc, tlist)
	{
		TargetEntry *tent = (TargetEntry *) lfirst(lc);
		Param	   *param;

		if (tent->resjunk)
			continue;

		param = generate_new_exec_param(root,
										exprType((Node *) tent->expr),
										exprTypmod((Node *) tent->expr),
										exprCollation((Node *) tent->expr));
		result = lappend(result, param);
		ids = lappend_int(ids, param->paramid);
	}

	*paramIds = ids;
	return result;
}
/* …（同文件无关代码省略）… */
static Node *
convert_testexpr(PlannerInfo *root,
				 Node *testexpr,
				 List *subst_nodes)
{
	convert_testexpr_context context;

	context.root = root;
	context.subst_nodes = subst_nodes;
	return convert_testexpr_mutator(testexpr, &context);
}
/* …（同文件无关代码省略）… */
static Node *
convert_testexpr_mutator(Node *node,
						 convert_testexpr_context *context)
{
	if (node == NULL)
		return NULL;
	if (IsA(node, Param))
	{
		Param	   *param = (Param *) node;

		if (param->paramkind == PARAM_SUBLINK)
		{
			if (param->paramid <= 0 ||
				param->paramid > list_length(context->subst_nodes))
				elog(ERROR, "unexpected PARAM_SUBLINK ID: %d", param->paramid);

			/*
			 * We copy the list item to avoid having doubly-linked
			 * substructure in the modified parse tree.  This is probably
			 * unnecessary when it's a Param, but be safe.
			 */
			return (Node *) copyObject(list_nth(context->subst_nodes,
												param->paramid - 1));
		}
	}
	if (IsA(node, SubLink))
	{
		/*
		 * If we come across a nested SubLink, it is neither necessary nor
		 * correct to recurse into it: any PARAM_SUBLINKs we might find inside
		 * belong to the inner SubLink not the outer. So just return it as-is.
		 *
		 * This reasoning depends on the assumption that nothing will pull
		 * subexpressions into or out of the testexpr field of a SubLink, at
		 * least not without replacing PARAM_SUBLINKs first.  If we did want
		 * to do that we'd need to rethink the parser-output representation
		 * altogether, since currently PARAM_SUBLINKs are only unique per
		 * SubLink not globally across the query.  The whole point of
		 * replacing them with Vars or PARAM_EXEC nodes is to make them
		 * globally unique before they escape from the SubLink's testexpr.
		 *
		 * Note: this can't happen when called during SS_process_sublinks,
		 * because that recursively processes inner SubLinks first.  It can
		 * happen when called from convert_ANY_sublink_to_join, though.
		 */
		return node;
	}
	return expression_tree_mutator(node, convert_testexpr_mutator, context);
}
/* …（同文件无关代码省略）… */
static bool
subplan_is_hashable(Plan *plan, bool unknownEqFalse)
{
	Size		hashtablesize;

	/*
	 * The estimated size of the hashtable holding the subquery result must
	 * fit in hash_mem.  (Note: reject on equality, to ensure that an estimate
	 * of SIZE_MAX disables hashing regardless of the hash_mem limit.)
	 */
	hashtablesize = EstimateSubplanHashTableSpace(plan->plan_rows,
												  plan->plan_width,
												  unknownEqFalse);
	if (hashtablesize >= get_hash_memory_limit())
		return false;

	return true;
}
/* …（同文件无关代码省略）… */
static bool
subpath_is_hashable(Path *path, bool unknownEqFalse)
{
	Size		hashtablesize;

	/*
	 * The estimated size of the hashtable holding the subquery result must
	 * fit in hash_mem.  (Note: reject on equality, to ensure that an estimate
	 * of SIZE_MAX disables hashing regardless of the hash_mem limit.)
	 */
	hashtablesize = EstimateSubplanHashTableSpace(path->rows,
												  path->pathtarget->width,
												  unknownEqFalse);
	if (hashtablesize >= get_hash_memory_limit())
		return false;

	return true;
}
/* …（同文件无关代码省略）… */
static bool
testexpr_is_hashable(Node *testexpr, List *param_ids)
{
	/*
	 * The testexpr must be a single OpExpr, or an AND-clause containing only
	 * OpExprs, each of which satisfy test_opexpr_is_hashable().
	 */
	if (testexpr && IsA(testexpr, OpExpr))
	{
		if (test_opexpr_is_hashable((OpExpr *) testexpr, param_ids))
			return true;
	}
	else if (is_andclause(testexpr))
	{
		ListCell   *l;

		foreach(l, ((BoolExpr *) testexpr)->args)
		{
			Node	   *andarg = (Node *) lfirst(l);

			if (!IsA(andarg, OpExpr))
				return false;
			if (!test_opexpr_is_hashable((OpExpr *) andarg, param_ids))
				return false;
		}
		return true;
	}

	return false;
}
/* …（同文件无关代码省略）… */
static bool
test_opexpr_is_hashable(OpExpr *testexpr, List *param_ids)
{
	/*
	 * The combining operator must be hashable and strict.  The need for
	 * hashability is obvious, since we want to use hashing.  Without
	 * strictness, behavior in the presence of nulls is too unpredictable.  We
	 * actually must assume even more than plain strictness: it can't yield
	 * NULL for non-null inputs, either (see nodeSubplan.c).  However, hash
	 * indexes and hash joins assume that too.
	 */
	if (!hash_ok_operator(testexpr))
		return false;

	/*
	 * The left and right inputs must belong to the outer and inner queries
	 * respectively; hence Params that will be supplied by the subquery must
	 * not appear in the LHS, and Vars of the outer query must not appear in
	 * the RHS.  (Ordinarily, this must be true because of the way that the
	 * parser builds an ANY SubLink's testexpr ... but inlining of functions
	 * could have changed the expression's structure, so we have to check.
	 * Such cases do not occur often enough to be worth trying to optimize, so
	 * we don't worry about trying to commute the clause or anything like
	 * that; we just need to be sure not to build an invalid plan.)
	 */
	if (list_length(testexpr->args) != 2)
		return false;
	if (contain_exec_param((Node *) linitial(testexpr->args), param_ids))
		return false;
	if (contain_var_clause((Node *) lsecond(testexpr->args)))
		return false;
	return true;
}
/* …（同文件无关代码省略）… */
static bool
hash_ok_operator(OpExpr *expr)
{
	Oid			opid = expr->opno;

	/* quick out if not a binary operator */
	if (list_length(expr->args) != 2)
		return false;
	if (opid == ARRAY_EQ_OP ||
		opid == RECORD_EQ_OP ||
		opid == RANGE_EQ_OP ||
		opid == MULTIRANGE_EQ_OP)
	{
		/* these are strict, but must check input type to ensure hashable */
		Node	   *leftarg = linitial(expr->args);

		return op_hashjoinable(opid, exprType(leftarg));
	}
	else
	{
		/* else must look up the operator properties */
		HeapTuple	tup;
		Form_pg_operator optup;

		tup = SearchSysCache1(OPEROID, ObjectIdGetDatum(opid));
		if (!HeapTupleIsValid(tup))
			elog(ERROR, "cache lookup failed for operator %u", opid);
		optup = (Form_pg_operator) GETSTRUCT(tup);
		if (!optup->oprcanhash || !func_strict(optup->oprcode))
		{
			ReleaseSysCache(tup);
			return false;
		}
		ReleaseSysCache(tup);
		return true;
	}
}
/* …（同文件无关代码省略）… */
static bool
simplify_EXISTS_query(PlannerInfo *root, Query *query)
{
	/*
	 * We don't try to simplify at all if the query uses set operations,
	 * aggregates, grouping sets, SRFs, modifying CTEs, HAVING, OFFSET, or FOR
	 * UPDATE/SHARE; none of these seem likely in normal usage and their
	 * possible effects are complex.  (Note: we could ignore an "OFFSET 0"
	 * clause, but that traditionally is used as an optimization fence, so we
	 * don't.)
	 */
	if (query->commandType != CMD_SELECT ||
		query->setOperations ||
		query->hasAggs ||
		query->groupingSets ||
		query->hasWindowFuncs ||
		query->hasTargetSRFs ||
		query->hasModifyingCTE ||
		query->havingQual ||
		query->limitOffset ||
		query->rowMarks)
		return false;

	/*
	 * LIMIT with a constant positive (or NULL) value doesn't affect the
	 * semantics of EXISTS, so let's ignore such clauses.  This is worth doing
	 * because people accustomed to certain other DBMSes may be in the habit
	 * of writing EXISTS(SELECT ... LIMIT 1) as an optimization.  If there's a
	 * LIMIT with anything else as argument, though, we can't simplify.
	 */
	if (query->limitCount)
	{
		/*
		 * The LIMIT clause has not yet been through eval_const_expressions,
		 * so we have to apply that here.  It might seem like this is a waste
		 * of cycles, since the only case plausibly worth worrying about is
		 * "LIMIT 1" ... but what we'll actually see is "LIMIT int8(1::int4)",
		 * so we have to fold constants or we're not going to recognize it.
		 */
		Node	   *node = eval_const_expressions(root, query->limitCount);
		Const	   *limit;

		/* Might as well update the query if we simplified the clause. */
		query->limitCount = node;

		if (!IsA(node, Const))
			return false;

		limit = (Const *) node;
		Assert(limit->consttype == INT8OID);
		if (!limit->constisnull && DatumGetInt64(limit->constvalue) <= 0)
			return false;

		/* Whether or not the targetlist is safe, we can drop the LIMIT. */
		query->limitCount = NULL;
	}

	/*
	 * Otherwise, we can throw away the targetlist, as well as any GROUP,
	 * WINDOW, DISTINCT, and ORDER BY clauses; none of those clauses will
	 * change a nonzero-rows result to zero rows or vice versa.  (Furthermore,
	 * since our parsetree representation of these clauses depends on the
	 * targetlist, we'd better throw them away if we drop the targetlist.)
	 */
	query->targetList = NIL;
	query->groupClause = NIL;
	query->windowClause = NIL;
	query->distinctClause = NIL;
	query->sortClause = NIL;
	query->hasDistinctOn = false;

	/*
	 * Since we have thrown away the GROUP BY clauses, we'd better get rid of
	 * the RTE_GROUP RTE and clear the hasGroupRTE flag.  To safely get rid of
	 * the RTE_GROUP RTE without shifting the index of any subsequent RTE in
	 * the rtable, we convert the RTE to be RTE_RESULT type in-place, and zero
	 * out RTE_GROUP-specific fields.
	 */
	if (query->hasGroupRTE)
	{
		foreach_node(RangeTblEntry, rte, query->rtable)
		{
			if (rte->rtekind == RTE_GROUP)
			{
				rte->rtekind = RTE_RESULT;
				rte->groupexprs = NIL;

				/* A query should only have one RTE_GROUP, so we can stop. */
				break;
			}
		}

		query->hasGroupRTE = false;
	}

	return true;
}
/* …（同文件无关代码省略）… */
static Query *
convert_EXISTS_to_ANY(PlannerInfo *root, Query *subselect,
					  Node **testexpr, List **paramIds)
{
	Node	   *whereClause;
	PlannerInfo subroot;
	List	   *leftargs,
			   *rightargs,
			   *opids,
			   *opcollations,
			   *newWhere,
			   *tlist,
			   *testlist,
			   *paramids;
	ListCell   *lc,
			   *rc,
			   *oc,
			   *cc;
	AttrNumber	resno;

	/*
	 * Query must not require a targetlist, since we have to insert a new one.
	 * Caller should have dealt with the case already.
	 */
	Assert(subselect->targetList == NIL);

	/*
	 * Separate out the WHERE clause.  (We could theoretically also remove
	 * top-level plain JOIN/ON clauses, but it's probably not worth the
	 * trouble.)
	 */
	whereClause = subselect->jointree->quals;
	subselect->jointree->quals = NULL;

	/*
	 * The rest of the sub-select must not refer to any Vars of the parent
	 * query.  (Vars of higher levels should be okay, though.)
	 *
	 * Note: we need not check for Aggrefs separately because we know the
	 * sub-select is as yet unoptimized; any uplevel Aggref must therefore
	 * contain an uplevel Var reference.  This is not the case below ...
	 */
	if (contain_vars_of_level((Node *) subselect, 1))
		return NULL;

	/*
	 * We don't risk optimizing if the WHERE clause is volatile, either.
	 */
	if (contain_volatile_functions(whereClause))
		return NULL;

	/*
	 * Clean up the WHERE clause by doing const-simplification etc on it.
	 * Aside from simplifying the processing we're about to do, this is
	 * important for being able to pull chunks of the WHERE clause up into the
	 * parent query.  Since we are invoked partway through the parent's
	 * preprocess_expression() work, earlier steps of preprocess_expression()
	 * wouldn't get applied to the pulled-up stuff unless we do them here. For
	 * the parts of the WHERE clause that get put back into the child query,
	 * this work is partially duplicative, but it shouldn't hurt.
	 *
	 * Note: we do not run flatten_join_alias_vars.  This is OK because any
	 * parent aliases were flattened already, and we're not going to pull any
	 * child Vars (of any description) into the parent.
	 *
	 * Note: we construct up an entirely dummy PlannerInfo to pass to
	 * eval_const_expressions.  This is fine because only the "glob" and
	 * "parse" links are used by eval_const_expressions.
	 */
	MemSet(&subroot, 0, sizeof(subroot));
	subroot.type = T_PlannerInfo;
	subroot.glob = root->glob;
	subroot.parse = subselect;
	whereClause = eval_const_expressions(&subroot, whereClause);
	whereClause = (Node *) canonicalize_qual((Expr *) whereClause, false);
	whereClause = (Node *) make_ands_implicit((Expr *) whereClause);

	/*
	 * We now have a flattened implicit-AND list of clauses, which we try to
	 * break apart into "outervar = innervar" hash clauses. Anything that
	 * can't be broken apart just goes back into the newWhere list.  Note that
	 * we aren't trying hard yet to ensure that we have only outer or only
	 * inner on each side; we'll check that if we get to the end.
	 */
	leftargs = rightargs = opids = opcollations = newWhere = NIL;
	foreach(lc, (List *) whereClause)
	{
		OpExpr	   *expr = (OpExpr *) lfirst(lc);

		if (IsA(expr, OpExpr) &&
			hash_ok_operator(expr))
		{
			Node	   *leftarg = (Node *) linitial(expr->args);
			Node	   *rightarg = (Node *) lsecond(expr->args);

			if (contain_vars_of_level(leftarg, 1))
			{
				leftargs = lappend(leftargs, leftarg);
				rightargs = lappend(rightargs, rightarg);
				opids = lappend_oid(opids, expr->opno);
				opcollations = lappend_oid(opcollations, expr->inputcollid);
				continue;
			}
			if (contain_vars_of_level(rightarg, 1))
			{
				/*
				 * We must commute the clause to put the outer var on the
				 * left, because the hashing code in nodeSubplan.c expects
				 * that.  This probably shouldn't ever fail, since hashable
				 * operators ought to have commutators, but be paranoid.
				 */
				expr->opno = get_commutator(expr->opno);
				if (OidIsValid(expr->opno) && hash_ok_operator(expr))
				{
					leftargs = lappend(leftargs, rightarg);
					rightargs = lappend(rightargs, leftarg);
					opids = lappend_oid(opids, expr->opno);
					opcollations = lappend_oid(opcollations, expr->inputcollid);
					continue;
				}
				/* If no commutator, no chance to optimize the WHERE clause */
				return NULL;
			}
		}
		/* Couldn't handle it as a hash clause */
		newWhere = lappend(newWhere, expr);
	}

	/*
	 * If we didn't find anything we could convert, fail.
	 */
	if (leftargs == NIL)
		return NULL;

	/*
	 * There mustn't be any parent Vars or Aggs in the stuff that we intend to
	 * put back into the child query.  Note: you might think we don't need to
	 * check for Aggs separately, because an uplevel Agg must contain an
	 * uplevel Var in its argument.  But it is possible that the uplevel Var
	 * got optimized away by eval_const_expressions.  Consider
	 *
	 * SUM(CASE WHEN false THEN uplevelvar ELSE 0 END)
	 */
	if (contain_vars_of_level((Node *) newWhere, 1) ||
		contain_vars_of_level((Node *) rightargs, 1))
		return NULL;
	if (root->parse->hasAggs &&
		(contain_aggs_of_level((Node *) newWhere, 1) ||
		 contain_aggs_of_level((Node *) rightargs, 1)))
		return NULL;

	/*
	 * And there can't be any child Vars in the stuff we intend to pull up.
	 * (Note: we'd need to check for child Aggs too, except we know the child
	 * has no aggs at all because of simplify_EXISTS_query's check. The same
	 * goes for window functions.)
	 */
	if (contain_vars_of_level((Node *) leftargs, 0))
		return NULL;

	/*
	 * Also reject sublinks in the stuff we intend to pull up.  (It might be
	 * possible to support this, but doesn't seem worth the complication.)
	 */
	if (contain_subplans((Node *) leftargs))
		return NULL;

	/*
	 * Okay, adjust the sublevelsup in the stuff we're pulling up.
	 */
	IncrementVarSublevelsUp((Node *) leftargs, -1, 1);

	/*
	 * Put back any child-level-only WHERE clauses.
	 */
	if (newWhere)
		subselect->jointree->quals = (Node *) make_ands_explicit(newWhere);

	/*
	 * Build a new targetlist for the child that emits the expressions we
	 * need.  Concurrently, build a testexpr for the parent using Params to
	 * reference the child outputs.  (Since we generate Params directly here,
	 * there will be no need to convert the testexpr in build_subplan.)
	 */
	tlist = testlist = paramids = NIL;
	resno = 1;
	forfour(lc, leftargs, rc, rightargs, oc, opids, cc, opcollations)
	{
		Node	   *leftarg = (Node *) lfirst(lc);
		Node	   *rightarg = (Node *) lfirst(rc);
		Oid			opid = lfirst_oid(oc);
		Oid			opcollation = lfirst_oid(cc);
		Param	   *param;

		param = generate_new_exec_param(root,
										exprType(rightarg),
										exprTypmod(rightarg),
										exprCollation(rightarg));
		tlist = lappend(tlist,
						makeTargetEntry((Expr *) rightarg,
										resno++,
										NULL,
										false));
		testlist = lappend(testlist,
						   make_opclause(opid, BOOLOID, false,
										 (Expr *) leftarg, (Expr *) param,
										 InvalidOid, opcollation));
		paramids = lappend_int(paramids, param->paramid);
	}

	/* Put everything where it should go, and we're done */
	subselect->targetList = tlist;
	*testexpr = (Node *) make_ands_explicit(testlist);
	*paramIds = paramids;

	return subselect;
}
/* …（同文件无关代码省略）… */
Node *
SS_process_sublinks(PlannerInfo *root, Node *expr, bool isQual)
{
	process_sublinks_context context;

	context.root = root;
	context.isTopQual = isQual;
	return process_sublinks_mutator(expr, &context);
}

static Node *
process_sublinks_mutator(Node *node, process_sublinks_context *context)
{
	process_sublinks_context locContext;

	locContext.root = context->root;

	if (node == NULL)
		return NULL;
	if (IsA(node, SubLink))
	{
		SubLink    *sublink = (SubLink *) node;
		Node	   *testexpr;

		/*
		 * First, recursively process the lefthand-side expressions, if any.
		 * They're not top-level anymore.
		 */
		locContext.isTopQual = false;
		testexpr = process_sublinks_mutator(sublink->testexpr, &locContext);

		/*
		 * Now build the SubPlan node and make the expr to return.
		 */
		return make_subplan(context->root,
							(Query *) sublink->subselect,
							sublink->subLinkType,
							sublink->subLinkId,
							testexpr,
							context->isTopQual);
	}

	/*
	 * Don't recurse into the arguments of an outer PHV, Aggref, GroupingFunc,
	 * or ReturningExpr here.  Any SubLinks in the arguments have to be dealt
	 * with at the outer query level; for an Aggref, GroupingFunc, or
	 * ReturningExpr they'll be handled when build_subplan collects it into
	 * the arguments to be passed down to the current subplan, while an outer
	 * PHV's expression has already been preprocessed by its owning level.
	 */
	if (IsA(node, PlaceHolderVar))
	{
		if (((PlaceHolderVar *) node)->phlevelsup > 0)
			return node;
	}
	else if (IsA(node, Aggref))
	{
		if (((Aggref *) node)->agglevelsup > 0)
			return node;
	}
	else if (IsA(node, GroupingFunc))
	{
		if (((GroupingFunc *) node)->agglevelsup > 0)
			return node;
	}
	else if (IsA(node, ReturningExpr))
	{
		if (((ReturningExpr *) node)->retlevelsup > 0)
			return node;
	}

	/*
	 * We should never see a SubPlan expression in the input (since this is
	 * the very routine that creates 'em to begin with).  We shouldn't find
	 * ourselves invoked directly on a Query, either.
	 */
	Assert(!IsA(node, SubPlan));
	Assert(!IsA(node, AlternativeSubPlan));
	Assert(!IsA(node, Query));

	/*
	 * Because make_subplan() could return an AND or OR clause, we have to
	 * take steps to preserve AND/OR flatness of a qual.  We assume the input
	 * has been AND/OR flattened and so we need no recursion here.
	 *
	 * (Due to the coding here, we will not get called on the List subnodes of
	 * an AND; and the input is *not* yet in implicit-AND format.  So no check
	 * is needed for a bare List.)
	 *
	 * Anywhere within the top-level AND/OR clause structure, we can tell
	 * make_subplan() that NULL and FALSE are interchangeable.  So isTopQual
	 * propagates down in both cases.  (Note that this is unlike the meaning
	 * of "top level qual" used in most other places in Postgres.)
	 */
	if (is_andclause(node))
	{
		List	   *newargs = NIL;
		ListCell   *l;

		/* Still at qual top-level */
		locContext.isTopQual = context->isTopQual;

		foreach(l, ((BoolExpr *) node)->args)
		{
			Node	   *newarg;

			newarg = process_sublinks_mutator(lfirst(l), &locContext);
			if (is_andclause(newarg))
				newargs = list_concat(newargs, ((BoolExpr *) newarg)->args);
			else
				newargs = lappend(newargs, newarg);
		}
		return (Node *) make_andclause(newargs);
	}

	if (is_orclause(node))
	{
		List	   *newargs = NIL;
		ListCell   *l;

		/* Still at qual top-level */
		locContext.isTopQual = context->isTopQual;

		foreach(l, ((BoolExpr *) node)->args)
		{
			Node	   *newarg;

			newarg = process_sublinks_mutator(lfirst(l), &locContext);
			if (is_orclause(newarg))
				newargs = list_concat(newargs, ((BoolExpr *) newarg)->args);
			else
				newargs = lappend(newargs, newarg);
		}
		return (Node *) make_orclause(newargs);
	}

	/*
	 * If we recurse down through anything other than an AND or OR node, we
	 * are definitely not at top qual level anymore.
	 */
	locContext.isTopQual = false;

	return expression_tree_mutator(node,
								   process_sublinks_mutator,
								   &locContext);
}
/* …（同文件无关代码省略）… */
static const char *
sublinktype_to_string(SubLinkType subLinkType)
{
	switch (subLinkType)
	{
		case EXISTS_SUBLINK:
			return "exists";
		case ALL_SUBLINK:
			return "all";
		case ANY_SUBLINK:
			return "any";
		case ROWCOMPARE_SUBLINK:
			return "rowcompare";
		case EXPR_SUBLINK:
			return "expr";
		case MULTIEXPR_SUBLINK:
			return "multiexpr";
		case ARRAY_SUBLINK:
			return "array";
		case CTE_SUBLINK:
			return "cte";
	}
	Assert(false);
	return "???";
}
