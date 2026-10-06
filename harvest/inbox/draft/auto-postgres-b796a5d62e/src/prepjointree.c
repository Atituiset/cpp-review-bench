// AUTO-DRAFT from postgres/postgres PR #d4c15309e7ed6e30e360b5911e1f4d3ca754aa2b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct nullingrel_info
{
	/*
	 * For each leaf RTE, nullingrels[rti] is the set of relids of outer joins
	 * that potentially null that RTE.
	 */
	Relids	   *nullingrels;
	/* Length of range table (maximum index in nullingrels[]) */
	int			rtlength;		/* used only for assertion checks */
} nullingrel_info;
/* …（同文件无关代码省略）… */
typedef enum ReplaceWrapOption
{
	REPLACE_WRAP_NONE,			/* no expressions need to be wrapped */
	REPLACE_WRAP_ALL,			/* all expressions need to be wrapped */
	REPLACE_WRAP_VARFREE,		/* variable-free expressions need to be
								 * wrapped */
} ReplaceWrapOption;
/* …（同文件无关代码省略）… */
typedef struct pullup_replace_vars_context
{
	PlannerInfo *root;
	List	   *targetlist;		/* tlist of subquery being pulled up */
	RangeTblEntry *target_rte;	/* RTE of subquery */
	int			result_relation;	/* the index of the result relation in the
									 * rewritten query */
	Relids		relids;			/* relids within subquery, as numbered after
								 * pullup (set only if target_rte->lateral) */
	nullingrel_info *nullinfo;	/* per-RTE nullingrel info (set only if
								 * target_rte->lateral) */
	bool	   *outer_hasSubLinks;	/* -> outer query's hasSubLinks */
	int			varno;			/* varno of subquery */
	ReplaceWrapOption wrap_option;	/* do we need certain outputs to be PHVs? */
	Node	  **rv_cache;		/* cache for results with PHVs */
} pullup_replace_vars_context;
/* …（同文件无关代码省略）… */
static bool jointree_contains_lateral_outer_refs(PlannerInfo *root,
												 Node *jtnode, bool restricted,
												 Relids safe_upper_varnos);
static void perform_pullup_replace_vars(PlannerInfo *root,
										pullup_replace_vars_context *rvcontext,
										AppendRelInfo *containing_appendrel);
/* …（同文件无关代码省略）… */
Query *
preprocess_relation_rtes(PlannerInfo *root)
{
	Query	   *parse = root->parse;
	int			rtable_size;
	int			rt_index;

	rtable_size = list_length(parse->rtable);

	for (rt_index = 0; rt_index < rtable_size; rt_index++)
	{
		RangeTblEntry *rte = rt_fetch(rt_index + 1, parse->rtable);
		Relation	relation;

		/* We only care about relation RTEs. */
		if (rte->rtekind != RTE_RELATION)
			continue;

		/*
		 * We need not lock the relation since it was already locked by the
		 * rewriter.
		 */
		relation = table_open(rte->relid, NoLock);

		/*
		 * Check to see if the relation actually has any children; if not,
		 * clear the inh flag so we can treat it as a plain base relation.
		 *
		 * Note: this could give a false-positive result, if the rel once had
		 * children but no longer does.  We used to be able to clear rte->inh
		 * later on when we discovered that, but no more; we have to handle
		 * such cases as full-fledged inheritance.
		 */
		if (rte->inh)
			rte->inh = relation->rd_rel->relhassubclass;

		/*
		 * Check to see if the relation has any column not-null constraints;
		 * if so, retrieve the constraint information and store it in a
		 * relation OID based hash table.
		 */
		get_relation_notnullatts(root, relation);

		/*
		 * Check to see if the relation has any virtual generated columns; if
		 * so, replace all Var nodes in the query that reference these columns
		 * with the generation expressions.
		 */
		parse = expand_virtual_generated_columns(root, parse,
												 rte, rt_index + 1,
												 relation);

		table_close(relation, NoLock);
	}

	return parse;
}
/* …（同文件无关代码省略）… */
static Query *
expand_virtual_generated_columns(PlannerInfo *root, Query *parse,
								 RangeTblEntry *rte, int rt_index,
								 Relation relation)
{
	TupleDesc	tupdesc;

	/* Only normal relations can have virtual generated columns */
	Assert(rte->rtekind == RTE_RELATION);

	tupdesc = RelationGetDescr(relation);
	if (tupdesc->constr && tupdesc->constr->has_generated_virtual)
	{
		List	   *tlist = NIL;
		pullup_replace_vars_context rvcontext;
		List	   *save_exclRelTlist = NIL;

		for (int i = 0; i < tupdesc->natts; i++)
		{
			Form_pg_attribute attr = TupleDescAttr(tupdesc, i);
			TargetEntry *tle;

			if (attr->attgenerated == ATTRIBUTE_GENERATED_VIRTUAL)
			{
				Node	   *defexpr;

				defexpr = build_generation_expression(relation, i + 1);
				ChangeVarNodes(defexpr, 1, rt_index, 0);

				tle = makeTargetEntry((Expr *) defexpr, i + 1, 0, false);
				tlist = lappend(tlist, tle);
			}
			else
			{
				Var		   *var;

				var = makeVar(rt_index,
							  i + 1,
							  attr->atttypid,
							  attr->atttypmod,
							  attr->attcollation,
							  0);

				tle = makeTargetEntry((Expr *) var, i + 1, 0, false);
				tlist = lappend(tlist, tle);
			}
		}

		Assert(list_length(tlist) > 0);
		Assert(!rte->lateral);

		/*
		 * The relation's targetlist items are now in the appropriate form to
		 * insert into the query, except that we may need to wrap them in
		 * PlaceHolderVars.  Set up required context data for
		 * pullup_replace_vars.
		 */
		rvcontext.root = root;
		rvcontext.targetlist = tlist;
		rvcontext.target_rte = rte;
		rvcontext.result_relation = parse->resultRelation;
		/* won't need these values */
		rvcontext.relids = NULL;
		rvcontext.nullinfo = NULL;
		/* pass NULL for outer_hasSubLinks */
		rvcontext.outer_hasSubLinks = NULL;
		rvcontext.varno = rt_index;
		/* this flag will be set below, if needed */
		rvcontext.wrap_option = REPLACE_WRAP_NONE;
		/* initialize cache array with indexes 0 .. length(tlist) */
		rvcontext.rv_cache = palloc0_array(Node *, list_length(tlist) + 1);

		/*
		 * If the query uses grouping sets, we need a PlaceHolderVar for each
		 * expression of the relation's targetlist items.  (See comments in
		 * pull_up_simple_subquery().)
		 */
		if (parse->groupingSets)
			rvcontext.wrap_option = REPLACE_WRAP_ALL;

		/*
		 * Apply pullup variable replacement throughout the query tree.
		 *
		 * We intentionally do not touch the EXCLUDED pseudo-relation's
		 * targetlist here.  Various places in the planner assume that it
		 * contains only Vars, and we want that to remain the case.  More
		 * importantly, we don't want setrefs.c to turn any expanded
		 * EXCLUDED.virtual_column expressions in other parts of the query
		 * back into Vars referencing the original virtual column, which
		 * set_plan_refs() would do if exclRelTlist contained matching
		 * expressions.
		 */
		if (parse->onConflict)
		{
			save_exclRelTlist = parse->onConflict->exclRelTlist;
			parse->onConflict->exclRelTlist = NIL;
		}

		parse = (Query *) pullup_replace_vars((Node *) parse, &rvcontext);

		if (parse->onConflict)
			parse->onConflict->exclRelTlist = save_exclRelTlist;
	}

	return parse;
}
/* …（同文件无关代码省略）… */
void
replace_empty_jointree(Query *parse)
{
	RangeTblEntry *rte;
	Index		rti;
	RangeTblRef *rtr;

	/* Nothing to do if jointree is already nonempty */
	if (parse->jointree->fromlist != NIL)
		return;

	/* We mustn't change it in the top level of a setop tree, either */
	if (parse->setOperations)
		return;

	/* Create suitable RTE */
	rte = makeNode(RangeTblEntry);
	rte->rtekind = RTE_RESULT;
	rte->eref = makeAlias("*RESULT*", NIL);

	/* Add it to rangetable */
	parse->rtable = lappend(parse->rtable, rte);
	rti = list_length(parse->rtable);

	/* And jam a reference into the jointree */
	rtr = makeNode(RangeTblRef);
	rtr->rtindex = rti;
	parse->jointree->fromlist = list_make1(rtr);
}
/* …（同文件无关代码省略）… */
void
pull_up_sublinks(PlannerInfo *root)
{
	Node	   *jtnode;
	Relids		relids;

	/* Begin recursion through the jointree */
	jtnode = pull_up_sublinks_jointree_recurse(root,
											   (Node *) root->parse->jointree,
											   &relids);

	/*
	 * root->parse->jointree must always be a FromExpr, so insert a dummy one
	 * if we got a bare RangeTblRef or JoinExpr out of the recursion.
	 */
	if (IsA(jtnode, FromExpr))
		root->parse->jointree = (FromExpr *) jtnode;
	else
		root->parse->jointree = makeFromExpr(list_make1(jtnode), NULL);
}
/* …（同文件无关代码省略）… */
static Node *
pull_up_sublinks_jointree_recurse(PlannerInfo *root, Node *jtnode,
								  Relids *relids)
{
	/* Since this function recurses, it could be driven to stack overflow. */
	check_stack_depth();

	if (jtnode == NULL)
	{
		*relids = NULL;
	}
	else if (IsA(jtnode, RangeTblRef))
	{
		int			varno = ((RangeTblRef *) jtnode)->rtindex;

		*relids = bms_make_singleton(varno);
		/* jtnode is returned unmodified */
	}
	else if (IsA(jtnode, FromExpr))
	{
		FromExpr   *f = (FromExpr *) jtnode;
		List	   *newfromlist = NIL;
		Relids		frelids = NULL;
		FromExpr   *newf;
		Node	   *jtlink;
		ListCell   *l;

		/* First, recurse to process children and collect their relids */
		foreach(l, f->fromlist)
		{
			Node	   *newchild;
			Relids		childrelids;

			newchild = pull_up_sublinks_jointree_recurse(root,
														 lfirst(l),
														 &childrelids);
			newfromlist = lappend(newfromlist, newchild);
			frelids = bms_join(frelids, childrelids);
		}
		/* Build the replacement FromExpr; no quals yet */
		newf = makeFromExpr(newfromlist, NULL);
		/* Set up a link representing the rebuilt jointree */
		jtlink = (Node *) newf;
		/* Now process qual --- all children are available for use */
		newf->quals = pull_up_sublinks_qual_recurse(root, f->quals,
													&jtlink, frelids,
													NULL, NULL);

		/*
		 * Note that the result will be either newf, or a stack of JoinExprs
		 * with newf at the base.  We rely on subsequent optimization steps to
		 * flatten this and rearrange the joins as needed.
		 *
		 * Although we could include the pulled-up subqueries in the returned
		 * relids, there's no need since upper quals couldn't refer to their
		 * outputs anyway.
		 */
		*relids = frelids;
		jtnode = jtlink;
	}
	else if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j;
		Relids		leftrelids;
		Relids		rightrelids;
		Node	   *jtlink;

		/*
		 * Make a modifiable copy of join node, but don't bother copying its
		 * subnodes (yet).
		 */
		j = palloc_object(JoinExpr);
		memcpy(j, jtnode, sizeof(JoinExpr));
		jtlink = (Node *) j;

		/* Recurse to process children and collect their relids */
		j->larg = pull_up_sublinks_jointree_recurse(root, j->larg,
													&leftrelids);
		j->rarg = pull_up_sublinks_jointree_recurse(root, j->rarg,
													&rightrelids);

		/*
		 * Now process qual, showing appropriate child relids as available,
		 * and attach any pulled-up jointree items at the right place. In the
		 * inner-join case we put new JoinExprs above the existing one (much
		 * as for a FromExpr-style join).  In outer-join cases the new
		 * JoinExprs must go into the nullable side of the outer join. The
		 * point of the available_rels machinations is to ensure that we only
		 * pull up quals for which that's okay.
		 *
		 * We don't expect to see any pre-existing JOIN_SEMI, JOIN_ANTI,
		 * JOIN_RIGHT_SEMI, or JOIN_RIGHT_ANTI jointypes here.
		 */
		switch (j->jointype)
		{
			case JOIN_INNER:
				j->quals = pull_up_sublinks_qual_recurse(root, j->quals,
														 &jtlink,
														 bms_union(leftrelids,
																   rightrelids),
														 NULL, NULL);
				break;
			case JOIN_LEFT:
				j->quals = pull_up_sublinks_qual_recurse(root, j->quals,
														 &j->rarg,
														 rightrelids,
														 NULL, NULL);
				break;
			case JOIN_FULL:
				/* can't do anything with full-join quals */
				break;
			case JOIN_RIGHT:
				j->quals = pull_up_sublinks_qual_recurse(root, j->quals,
														 &j->larg,
														 leftrelids,
														 NULL, NULL);
				break;
			default:
				elog(ERROR, "unrecognized join type: %d",
					 (int) j->jointype);
				break;
		}

		/*
		 * Although we could include the pulled-up subqueries in the returned
		 * relids, there's no need since upper quals couldn't refer to their
		 * outputs anyway.  But we *do* need to include the join's own rtindex
		 * because we haven't yet collapsed join alias variables, so upper
		 * levels would mistakenly think they couldn't use references to this
		 * join.
		 */
		*relids = bms_join(leftrelids, rightrelids);
		if (j->rtindex)
			*relids = bms_add_member(*relids, j->rtindex);
		jtnode = jtlink;
	}
	else
		elog(ERROR, "unrecognized node type: %d",
			 (int) nodeTag(jtnode));
	return jtnode;
}
/* …（同文件无关代码省略）… */
void
preprocess_function_rtes(PlannerInfo *root)
{
	ListCell   *rt;

	foreach(rt, root->parse->rtable)
	{
		RangeTblEntry *rte = (RangeTblEntry *) lfirst(rt);

		if (rte->rtekind == RTE_FUNCTION)
		{
			Query	   *funcquery;

			/* Apply const-simplification */
			rte->functions = (List *)
				eval_const_expressions(root, (Node *) rte->functions);

			/* Check safety of expansion, and expand if possible */
			funcquery = inline_function_in_from(root, rte);
			if (funcquery)
			{
				/* Successful expansion, convert the RTE to a subquery */
				rte->rtekind = RTE_SUBQUERY;
				rte->subquery = funcquery;
				rte->security_barrier = false;

				/*
				 * Clear fields that should not be set in a subquery RTE.
				 * However, we leave rte->functions filled in for the moment,
				 * in case makeWholeRowVar needs to consult it.  We'll clear
				 * it in setrefs.c (see add_rte_to_flat_rtable) so that this
				 * abuse of the data structure doesn't escape the planner.
				 */
				rte->funcordinality = false;
			}
		}
	}
}
/* …（同文件无关代码省略）… */
void
pull_up_subqueries(PlannerInfo *root)
{
	/* Top level of jointree must always be a FromExpr */
	Assert(IsA(root->parse->jointree, FromExpr));
	/* Recursion starts with no containing join nor appendrel */
	root->parse->jointree = (FromExpr *)
		pull_up_subqueries_recurse(root, (Node *) root->parse->jointree,
								   NULL, NULL);
	/* We should still have a FromExpr */
	Assert(IsA(root->parse->jointree, FromExpr));
}
/* …（同文件无关代码省略）… */
static Node *
pull_up_subqueries_recurse(PlannerInfo *root, Node *jtnode,
						   JoinExpr *lowest_outer_join,
						   AppendRelInfo *containing_appendrel)
{
	/* Since this function recurses, it could be driven to stack overflow. */
	check_stack_depth();
	/* Also, since it's a bit expensive, let's check for query cancel. */
	CHECK_FOR_INTERRUPTS();

	Assert(jtnode != NULL);
	if (IsA(jtnode, RangeTblRef))
	{
		int			varno = ((RangeTblRef *) jtnode)->rtindex;
		RangeTblEntry *rte = rt_fetch(varno, root->parse->rtable);

		/*
		 * Is this a subquery RTE, and if so, is the subquery simple enough to
		 * pull up?
		 *
		 * If we are looking at an append-relation member, we can't pull it up
		 * unless is_safe_append_member says so.
		 */
		if (rte->rtekind == RTE_SUBQUERY &&
			is_simple_subquery(root, rte->subquery, rte, lowest_outer_join) &&
			(containing_appendrel == NULL ||
			 is_safe_append_member(rte->subquery)))
			return pull_up_simple_subquery(root, jtnode, rte,
										   lowest_outer_join,
										   containing_appendrel);

		/*
		 * Alternatively, is it a simple UNION ALL subquery?  If so, flatten
		 * into an "append relation".
		 *
		 * It's safe to do this regardless of whether this query is itself an
		 * appendrel member.  (If you're thinking we should try to flatten the
		 * two levels of appendrel together, you're right; but we handle that
		 * in set_append_rel_pathlist, not here.)
		 */
		if (rte->rtekind == RTE_SUBQUERY &&
			is_simple_union_all(rte->subquery))
			return pull_up_simple_union_all(root, jtnode, rte);

		/*
		 * Or perhaps it's a simple VALUES RTE?
		 *
		 * We don't allow VALUES pullup below an outer join nor into an
		 * appendrel (such cases are impossible anyway at the moment).
		 */
		if (rte->rtekind == RTE_VALUES &&
			lowest_outer_join == NULL &&
			containing_appendrel == NULL &&
			is_simple_values(root, rte))
			return pull_up_simple_values(root, jtnode, rte);

		/*
		 * Or perhaps it's a FUNCTION RTE that we could inline?
		 */
		if (rte->rtekind == RTE_FUNCTION)
			return pull_up_constant_function(root, jtnode, rte,
											 containing_appendrel);

		/* Otherwise, do nothing at this node. */
	}
	else if (IsA(jtnode, FromExpr))
	{
		FromExpr   *f = (FromExpr *) jtnode;
		ListCell   *l;

		Assert(containing_appendrel == NULL);
		/* Recursively transform all the child nodes */
		foreach(l, f->fromlist)
		{
			lfirst(l) = pull_up_subqueries_recurse(root, lfirst(l),
												   lowest_outer_join,
												   NULL);
		}
	}
	else if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j = (JoinExpr *) jtnode;

		Assert(containing_appendrel == NULL);
		/* Recurse, being careful to tell myself when inside outer join */
		switch (j->jointype)
		{
			case JOIN_INNER:
				j->larg = pull_up_subqueries_recurse(root, j->larg,
													 lowest_outer_join,
													 NULL);
				j->rarg = pull_up_subqueries_recurse(root, j->rarg,
													 lowest_outer_join,
													 NULL);
				break;
			case JOIN_LEFT:
			case JOIN_SEMI:
			case JOIN_ANTI:
				j->larg = pull_up_subqueries_recurse(root, j->larg,
													 j,
													 NULL);
				j->rarg = pull_up_subqueries_recurse(root, j->rarg,
													 j,
													 NULL);
				break;
			case JOIN_FULL:
				j->larg = pull_up_subqueries_recurse(root, j->larg,
													 j,
													 NULL);
				j->rarg = pull_up_subqueries_recurse(root, j->rarg,
													 j,
													 NULL);
				break;
			case JOIN_RIGHT:
				j->larg = pull_up_subqueries_recurse(root, j->larg,
													 j,
													 NULL);
				j->rarg = pull_up_subqueries_recurse(root, j->rarg,
													 j,
													 NULL);
				break;
			default:
				elog(ERROR, "unrecognized join type: %d",
					 (int) j->jointype);
				break;
		}
	}
	else
		elog(ERROR, "unrecognized node type: %d",
			 (int) nodeTag(jtnode));
	return jtnode;
}
/* …（同文件无关代码省略）… */
static Node *
pull_up_simple_subquery(PlannerInfo *root, Node *jtnode, RangeTblEntry *rte,
						JoinExpr *lowest_outer_join,
						AppendRelInfo *containing_appendrel)
{
	Query	   *parse = root->parse;
	int			varno = ((RangeTblRef *) jtnode)->rtindex;
	Query	   *subquery;
	PlannerInfo *subroot;
	int			rtoffset;
	pullup_replace_vars_context rvcontext;
	ListCell   *lc;

	/*
	 * Make a modifiable copy of the subquery to hack on, so that the RTE will
	 * be left unchanged in case we decide below that we can't pull it up
	 * after all.
	 */
	subquery = copyObject(rte->subquery);

	/*
	 * Create a PlannerInfo data structure for this subquery.
	 *
	 * NOTE: the next few steps should match the first processing in
	 * subquery_planner().  Can we refactor to avoid code duplication, or
	 * would that just make things uglier?
	 */
	subroot = makeNode(PlannerInfo);
	subroot->parse = subquery;
	subroot->glob = root->glob;
	subroot->query_level = root->query_level;
	subroot->plan_name = root->plan_name;
	subroot->alternative_plan_name = root->alternative_plan_name;
	subroot->parent_root = root->parent_root;
	subroot->plan_params = NIL;
	subroot->outer_params = NULL;
	subroot->planner_cxt = CurrentMemoryContext;
	subroot->init_plans = NIL;
	subroot->cte_plan_ids = NIL;
	subroot->multiexpr_params = NIL;
	subroot->join_domains = NIL;
	subroot->eq_classes = NIL;
	subroot->ec_merging_done = false;
	subroot->last_rinfo_serial = 0;
	subroot->all_result_relids = NULL;
	subroot->leaf_result_relids = NULL;
	subroot->append_rel_list = NIL;
	subroot->row_identity_vars = NIL;
	subroot->rowMarks = NIL;
	memset(subroot->upper_rels, 0, sizeof(subroot->upper_rels));
	memset(subroot->upper_targets, 0, sizeof(subroot->upper_targets));
	subroot->processed_groupClause = NIL;
	subroot->processed_distinctClause = NIL;
	subroot->processed_tlist = NIL;
	subroot->update_colnos = NIL;
	subroot->grouping_map = NULL;
	subroot->minmax_aggs = NIL;
	subroot->qual_security_level = 0;
	subroot->placeholdersFrozen = false;
	subroot->hasRecursion = false;
	subroot->assumeReplanning = false;
	subroot->wt_param_id = -1;
	subroot->non_recursive_path = NULL;
	/* We don't currently need a top JoinDomain for the subroot */

	/* No CTEs to worry about */
	Assert(subquery->cteList == NIL);

	/*
	 * Scan the rangetable for relation RTEs and retrieve the necessary
	 * catalog information for each relation.  Using this information, clear
	 * the inh flag for any relation that has no children, collect not-null
	 * attribute numbers for any relation that has column not-null
	 * constraints, and expand virtual generated columns for any relation that
	 * contains them.
	 */
	subquery = subroot->parse = preprocess_relation_rtes(subroot);

	/*
	 * If the FROM clause is empty, replace it with a dummy RTE_RESULT RTE, so
	 * that we don't need so many special cases to deal with that situation.
	 */
	replace_empty_jointree(subquery);

	/*
	 * Pull up any SubLinks within the subquery's quals, so that we don't
	 * leave unoptimized SubLinks behind.
	 */
	if (subquery->hasSubLinks)
		pull_up_sublinks(subroot);

	/*
	 * Similarly, preprocess its function RTEs to inline any set-returning
	 * functions in its rangetable.
	 */
	preprocess_function_rtes(subroot);

	/*
	 * Recursively pull up the subquery's subqueries, so that
	 * pull_up_subqueries' processing is complete for its jointree and
	 * rangetable.
	 *
	 * Note: it's okay that the subquery's recursion starts with NULL for
	 * containing-join info, even if we are within an outer join in the upper
	 * query; the lower query starts with a clean slate for outer-join
	 * semantics.  Likewise, we needn't pass down appendrel state.
	 */
	pull_up_subqueries(subroot);

	/*
	 * Now we must recheck whether the subquery is still simple enough to pull
	 * up.  If not, abandon processing it.
	 *
	 * We don't really need to recheck all the conditions involved, but it's
	 * easier just to keep this "if" looking the same as the one in
	 * pull_up_subqueries_recurse.
	 */
	if (is_simple_subquery(root, subquery, rte, lowest_outer_join) &&
		(containing_appendrel == NULL || is_safe_append_member(subquery)))
	{
		/* good to go */
	}
	else
	{
		/*
		 * Give up, return unmodified RangeTblRef.
		 *
		 * Note: The work we just did will be redone when the subquery gets
		 * planned on its own.  Perhaps we could avoid that by storing the
		 * modified subquery back into the rangetable, but I'm not gonna risk
		 * it now.
		 */
		return jtnode;
	}

	/*
	 * We must flatten any join alias Vars in the subquery's targetlist,
	 * because pulling up the subquery's subqueries might have changed their
	 * expansions into arbitrary expressions, which could affect
	 * pullup_replace_vars' decisions about whether PlaceHolderVar wrappers
	 * are needed for tlist entries.  (Likely it'd be better to do
	 * flatten_join_alias_vars on the whole query tree at some earlier stage,
	 * maybe even in the rewriter; but for now let's just fix this case here.)
	 */
	subquery->targetList = (List *)
		flatten_join_alias_vars(subroot, subroot->parse,
								(Node *) subquery->targetList);

	/*
	 * Adjust level-0 varnos in subquery so that we can append its rangetable
	 * to upper query's.  We have to fix the subquery's append_rel_list as
	 * well.
	 */
	rtoffset = list_length(parse->rtable);
	OffsetVarNodes((Node *) subquery, rtoffset, 0);
	OffsetVarNodes((Node *) subroot->append_rel_list, rtoffset, 0);

	/*
	 * Upper-level vars in subquery are now one level closer to their parent
	 * than before.
	 */
	IncrementVarSublevelsUp((Node *) subquery, -1, 1);
	IncrementVarSublevelsUp((Node *) subroot->append_rel_list, -1, 1);

	/*
	 * The subquery's targetlist items are now in the appropriate form to
	 * insert into the top query, except that we may need to wrap them in
	 * PlaceHolderVars.  Set up required context data for pullup_replace_vars.
	 * (Note that we should include the subquery's inner joins in relids,
	 * since it may include join alias vars referencing them.)
	 */
	rvcontext.root = root;
	rvcontext.targetlist = subquery->targetList;
	rvcontext.target_rte = rte;
	rvcontext.result_relation = 0;
	if (rte->lateral)
	{
		rvcontext.relids = get_relids_in_jointree((Node *) subquery->jointree,
												  true, true);
		rvcontext.nullinfo = get_nullingrels(parse);
	}
	else						/* won't need these values */
	{
		rvcontext.relids = NULL;
		rvcontext.nullinfo = NULL;
	}
	rvcontext.outer_hasSubLinks = &parse->hasSubLinks;
	rvcontext.varno = varno;
	/* this flag will be set below, if needed */
	rvcontext.wrap_option = REPLACE_WRAP_NONE;
	/* initialize cache array with indexes 0 .. length(tlist) */
	rvcontext.rv_cache = palloc0_array(Node *, list_length(subquery->targetList) + 1);

	/*
	 * If the parent query uses grouping sets, we need a PlaceHolderVar for
	 * each expression of the subquery's targetlist items.  This ensures that
	 * expressions retain their separate identity so that they will match
	 * grouping set columns when appropriate.  (It'd be sufficient to wrap
	 * values used in grouping set columns, and do so only in non-aggregated
	 * portions of the tlist and havingQual, but that would require a lot of
	 * infrastructure that pullup_replace_vars hasn't currently got.)
	 */
	if (parse->groupingSets)
		rvcontext.wrap_option = REPLACE_WRAP_ALL;

	/*
	 * Replace all of the top query's references to the subquery's outputs
	 * with copies of the adjusted subtlist items, being careful not to
	 * replace any of the jointree structure.
	 */
	perform_pullup_replace_vars(root, &rvcontext,
								containing_appendrel);

	/*
	 * If the subquery had a LATERAL marker, propagate that to any of its
	 * child RTEs that could possibly now contain lateral cross-references.
	 * The children might or might not contain any actual lateral
	 * cross-references, but we have to mark the pulled-up child RTEs so that
	 * later planner stages will check for such.
	 */
	if (rte->lateral)
	{
		foreach(lc, subquery->rtable)
		{
			RangeTblEntry *child_rte = (RangeTblEntry *) lfirst(lc);

			switch (child_rte->rtekind)
			{
				case RTE_RELATION:
					if (child_rte->tablesample)
						child_rte->lateral = true;
					break;
				case RTE_SUBQUERY:
				case RTE_FUNCTION:
				case RTE_VALUES:
				case RTE_TABLEFUNC:
					child_rte->lateral = true;
					break;
				case RTE_JOIN:
				case RTE_CTE:
				case RTE_NAMEDTUPLESTORE:
				case RTE_RESULT:
				case RTE_GROUP:
					/* these can't contain any lateral references */
					break;
			}
		}
	}

	/*
	 * Now append the adjusted rtable entries and their perminfos to upper
	 * query. (We hold off until after fixing the upper rtable entries; no
	 * point in running that code on the subquery ones too.)
	 */
	CombineRangeTables(&parse->rtable, &parse->rteperminfos,
					   subquery->rtable, subquery->rteperminfos);

	/*
	 * Pull up any FOR UPDATE/SHARE markers, too.  (OffsetVarNodes already
	 * adjusted the marker rtindexes, so just concat the lists.)
	 */
	parse->rowMarks = list_concat(parse->rowMarks, subquery->rowMarks);

	/*
	 * We also have to fix the relid sets of any PlaceHolderVar nodes in the
	 * parent query.  (This could perhaps be done by pullup_replace_vars(),
	 * but it seems cleaner to use two passes.)  Note in particular that any
	 * PlaceHolderVar nodes just created by pullup_replace_vars() will be
	 * adjusted, so having created them with the subquery's varno is correct.
	 *
	 * Likewise, relids appearing in AppendRelInfo nodes have to be fixed. We
	 * already checked that this won't require introducing multiple subrelids
	 * into the single-slot AppendRelInfo structs.
	 */
	if (root->glob->lastPHId != 0 || root->append_rel_list)
	{
		Relids		subrelids;

		subrelids = get_relids_in_jointree((Node *) subquery->jointree,
										   true, false);
		if (root->glob->lastPHId != 0)
			substitute_phv_relids((Node *) parse, varno, subrelids);
		fix_append_rel_relids(root, varno, subrelids);
	}

	/*
	 * And now add subquery's AppendRelInfos to our list.
	 */
	root->append_rel_list = list_concat(root->append_rel_list,
										subroot->append_rel_list);

	/*
	 * We don't have to do the equivalent bookkeeping for outer-join info,
	 * because that hasn't been set up yet.  placeholder_list likewise.
	 */
	Assert(root->join_info_list == NIL);
	Assert(subroot->join_info_list == NIL);
	Assert(root->placeholder_list == NIL);
	Assert(subroot->placeholder_list == NIL);

	/*
	 * We no longer need the RTE's copy of the subquery's query tree.  Getting
	 * rid of it saves nothing in particular so far as this level of query is
	 * concerned; but if this query level is in turn pulled up into a parent,
	 * we'd waste cycles copying the now-unused query tree.
	 */
	rte->subquery = NULL;

	/*
	 * Miscellaneous housekeeping.
	 *
	 * Although replace_rte_variables() faithfully updated parse->hasSubLinks
	 * if it copied any SubLinks out of the subquery's targetlist, we still
	 * could have SubLinks added to the query in the expressions of FUNCTION
	 * and VALUES RTEs copied up from the subquery.  So it's necessary to copy
	 * subquery->hasSubLinks anyway.  Perhaps this can be improved someday.
	 */
	parse->hasSubLinks |= subquery->hasSubLinks;

	/* If subquery had any RLS conditions, now main query does too */
	parse->hasRowSecurity |= subquery->hasRowSecurity;

	/*
	 * subquery won't be pulled up if it hasAggs, hasWindowFuncs, or
	 * hasTargetSRFs, so no work needed on those flags
	 */

	/*
	 * Return the adjusted subquery jointree to replace the RangeTblRef entry
	 * in parent's jointree; or, if the FromExpr is degenerate, just return
	 * its single member.
	 */
	Assert(IsA(subquery->jointree, FromExpr));
	Assert(subquery->jointree->fromlist != NIL);
	if (subquery->jointree->quals == NULL &&
		list_length(subquery->jointree->fromlist) == 1)
		return (Node *) linitial(subquery->jointree->fromlist);

	return (Node *) subquery->jointree;
}
/* …（同文件无关代码省略）… */
static Node *
pull_up_simple_union_all(PlannerInfo *root, Node *jtnode, RangeTblEntry *rte)
{
	int			varno = ((RangeTblRef *) jtnode)->rtindex;
	Query	   *subquery = rte->subquery;
	int			rtoffset = list_length(root->parse->rtable);
	List	   *rtable;

	/*
	 * Make a modifiable copy of the subquery's rtable, so we can adjust
	 * upper-level Vars in it.  There are no such Vars in the setOperations
	 * tree proper, so fixing the rtable should be sufficient.
	 */
	rtable = copyObject(subquery->rtable);

	/*
	 * Upper-level vars in subquery are now one level closer to their parent
	 * than before.  We don't have to worry about offsetting varnos, though,
	 * because the UNION leaf queries can't cross-reference each other.
	 */
	IncrementVarSublevelsUp_rtable(rtable, -1, 1);

	/*
	 * If the UNION ALL subquery had a LATERAL marker, propagate that to all
	 * its children.  The individual children might or might not contain any
	 * actual lateral cross-references, but we have to mark the pulled-up
	 * child RTEs so that later planner stages will check for such.
	 */
	if (rte->lateral)
	{
		ListCell   *rt;

		foreach(rt, rtable)
		{
			RangeTblEntry *child_rte = (RangeTblEntry *) lfirst(rt);

			Assert(child_rte->rtekind == RTE_SUBQUERY);
			child_rte->lateral = true;
		}
	}

	/*
	 * Append child RTEs (and their perminfos) to parent rtable.
	 */
	CombineRangeTables(&root->parse->rtable, &root->parse->rteperminfos,
					   rtable, subquery->rteperminfos);

	/*
	 * Recursively scan the subquery's setOperations tree and add
	 * AppendRelInfo nodes for leaf subqueries to the parent's
	 * append_rel_list.  Also apply pull_up_subqueries to the leaf subqueries.
	 */
	Assert(subquery->setOperations);
	pull_up_union_leaf_queries(subquery->setOperations, root, varno, subquery,
							   rtoffset);

	/*
	 * Mark the parent as an append relation.
	 */
	rte->inh = true;

	return jtnode;
}
/* …（同文件无关代码省略）… */
static void
pull_up_union_leaf_queries(Node *setOp, PlannerInfo *root, int parentRTindex,
						   Query *setOpQuery, int childRToffset)
{
	if (IsA(setOp, RangeTblRef))
	{
		RangeTblRef *rtr = (RangeTblRef *) setOp;
		int			childRTindex;
		AppendRelInfo *appinfo;

		/*
		 * Calculate the index in the parent's range table
		 */
		childRTindex = childRToffset + rtr->rtindex;

		/*
		 * Build a suitable AppendRelInfo, and attach to parent's list.
		 */
		appinfo = makeNode(AppendRelInfo);
		appinfo->parent_relid = parentRTindex;
		appinfo->child_relid = childRTindex;
		appinfo->parent_reltype = InvalidOid;
		appinfo->child_reltype = InvalidOid;
		make_setop_translation_list(setOpQuery, childRTindex, appinfo);
		appinfo->parent_reloid = InvalidOid;
		root->append_rel_list = lappend(root->append_rel_list, appinfo);

		/*
		 * Recursively apply pull_up_subqueries to the new child RTE.  (We
		 * must build the AppendRelInfo first, because this will modify it;
		 * indeed, that's the only part of the upper query where Vars
		 * referencing childRTindex can exist at this point.)
		 *
		 * Note that we can pass NULL for containing-join info even if we're
		 * actually under an outer join, because the child's expressions
		 * aren't going to propagate up to the join.  Also, we ignore the
		 * possibility that pull_up_subqueries_recurse() returns a different
		 * jointree node than what we pass it; if it does, the important thing
		 * is that it replaced the child relid in the AppendRelInfo node.
		 */
		rtr = makeNode(RangeTblRef);
		rtr->rtindex = childRTindex;
		(void) pull_up_subqueries_recurse(root, (Node *) rtr,
										  NULL, appinfo);
	}
	else if (IsA(setOp, SetOperationStmt))
	{
		SetOperationStmt *op = (SetOperationStmt *) setOp;

		/* Recurse to reach leaf queries */
		pull_up_union_leaf_queries(op->larg, root, parentRTindex, setOpQuery,
								   childRToffset);
		pull_up_union_leaf_queries(op->rarg, root, parentRTindex, setOpQuery,
								   childRToffset);
	}
	else
	{
		elog(ERROR, "unrecognized node type: %d",
			 (int) nodeTag(setOp));
	}
}
/* …（同文件无关代码省略）… */
static void
make_setop_translation_list(Query *query, int newvarno,
							AppendRelInfo *appinfo)
{
	List	   *vars = NIL;
	AttrNumber *pcolnos;
	ListCell   *l;

	/* Initialize reverse-translation array with all entries zero */
	/* (entries for resjunk columns will stay that way) */
	appinfo->num_child_cols = list_length(query->targetList);
	appinfo->parent_colnos = pcolnos = palloc0_array(AttrNumber, appinfo->num_child_cols);

	foreach(l, query->targetList)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(l);

		if (tle->resjunk)
			continue;

		vars = lappend(vars, makeVarFromTargetEntry(newvarno, tle));
		pcolnos[tle->resno - 1] = tle->resno;
	}

	appinfo->translated_vars = vars;
}
/* …（同文件无关代码省略）… */
static bool
is_simple_subquery(PlannerInfo *root, Query *subquery, RangeTblEntry *rte,
				   JoinExpr *lowest_outer_join)
{
	/*
	 * Let's just make sure it's a valid subselect ...
	 */
	if (!IsA(subquery, Query) ||
		subquery->commandType != CMD_SELECT)
		elog(ERROR, "subquery is bogus");

	/*
	 * Can't currently pull up a query with setops (unless it's simple UNION
	 * ALL, which is handled by a different code path). Maybe after querytree
	 * redesign...
	 */
	if (subquery->setOperations)
		return false;

	/*
	 * Can't pull up a subquery involving grouping, aggregation, SRFs,
	 * sorting, limiting, or WITH.  (XXX WITH could possibly be allowed later)
	 *
	 * We also don't pull up a subquery that has explicit FOR UPDATE/SHARE
	 * clauses, because pullup would cause the locking to occur semantically
	 * higher than it should.  Implicit FOR UPDATE/SHARE is okay because in
	 * that case the locking was originally declared in the upper query
	 * anyway.
	 */
	if (subquery->hasAggs ||
		subquery->hasWindowFuncs ||
		subquery->hasTargetSRFs ||
		subquery->groupClause ||
		subquery->groupingSets ||
		subquery->havingQual ||
		subquery->sortClause ||
		subquery->distinctClause ||
		subquery->limitOffset ||
		subquery->limitCount ||
		subquery->hasForUpdate ||
		subquery->cteList)
		return false;

	/*
	 * Don't pull up if the RTE represents a security-barrier view; we
	 * couldn't prevent information leakage once the RTE's Vars are scattered
	 * about in the upper query.
	 */
	if (rte->security_barrier)
		return false;

	/*
	 * If the subquery is LATERAL, check for pullup restrictions from that.
	 */
	if (rte->lateral)
	{
		bool		restricted;
		Relids		safe_upper_varnos;

		/*
		 * The subquery's WHERE and JOIN/ON quals mustn't contain any lateral
		 * references to rels outside a higher outer join (including the case
		 * where the outer join is within the subquery itself).  In such a
		 * case, pulling up would result in a situation where we need to
		 * postpone quals from below an outer join to above it, which is
		 * probably completely wrong and in any case is a complication that
		 * doesn't seem worth addressing at the moment.
		 */
		if (lowest_outer_join != NULL)
		{
			restricted = true;
			safe_upper_varnos = get_relids_in_jointree((Node *) lowest_outer_join,
													   true, true);
		}
		else
		{
			restricted = false;
			safe_upper_varnos = NULL;	/* doesn't matter */
		}

		if (jointree_contains_lateral_outer_refs(root,
												 (Node *) subquery->jointree,
												 restricted, safe_upper_varnos))
			return false;

		/*
		 * If there's an outer join above the LATERAL subquery, also disallow
		 * pullup if the subquery's targetlist has any references to rels
		 * outside the outer join, since these might get pulled into quals
		 * above the subquery (but in or below the outer join) and then lead
		 * to qual-postponement issues similar to the case checked for above.
		 * (We wouldn't need to prevent pullup if no such references appear in
		 * outer-query quals, but we don't have enough info here to check
		 * that.  Also, maybe this restriction could be removed if we forced
		 * such refs to be wrapped in PlaceHolderVars, even when they're below
		 * the nearest outer join?	But it's a pretty hokey usage, so not
		 * clear this is worth sweating over.)
		 *
		 * If you change this, see also the comments about lateral references
		 * in pullup_replace_vars_callback().
		 */
		if (lowest_outer_join != NULL)
		{
			Relids		lvarnos = pull_varnos_of_level(root,
													   (Node *) subquery->targetList,
													   1);

			if (!bms_is_subset(lvarnos, safe_upper_varnos))
				return false;
		}
	}

	/*
	 * Don't pull up a subquery that has any volatile functions in its
	 * targetlist.  Otherwise we might introduce multiple evaluations of these
	 * functions, if they get copied to multiple places in the upper query,
	 * leading to surprising results.  (Note: the PlaceHolderVar mechanism
	 * doesn't quite guarantee single evaluation; else we could pull up anyway
	 * and just wrap such items in PlaceHolderVars ...)
	 */
	if (contain_volatile_functions((Node *) subquery->targetList))
		return false;

	return true;
}
/* …（同文件无关代码省略）… */
static Node *
pull_up_simple_values(PlannerInfo *root, Node *jtnode, RangeTblEntry *rte)
{
	Query	   *parse = root->parse;
	int			varno = ((RangeTblRef *) jtnode)->rtindex;
	List	   *values_list;
	List	   *tlist;
	AttrNumber	attrno;
	pullup_replace_vars_context rvcontext;
	ListCell   *lc;

	Assert(rte->rtekind == RTE_VALUES);
	Assert(list_length(rte->values_lists) == 1);

	/*
	 * Need a modifiable copy of the VALUES list to hack on, just in case it's
	 * multiply referenced.
	 */
	values_list = copyObject(linitial(rte->values_lists));

	/*
	 * The VALUES RTE can't contain any Vars of level zero, let alone any that
	 * are join aliases, so no need to flatten join alias Vars.
	 */
	Assert(!contain_vars_of_level((Node *) values_list, 0));

	/*
	 * Set up required context data for pullup_replace_vars.  In particular,
	 * we have to make the VALUES list look like a subquery targetlist.
	 */
	tlist = NIL;
	attrno = 1;
	foreach(lc, values_list)
	{
		tlist = lappend(tlist,
						makeTargetEntry((Expr *) lfirst(lc),
										attrno,
										NULL,
										false));
		attrno++;
	}
	rvcontext.root = root;
	rvcontext.targetlist = tlist;
	rvcontext.target_rte = rte;
	rvcontext.result_relation = 0;
	rvcontext.relids = NULL;	/* can't be any lateral references here */
	rvcontext.nullinfo = NULL;
	rvcontext.outer_hasSubLinks = &parse->hasSubLinks;
	rvcontext.varno = varno;
	rvcontext.wrap_option = REPLACE_WRAP_NONE;
	/* initialize cache array with indexes 0 .. length(tlist) */
	rvcontext.rv_cache = palloc0_array(Node *, list_length(tlist) + 1);

	/*
	 * Replace all of the top query's references to the RTE's outputs with
	 * copies of the adjusted VALUES expressions, being careful not to replace
	 * any of the jointree structure.  We can assume there's no outer joins or
	 * appendrels in the dummy Query that surrounds a VALUES RTE.
	 */
	perform_pullup_replace_vars(root, &rvcontext, NULL);

	/*
	 * There should be no appendrels to fix, nor any outer joins and hence no
	 * PlaceHolderVars.
	 */
	Assert(root->append_rel_list == NIL);
	Assert(root->join_info_list == NIL);
	Assert(root->placeholder_list == NIL);

	/*
	 * Replace the VALUES RTE with a RESULT RTE.  The VALUES RTE is the only
	 * rtable entry in the current query level, so this is easy.
	 */
	Assert(list_length(parse->rtable) == 1);

	/* Create suitable RTE */
	rte = makeNode(RangeTblEntry);
	rte->rtekind = RTE_RESULT;
	rte->eref = makeAlias("*RESULT*", NIL);

	/* Replace rangetable */
	parse->rtable = list_make1(rte);

	/* We could manufacture a new RangeTblRef, but the one we have is fine */
	Assert(varno == 1);

	return jtnode;
}
/* …（同文件无关代码省略）… */
static bool
is_simple_values(PlannerInfo *root, RangeTblEntry *rte)
{
	Assert(rte->rtekind == RTE_VALUES);

	/*
	 * There must be exactly one VALUES list, else it's not semantically
	 * correct to replace the VALUES RTE with a RESULT RTE, nor would we have
	 * a unique set of expressions to substitute into the parent query.
	 */
	if (list_length(rte->values_lists) != 1)
		return false;

	/*
	 * Because VALUES can't appear under an outer join (or at least, we won't
	 * try to pull it up if it does), we need not worry about LATERAL, nor
	 * about validity of PHVs for the VALUES' outputs.
	 */

	/*
	 * Don't pull up a VALUES that contains any set-returning or volatile
	 * functions.  The considerations here are basically identical to the
	 * restrictions on a pull-able subquery's targetlist.
	 */
	if (expression_returns_set((Node *) rte->values_lists) ||
		contain_volatile_functions((Node *) rte->values_lists))
		return false;

	/*
	 * Do not pull up a VALUES that's not the only RTE in its parent query.
	 * This is actually the only case that the parser will generate at the
	 * moment, and assuming this is true greatly simplifies
	 * pull_up_simple_values().
	 */
	if (list_length(root->parse->rtable) != 1 ||
		rte != (RangeTblEntry *) linitial(root->parse->rtable))
		return false;

	return true;
}
/* …（同文件无关代码省略）… */
static Node *
pull_up_constant_function(PlannerInfo *root, Node *jtnode,
						  RangeTblEntry *rte,
						  AppendRelInfo *containing_appendrel)
{
	Query	   *parse = root->parse;
	RangeTblFunction *rtf;
	TypeFuncClass functypclass;
	Oid			funcrettype;
	TupleDesc	tupdesc;
	pullup_replace_vars_context rvcontext;

	/* Fail if the RTE has ORDINALITY - we don't implement that here. */
	if (rte->funcordinality)
		return jtnode;

	/* Fail if RTE isn't a single, simple Const expr */
	if (list_length(rte->functions) != 1)
		return jtnode;
	rtf = linitial_node(RangeTblFunction, rte->functions);
	if (!IsA(rtf->funcexpr, Const))
		return jtnode;

	/*
	 * If the function's result is not a scalar, we punt.  In principle we
	 * could break the composite constant value apart into per-column
	 * constants, but for now it seems not worth the work.
	 */
	if (rtf->funccolcount != 1)
		return jtnode;			/* definitely composite */

	/* If it has a coldeflist, it certainly returns RECORD */
	if (rtf->funccolnames != NIL)
		return jtnode;			/* must be a one-column RECORD type */

	functypclass = get_expr_result_type(rtf->funcexpr,
										&funcrettype,
										&tupdesc);
	if (functypclass != TYPEFUNC_SCALAR)
		return jtnode;			/* must be a one-column composite type */

	/* Create context for applying pullup_replace_vars */
	rvcontext.root = root;
	rvcontext.targetlist = list_make1(makeTargetEntry((Expr *) rtf->funcexpr,
													  1,	/* resno */
													  NULL, /* resname */
													  false));	/* resjunk */
	rvcontext.target_rte = rte;
	rvcontext.result_relation = 0;

	/*
	 * Since this function was reduced to a Const, it doesn't contain any
	 * lateral references, even if it's marked as LATERAL.  This means we
	 * don't need to fill relids or nullinfo.
	 */
	rvcontext.relids = NULL;
	rvcontext.nullinfo = NULL;

	rvcontext.outer_hasSubLinks = &parse->hasSubLinks;
	rvcontext.varno = ((RangeTblRef *) jtnode)->rtindex;
	/* this flag will be set below, if needed */
	rvcontext.wrap_option = REPLACE_WRAP_NONE;
	/* initialize cache array with indexes 0 .. length(tlist) */
	rvcontext.rv_cache = palloc0_array(Node *, list_length(rvcontext.targetlist) + 1);

	/*
	 * If the parent query uses grouping sets, we need a PlaceHolderVar for
	 * each expression of the subquery's targetlist items.  (See comments in
	 * pull_up_simple_subquery().)
	 */
	if (parse->groupingSets)
		rvcontext.wrap_option = REPLACE_WRAP_ALL;

	/*
	 * Replace all of the top query's references to the RTE's output with
	 * copies of the funcexpr, being careful not to replace any of the
	 * jointree structure.
	 */
	perform_pullup_replace_vars(root, &rvcontext,
								containing_appendrel);

	/*
	 * We don't need to bother with changing PlaceHolderVars in the parent
	 * query.  Their references to the RT index are still good for now, and
	 * will get removed later if we're able to drop the RTE_RESULT.
	 */

	/*
	 * Convert the RTE to be RTE_RESULT type, signifying that we don't need to
	 * scan it anymore, and zero out RTE_FUNCTION-specific fields.  Also make
	 * sure the RTE is not marked LATERAL, since elsewhere we don't expect
	 * RTE_RESULTs to be LATERAL.
	 */
	rte->rtekind = RTE_RESULT;
	rte->functions = NIL;
	rte->lateral = false;

	/*
	 * We can reuse the RangeTblRef node.
	 */
	return jtnode;
}
/* …（同文件无关代码省略）… */
static bool
is_simple_union_all(Query *subquery)
{
	SetOperationStmt *topop;

	/* Let's just make sure it's a valid subselect ... */
	if (!IsA(subquery, Query) ||
		subquery->commandType != CMD_SELECT)
		elog(ERROR, "subquery is bogus");

	/* Is it a set-operation query at all? */
	topop = castNode(SetOperationStmt, subquery->setOperations);
	if (!topop)
		return false;

	/* Can't handle ORDER BY, LIMIT/OFFSET, locking, or WITH */
	if (subquery->sortClause ||
		subquery->limitOffset ||
		subquery->limitCount ||
		subquery->rowMarks ||
		subquery->cteList)
		return false;

	/* Recursively check the tree of set operations */
	return is_simple_union_all_recurse((Node *) topop, subquery,
									   topop->colTypes);
}
/* …（同文件无关代码省略）… */
static bool
is_simple_union_all_recurse(Node *setOp, Query *setOpQuery, List *colTypes)
{
	/* Since this function recurses, it could be driven to stack overflow. */
	check_stack_depth();

	if (IsA(setOp, RangeTblRef))
	{
		RangeTblRef *rtr = (RangeTblRef *) setOp;
		RangeTblEntry *rte = rt_fetch(rtr->rtindex, setOpQuery->rtable);
		Query	   *subquery = rte->subquery;

		Assert(subquery != NULL);

		/* Leaf nodes are OK if they match the toplevel column types */
		/* We don't have to compare typmods or collations here */
		return tlist_same_datatypes(subquery->targetList, colTypes, true);
	}
	else if (IsA(setOp, SetOperationStmt))
	{
		SetOperationStmt *op = (SetOperationStmt *) setOp;

		/* Must be UNION ALL */
		if (op->op != SETOP_UNION || !op->all)
			return false;

		/* Recurse to check inputs */
		return is_simple_union_all_recurse(op->larg, setOpQuery, colTypes) &&
			is_simple_union_all_recurse(op->rarg, setOpQuery, colTypes);
	}
	else
	{
		elog(ERROR, "unrecognized node type: %d",
			 (int) nodeTag(setOp));
		return false;			/* keep compiler quiet */
	}
}
/* …（同文件无关代码省略）… */
static bool
is_safe_append_member(Query *subquery)
{
	FromExpr   *jtnode;

	/*
	 * It's only safe to pull up the child if its jointree contains exactly
	 * one RTE, else the AppendRelInfo data structure breaks. The one base RTE
	 * could be buried in several levels of FromExpr, however.  Also, if the
	 * child's jointree is completely empty, we can pull up because
	 * pull_up_simple_subquery will insert a single RTE_RESULT RTE instead.
	 *
	 * Also, the child can't have any WHERE quals because there's no place to
	 * put them in an appendrel.  (This is a bit annoying...) If we didn't
	 * need to check this, we'd just test whether get_relids_in_jointree()
	 * yields a singleton set, to be more consistent with the coding of
	 * fix_append_rel_relids().
	 */
	jtnode = subquery->jointree;
	Assert(IsA(jtnode, FromExpr));
	/* Check the completely-empty case */
	if (jtnode->fromlist == NIL && jtnode->quals == NULL)
		return true;
	/* Check the more general case */
	while (IsA(jtnode, FromExpr))
	{
		if (jtnode->quals != NULL)
			return false;
		if (list_length(jtnode->fromlist) != 1)
			return false;
		jtnode = linitial(jtnode->fromlist);
	}
	if (!IsA(jtnode, RangeTblRef))
		return false;

	return true;
}
/* …（同文件无关代码省略）… */
static bool
jointree_contains_lateral_outer_refs(PlannerInfo *root, Node *jtnode,
									 bool restricted,
									 Relids safe_upper_varnos)
{
	if (jtnode == NULL)
		return false;
	if (IsA(jtnode, RangeTblRef))
		return false;
	else if (IsA(jtnode, FromExpr))
	{
		FromExpr   *f = (FromExpr *) jtnode;
		ListCell   *l;

		/* First, recurse to check child joins */
		foreach(l, f->fromlist)
		{
			if (jointree_contains_lateral_outer_refs(root,
													 lfirst(l),
													 restricted,
													 safe_upper_varnos))
				return true;
		}

		/* Then check the top-level quals */
		if (restricted &&
			!bms_is_subset(pull_varnos_of_level(root, f->quals, 1),
						   safe_upper_varnos))
			return true;
	}
	else if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j = (JoinExpr *) jtnode;

		/*
		 * If this is an outer join, we mustn't allow any upper lateral
		 * references in or below it.
		 */
		if (j->jointype != JOIN_INNER)
		{
			restricted = true;
			safe_upper_varnos = NULL;
		}

		/* Check the child joins */
		if (jointree_contains_lateral_outer_refs(root,
												 j->larg,
												 restricted,
												 safe_upper_varnos))
			return true;
		if (jointree_contains_lateral_outer_refs(root,
												 j->rarg,
												 restricted,
												 safe_upper_varnos))
			return true;

		/* Check the JOIN's qual clauses */
		if (restricted &&
			!bms_is_subset(pull_varnos_of_level(root, j->quals, 1),
						   safe_upper_varnos))
			return true;
	}
	else
		elog(ERROR, "unrecognized node type: %d",
			 (int) nodeTag(jtnode));
	return false;
}

/*
 * Perform pullup_replace_vars everyplace it's needed in the query tree.
 *
/* …（同文件无关代码省略）… */
static void
perform_pullup_replace_vars(PlannerInfo *root,
							pullup_replace_vars_context *rvcontext,
							AppendRelInfo *containing_appendrel)
{
	Query	   *parse = root->parse;
	ListCell   *lc;

	/*
	 * If we are considering an appendrel child subquery (that is, a UNION ALL
	 * member query that we're pulling up), then the only part of the upper
	 * query that could reference the child yet is the translated_vars list of
	 * the associated AppendRelInfo.  Furthermore, we do not want to force use
	 * of PHVs in the AppendRelInfo --- there isn't any outer join between.
	 */
	if (containing_appendrel)
	{
		ReplaceWrapOption save_wrap_option = rvcontext->wrap_option;

		rvcontext->wrap_option = REPLACE_WRAP_NONE;
		containing_appendrel->translated_vars = (List *)
			pullup_replace_vars((Node *) containing_appendrel->translated_vars,
								rvcontext);
		rvcontext->wrap_option = save_wrap_option;
		return;
	}

	/*
	 * Replace all of the top query's references to the subquery's outputs
	 * with copies of the adjusted subtlist items, being careful not to
	 * replace any of the jointree structure.  (This'd be a lot cleaner if we
	 * could use query_tree_mutator.)  We have to use PHVs in the targetList,
	 * returningList, and havingQual, since those are certainly above any
	 * outer join.  replace_vars_in_jointree tracks its location in the
	 * jointree and uses PHVs or not appropriately.
	 */
	parse->targetList = (List *)
		pullup_replace_vars((Node *) parse->targetList, rvcontext);
	parse->returningList = (List *)
		pullup_replace_vars((Node *) parse->returningList, rvcontext);

	if (parse->onConflict)
	{
		parse->onConflict->onConflictSet = (List *)
			pullup_replace_vars((Node *) parse->onConflict->onConflictSet,
								rvcontext);
		parse->onConflict->onConflictWhere =
			pullup_replace_vars(parse->onConflict->onConflictWhere,
								rvcontext);

		/*
		 * We assume ON CONFLICT's arbiterElems, arbiterWhere, exclRelTlist
		 * can't contain any references to a subquery.
		 */
	}
	if (parse->mergeActionList)
	{
		foreach(lc, parse->mergeActionList)
		{
			MergeAction *action = lfirst(lc);

			action->qual = pullup_replace_vars(action->qual, rvcontext);
			action->targetList = (List *)
				pullup_replace_vars((Node *) action->targetList, rvcontext);
		}
	}
	parse->mergeJoinCondition = pullup_replace_vars(parse->mergeJoinCondition,
													rvcontext);
	replace_vars_in_jointree((Node *) parse->jointree, rvcontext);
	Assert(parse->setOperations == NULL);
	parse->havingQual = pullup_replace_vars(parse->havingQual, rvcontext);

	/*
	 * Replace references in the translated_vars lists of appendrels.
	 */
	foreach(lc, root->append_rel_list)
	{
		AppendRelInfo *appinfo = (AppendRelInfo *) lfirst(lc);

		appinfo->translated_vars = (List *)
			pullup_replace_vars((Node *) appinfo->translated_vars, rvcontext);
	}

	/*
	 * Replace references in the joinaliasvars lists of join RTEs and the
	 * groupexprs list of group RTE.
	 */
	foreach(lc, parse->rtable)
	{
		RangeTblEntry *otherrte = (RangeTblEntry *) lfirst(lc);

		if (otherrte->rtekind == RTE_JOIN)
			otherrte->joinaliasvars = (List *)
				pullup_replace_vars((Node *) otherrte->joinaliasvars,
									rvcontext);
		else if (otherrte->rtekind == RTE_GROUP)
			otherrte->groupexprs = (List *)
				pullup_replace_vars((Node *) otherrte->groupexprs,
									rvcontext);
	}
}
/* …（同文件无关代码省略）… */
static void
replace_vars_in_jointree(Node *jtnode,
						 pullup_replace_vars_context *context)
{
	if (jtnode == NULL)
		return;
	if (IsA(jtnode, RangeTblRef))
	{
		/*
		 * If the RangeTblRef refers to a LATERAL subquery (that isn't the
		 * same subquery we're pulling up), it might contain references to the
		 * target subquery, which we must replace.  We drive this from the
		 * jointree scan, rather than a scan of the rtable, so that we can
		 * avoid processing no-longer-referenced RTEs.
		 */
		int			varno = ((RangeTblRef *) jtnode)->rtindex;

		if (varno != context->varno)	/* ignore target subquery itself */
		{
			RangeTblEntry *rte = rt_fetch(varno, context->root->parse->rtable);

			Assert(rte != context->target_rte);
			if (rte->lateral)
			{
				switch (rte->rtekind)
				{
					case RTE_RELATION:
						/* shouldn't be marked LATERAL unless tablesample */
						Assert(rte->tablesample);
						rte->tablesample = (TableSampleClause *)
							pullup_replace_vars((Node *) rte->tablesample,
												context);
						break;
					case RTE_SUBQUERY:
						rte->subquery =
							pullup_replace_vars_subquery(rte->subquery,
														 context);
						break;
					case RTE_FUNCTION:
						rte->functions = (List *)
							pullup_replace_vars((Node *) rte->functions,
												context);
						break;
					case RTE_TABLEFUNC:
						rte->tablefunc = (TableFunc *)
							pullup_replace_vars((Node *) rte->tablefunc,
												context);
						break;
					case RTE_VALUES:
						rte->values_lists = (List *)
							pullup_replace_vars((Node *) rte->values_lists,
												context);
						break;
					case RTE_JOIN:
					case RTE_CTE:
					case RTE_NAMEDTUPLESTORE:
					case RTE_RESULT:
					case RTE_GROUP:
						/* these shouldn't be marked LATERAL */
						Assert(false);
						break;
				}
			}
		}
	}
	else if (IsA(jtnode, FromExpr))
	{
		FromExpr   *f = (FromExpr *) jtnode;
		ListCell   *l;

		foreach(l, f->fromlist)
			replace_vars_in_jointree(lfirst(l), context);
		f->quals = pullup_replace_vars(f->quals, context);
	}
	else if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j = (JoinExpr *) jtnode;
		ReplaceWrapOption save_wrap_option = context->wrap_option;

		replace_vars_in_jointree(j->larg, context);
		replace_vars_in_jointree(j->rarg, context);

		/*
		 * Use PHVs within the join quals of a full join for variable-free
		 * expressions.  Otherwise, we cannot identify which side of the join
		 * a pulled-up variable-free expression came from, which can lead to
		 * failure to make a plan at all because none of the quals appear to
		 * be mergeable or hashable conditions.
		 */
		if (j->jointype == JOIN_FULL)
			context->wrap_option = REPLACE_WRAP_VARFREE;

		j->quals = pullup_replace_vars(j->quals, context);

		context->wrap_option = save_wrap_option;
	}
	else
		elog(ERROR, "unrecognized node type: %d",
			 (int) nodeTag(jtnode));
}
/* …（同文件无关代码省略）… */
static Node *
pullup_replace_vars(Node *expr, pullup_replace_vars_context *context)
{
	return replace_rte_variables(expr,
								 context->varno, 0,
								 pullup_replace_vars_callback,
								 context,
								 context->outer_hasSubLinks);
}
/* …（同文件无关代码省略）… */
static Node *
pullup_replace_vars_callback(const Var *var,
							 replace_rte_variables_context *context)
{
	pullup_replace_vars_context *rcon = (pullup_replace_vars_context *) context->callback_arg;
	int			varattno = var->varattno;
	bool		need_phv;
	Node	   *newnode;

	/* System columns are not replaced. */
	if (varattno < InvalidAttrNumber)
		return (Node *) copyObject(var);

	/*
	 * We need a PlaceHolderVar if the Var-to-be-replaced has nonempty
	 * varnullingrels (unless we find below that the replacement expression is
	 * a Var or PlaceHolderVar that we can just add the nullingrels to).  We
	 * also need one if the caller has instructed us that certain expression
	 * replacements need to be wrapped for identification purposes.
	 */
	need_phv = (var->varnullingrels != NULL) ||
		(rcon->wrap_option != REPLACE_WRAP_NONE);

	/*
	 * If PlaceHolderVars are needed, we cache the modified expressions in
	 * rcon->rv_cache[].  This is not in hopes of any material speed gain
	 * within this function, but to avoid generating identical PHVs with
	 * different IDs.  That would result in duplicate evaluations at runtime,
	 * and possibly prevent optimizations that rely on recognizing different
	 * references to the same subquery output as being equal().  So it's worth
	 * a bit of extra effort to avoid it.
	 *
	 * The cached items have phlevelsup = 0 and phnullingrels = NULL; we'll
	 * copy them and adjust those values for this reference site below.
	 */
	if (need_phv &&
		varattno >= InvalidAttrNumber &&
		varattno <= list_length(rcon->targetlist) &&
		rcon->rv_cache[varattno] != NULL)
	{
		/* Just copy the entry and fall through to adjust phlevelsup etc */
		newnode = copyObject(rcon->rv_cache[varattno]);
	}
	else
	{
		/*
		 * Generate the replacement expression.  This takes care of expanding
		 * wholerow references and dealing with non-default varreturningtype.
		 */
		newnode = ReplaceVarFromTargetList(var,
										   rcon->target_rte,
										   rcon->targetlist,
										   rcon->result_relation,
										   REPLACEVARS_REPORT_ERROR,
										   0);

		/* Insert PlaceHolderVar if needed */
		if (need_phv)
		{
			bool		wrap;

			if (rcon->wrap_option == REPLACE_WRAP_ALL)
			{
				/* Caller told us to wrap all expressions in a PlaceHolderVar */
				wrap = true;
			}
			else if (varattno == InvalidAttrNumber)
			{
				/*
				 * Insert PlaceHolderVar for whole-tuple reference.  Notice
				 * that we are wrapping one PlaceHolderVar around the whole
				 * RowExpr, rather than putting one around each element of the
				 * row.  This is because we need the expression to yield NULL,
				 * not ROW(NULL,NULL,...) when it is forced to null by an
				 * outer join.
				 */
				wrap = true;
			}
			else if (newnode && IsA(newnode, Var) &&
					 ((Var *) newnode)->varlevelsup == 0)
			{
				/*
				 * Simple Vars always escape being wrapped, unless they are
				 * lateral references to something outside the subquery being
				 * pulled up and the referenced rel is not under the same
				 * lowest nulling outer join.
				 */
				wrap = false;
				if (rcon->target_rte->lateral &&
					!bms_is_member(((Var *) newnode)->varno, rcon->relids))
				{
					nullingrel_info *nullinfo = rcon->nullinfo;
					int			lvarno = ((Var *) newnode)->varno;

					Assert(lvarno > 0 && lvarno <= nullinfo->rtlength);
					if (!bms_is_subset(nullinfo->nullingrels[rcon->varno],
									   nullinfo->nullingrels[lvarno]))
						wrap = true;
				}
			}
			else if (newnode && IsA(newnode, PlaceHolderVar) &&
					 ((PlaceHolderVar *) newnode)->phlevelsup == 0)
			{
				/* The same rules apply for a PlaceHolderVar */
				wrap = false;
				if (rcon->target_rte->lateral &&
					!bms_is_subset(((PlaceHolderVar *) newnode)->phrels,
								   rcon->relids))
				{
					nullingrel_info *nullinfo = rcon->nullinfo;
					Relids		lvarnos = ((PlaceHolderVar *) newnode)->phrels;
					int			lvarno;

					lvarno = -1;
					while ((lvarno = bms_next_member(lvarnos, lvarno)) >= 0)
					{
						Assert(lvarno > 0 && lvarno <= nullinfo->rtlength);
						if (!bms_is_subset(nullinfo->nullingrels[rcon->varno],
										   nullinfo->nullingrels[lvarno]))
						{
							wrap = true;
							break;
						}
					}
				}
			}
			else
			{
				/*
				 * If the node contains Var(s) or PlaceHolderVar(s) of the
				 * subquery being pulled up, or of rels that are under the
				 * same lowest nulling outer join as the subquery, and does
				 * not contain any non-strict constructs, then instead of
				 * adding a PHV on top we can add the required nullingrels to
				 * those Vars/PHVs.  (This is fundamentally a generalization
				 * of the above cases for bare Vars and PHVs.)
				 *
				 * This test is somewhat expensive, but it avoids pessimizing
				 * the plan in cases where the nullingrels get removed again
				 * later by outer join reduction.
				 *
				 * Note that we don't force wrapping of expressions containing
				 * lateral references, so long as they also contain Vars/PHVs
				 * of the subquery, or of rels that are under the same lowest
				 * nulling outer join as the subquery.  This is okay because
				 * of the restriction to strict constructs: if those Vars/PHVs
				 * have been forced to NULL by an outer join then the end
				 * result of the expression will be NULL too, regardless of
				 * the lateral references.  So it's not necessary to force the
				 * expression to be evaluated below the outer join.  This can
				 * be a very valuable optimization, because it may allow us to
				 * avoid using a nested loop to pass the lateral reference
				 * down.
				 *
				 * This analysis could be tighter: in particular, a non-strict
				 * construct hidden within a lower-level PlaceHolderVar is not
				 * reason to add another PHV.  But for now it doesn't seem
				 * worth the code to be more exact.  This is also why it's
				 * preferable to handle bare PHVs in the above branch, rather
				 * than this branch.  We also prefer to handle bare Vars in a
				 * separate branch, as it's cheaper this way and parallels the
				 * handling of PHVs.
				 *
				 * For a LATERAL subquery, we have to check the actual var
				 * membership of the node, but if it's non-lateral then any
				 * level-zero var must belong to the subquery.
				 */
				bool		contain_nullable_vars = false;

				if (!rcon->target_rte->lateral)
				{
					if (contain_vars_of_level(newnode, 0))
						contain_nullable_vars = true;
				}
				else
				{
					Relids		all_varnos;

					all_varnos = pull_varnos(rcon->root, newnode);
					if (bms_overlap(all_varnos, rcon->relids))
						contain_nullable_vars = true;
					else
					{
						nullingrel_info *nullinfo = rcon->nullinfo;
						int			varno;

						varno = -1;
						while ((varno = bms_next_member(all_varnos, varno)) >= 0)
						{
							Assert(varno > 0 && varno <= nullinfo->rtlength);
							if (bms_is_subset(nullinfo->nullingrels[rcon->varno],
											  nullinfo->nullingrels[varno]))
							{
								contain_nullable_vars = true;
								break;
							}
						}
					}
				}

				if (contain_nullable_vars &&
					!contain_nonstrict_functions(newnode))
				{
					/* No wrap needed */
					wrap = false;
				}
				else
				{
					/* Else wrap it in a PlaceHolderVar */
					wrap = true;
				}
			}

			if (wrap)
			{
				newnode = (Node *)
					make_placeholder_expr(rcon->root,
										  (Expr *) newnode,
										  bms_make_singleton(rcon->varno));

				/*
				 * Cache it if possible (ie, if the attno is in range, which
				 * it probably always should be).
				 */
				if (varattno >= InvalidAttrNumber &&
					varattno <= list_length(rcon->targetlist))
					rcon->rv_cache[varattno] = copyObject(newnode);
			}
		}
	}

	/* Propagate any varnullingrels into the replacement expression */
	if (var->varnullingrels != NULL)
	{
		if (IsA(newnode, Var))
		{
			Var		   *newvar = (Var *) newnode;

			Assert(newvar->varlevelsup == 0);
			newvar->varnullingrels = bms_add_members(newvar->varnullingrels,
													 var->varnullingrels);
		}
		else if (IsA(newnode, PlaceHolderVar))
		{
			PlaceHolderVar *newphv = (PlaceHolderVar *) newnode;

			Assert(newphv->phlevelsup == 0);
			newphv->phnullingrels = bms_add_members(newphv->phnullingrels,
													var->varnullingrels);
		}
		else
		{
			/*
			 * There should be Vars/PHVs within the expression that we can
			 * modify.  Vars/PHVs of the subquery should have the full
			 * var->varnullingrels added to them, but if there are lateral
			 * references within the expression, those must be marked with
			 * only the nullingrels that potentially apply to them.  (This
			 * corresponds to the fact that the expression will now be
			 * evaluated at the join level of the Var that we are replacing:
			 * the lateral references may have bubbled up through fewer outer
			 * joins than the subquery's Vars have.  Per the discussion above,
			 * we'll still get the right answers.)  That relid set could be
			 * different for different lateral relations, so we have to do
			 * this work for each one.
			 *
			 * (Currently, the restrictions in is_simple_subquery() mean that
			 * at most we have to remove the lowest outer join's relid from
			 * the nullingrels of a lateral reference.  However, we might
			 * relax those restrictions someday, so let's do this right.)
			 */
			if (rcon->target_rte->lateral)
			{
				nullingrel_info *nullinfo = rcon->nullinfo;
				Relids		lvarnos;
				int			lvarno;

				/*
				 * Identify lateral varnos used within newnode.  We must do
				 * this before injecting var->varnullingrels into the tree.
				 */
				lvarnos = pull_varnos(rcon->root, newnode);
				lvarnos = bms_del_members(lvarnos, rcon->relids);
				/* For each one, add relevant nullingrels if any */
				lvarno = -1;
				while ((lvarno = bms_next_member(lvarnos, lvarno)) >= 0)
				{
					Relids		lnullingrels;

					Assert(lvarno > 0 && lvarno <= nullinfo->rtlength);
					lnullingrels = bms_intersect(var->varnullingrels,
												 nullinfo->nullingrels[lvarno]);
					if (!bms_is_empty(lnullingrels))
						newnode = add_nulling_relids(newnode,
													 bms_make_singleton(lvarno),
													 lnullingrels);
				}
			}

			/* Finally, deal with Vars/PHVs of the subquery itself */
			newnode = add_nulling_relids(newnode,
										 rcon->relids,
										 var->varnullingrels);
			/* Assert we did put the varnullingrels into the expression */
			Assert(bms_is_subset(var->varnullingrels,
								 pull_varnos(rcon->root, newnode)));
		}
	}

	/* Must adjust varlevelsup if replaced Var is within a subquery */
	if (var->varlevelsup > 0)
		IncrementVarSublevelsUp(newnode, var->varlevelsup, 0);

	return newnode;
}
/* …（同文件无关代码省略）… */
static Query *
pullup_replace_vars_subquery(Query *query,
							 pullup_replace_vars_context *context)
{
	Assert(IsA(query, Query));
	return (Query *) replace_rte_variables((Node *) query,
										   context->varno, 1,
										   pullup_replace_vars_callback,
										   context,
										   NULL);
}
/* …（同文件无关代码省略）… */
typedef struct
{
	int			varno;
	int			sublevels_up;
	Relids		subrelids;
} substitute_phv_relids_context;
/* …（同文件无关代码省略）… */
static bool
substitute_phv_relids_walker(Node *node,
							 substitute_phv_relids_context *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, PlaceHolderVar))
	{
		PlaceHolderVar *phv = (PlaceHolderVar *) node;

		if (phv->phlevelsup == context->sublevels_up &&
			bms_is_member(context->varno, phv->phrels))
		{
			phv->phrels = bms_union(phv->phrels,
									context->subrelids);
			phv->phrels = bms_del_member(phv->phrels,
										 context->varno);
			/* Assert we haven't broken the PHV */
			Assert(!bms_is_empty(phv->phrels));
		}
		/* fall through to examine children */
	}
	if (IsA(node, Query))
	{
		/* Recurse into subselects */
		bool		result;

		context->sublevels_up++;
		result = query_tree_walker((Query *) node,
								   substitute_phv_relids_walker,
								   context, 0);
		context->sublevels_up--;
		return result;
	}
	/* Shouldn't need to handle planner auxiliary nodes here */
	Assert(!IsA(node, SpecialJoinInfo));
	Assert(!IsA(node, AppendRelInfo));
	Assert(!IsA(node, PlaceHolderInfo));
	Assert(!IsA(node, MinMaxAggInfo));

	return expression_tree_walker(node, substitute_phv_relids_walker, context);
}
/* …（同文件无关代码省略）… */
static void
substitute_phv_relids(Node *node, int varno, Relids subrelids)
{
	substitute_phv_relids_context context;

	context.varno = varno;
	context.sublevels_up = 0;
	context.subrelids = subrelids;

	/*
	 * Must be prepared to start with a Query or a bare expression tree.
	 */
	query_or_expression_tree_walker(node,
									substitute_phv_relids_walker,
									&context,
									0);
}
/* …（同文件无关代码省略）… */
static void
fix_append_rel_relids(PlannerInfo *root, int varno, Relids subrelids)
{
	ListCell   *l;
	int			subvarno = -1;

	/*
	 * We only want to extract the member relid once, but we mustn't fail
	 * immediately if there are multiple members; it could be that none of the
	 * AppendRelInfo nodes refer to it.  So compute it on first use. Note that
	 * bms_singleton_member will complain if set is not singleton.
	 */
	foreach(l, root->append_rel_list)
	{
		AppendRelInfo *appinfo = (AppendRelInfo *) lfirst(l);

		/* The parent_relid shouldn't ever be a pullup target */
		Assert(appinfo->parent_relid != varno);

		if (appinfo->child_relid == varno)
		{
			if (subvarno < 0)
				subvarno = bms_singleton_member(subrelids);
			appinfo->child_relid = subvarno;
		}

		/* Also fix up any PHVs in its translated vars */
		if (root->glob->lastPHId != 0)
			substitute_phv_relids((Node *) appinfo->translated_vars,
								  varno, subrelids);
	}
}
/* …（同文件无关代码省略）… */
Relids
get_relids_in_jointree(Node *jtnode, bool include_outer_joins,
					   bool include_inner_joins)
{
	Relids		result = NULL;

	if (jtnode == NULL)
		return result;
	if (IsA(jtnode, RangeTblRef))
	{
		int			varno = ((RangeTblRef *) jtnode)->rtindex;

		result = bms_make_singleton(varno);
	}
	else if (IsA(jtnode, FromExpr))
	{
		FromExpr   *f = (FromExpr *) jtnode;
		ListCell   *l;

		foreach(l, f->fromlist)
		{
			result = bms_join(result,
							  get_relids_in_jointree(lfirst(l),
													 include_outer_joins,
													 include_inner_joins));
		}
	}
	else if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j = (JoinExpr *) jtnode;

		result = get_relids_in_jointree(j->larg,
										include_outer_joins,
										include_inner_joins);
		result = bms_join(result,
						  get_relids_in_jointree(j->rarg,
												 include_outer_joins,
												 include_inner_joins));
		if (j->rtindex)
		{
			if (j->jointype == JOIN_INNER)
			{
				if (include_inner_joins)
					result = bms_add_member(result, j->rtindex);
			}
			else
			{
				if (include_outer_joins)
					result = bms_add_member(result, j->rtindex);
			}
		}
	}
	else
		elog(ERROR, "unrecognized node type: %d",
			 (int) nodeTag(jtnode));
	return result;
}
/* …（同文件无关代码省略）… */
static nullingrel_info *
get_nullingrels(Query *parse)
{
	nullingrel_info *result = palloc_object(nullingrel_info);

	result->rtlength = list_length(parse->rtable);
	result->nullingrels = palloc0_array(Relids, result->rtlength + 1);
	get_nullingrels_recurse((Node *) parse->jointree, NULL, result);
	return result;
}
/* …（同文件无关代码省略）… */
static void
get_nullingrels_recurse(Node *jtnode, Relids upper_nullingrels,
						nullingrel_info *info)
{
	if (jtnode == NULL)
		return;
	if (IsA(jtnode, RangeTblRef))
	{
		int			varno = ((RangeTblRef *) jtnode)->rtindex;

		Assert(varno > 0 && varno <= info->rtlength);
		info->nullingrels[varno] = upper_nullingrels;
	}
	else if (IsA(jtnode, FromExpr))
	{
		FromExpr   *f = (FromExpr *) jtnode;
		ListCell   *l;

		foreach(l, f->fromlist)
		{
			get_nullingrels_recurse(lfirst(l), upper_nullingrels, info);
		}
	}
	else if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j = (JoinExpr *) jtnode;
		Relids		local_nullingrels;

		switch (j->jointype)
		{
			case JOIN_INNER:
				get_nullingrels_recurse(j->larg, upper_nullingrels, info);
				get_nullingrels_recurse(j->rarg, upper_nullingrels, info);
				break;
			case JOIN_LEFT:
			case JOIN_SEMI:
			case JOIN_ANTI:
				local_nullingrels = bms_add_member(bms_copy(upper_nullingrels),
												   j->rtindex);
				get_nullingrels_recurse(j->larg, upper_nullingrels, info);
				get_nullingrels_recurse(j->rarg, local_nullingrels, info);
				break;
			case JOIN_FULL:
				local_nullingrels = bms_add_member(bms_copy(upper_nullingrels),
												   j->rtindex);
				get_nullingrels_recurse(j->larg, local_nullingrels, info);
				get_nullingrels_recurse(j->rarg, local_nullingrels, info);
				break;
			case JOIN_RIGHT:
				local_nullingrels = bms_add_member(bms_copy(upper_nullingrels),
												   j->rtindex);
				get_nullingrels_recurse(j->larg, local_nullingrels, info);
				get_nullingrels_recurse(j->rarg, upper_nullingrels, info);
				break;
			default:
				elog(ERROR, "unrecognized join type: %d",
					 (int) j->jointype);
				break;
		}
	}
	else
		elog(ERROR, "unrecognized node type: %d",
			 (int) nodeTag(jtnode));
}
