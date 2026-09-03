// AUTO-DRAFT from postgres/postgres PR #ad4b80b6f49bcac519175fcb632aa94cdb7f6948
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
typedef struct JoinTreeItem
{
	/* Fields filled during deconstruct_recurse: */
	Node	   *jtnode;			/* jointree node to examine */
	JoinDomain *jdomain;		/* join domain for its ON/WHERE clauses */
	struct JoinTreeItem *jti_parent;	/* JoinTreeItem for this node's
										 * parent, or NULL if it's the top */
	Relids		qualscope;		/* base+OJ Relids syntactically included in
								 * this jointree node */
	Relids		inner_join_rels;	/* base+OJ Relids syntactically included
									 * in inner joins appearing at or below
									 * this jointree node */
	Relids		left_rels;		/* if join node, Relids of the left side */
	Relids		right_rels;		/* if join node, Relids of the right side */
	Relids		nonnullable_rels;	/* if outer join, Relids of the
									 * non-nullable side */
	/* Fields filled during deconstruct_distribute: */
	SpecialJoinInfo *sjinfo;	/* if outer join, its SpecialJoinInfo */
	List	   *oj_joinclauses; /* outer join quals not yet distributed */
	List	   *lateral_clauses;	/* quals postponed from children due to
									 * lateral references */
} JoinTreeItem;
/* …（同文件无关代码省略）… */
									bool has_clone,
									bool is_clone,
									List **postponed_oj_qual_list);
static bool check_redundant_nullability_qual(PlannerInfo *root, Node *clause);
static Relids get_join_domain_min_rels(PlannerInfo *root, Relids domain_relids);
static void check_mergejoinable(RestrictInfo *restrictinfo);
static void check_hashjoinable(RestrictInfo *restrictinfo);
/* …（同文件无关代码省略）… */
void
add_vars_to_targetlist(PlannerInfo *root, List *vars,
					   Relids where_needed)
{
	ListCell   *temp;

	Assert(!bms_is_empty(where_needed));

	foreach(temp, vars)
	{
		Node	   *node = (Node *) lfirst(temp);

		if (IsA(node, Var))
		{
			Var		   *var = (Var *) node;
			RelOptInfo *rel = find_base_rel(root, var->varno);
			int			attno = var->varattno;

			if (bms_is_subset(where_needed, rel->relids))
				continue;
			Assert(attno >= rel->min_attr && attno <= rel->max_attr);
			attno -= rel->min_attr;
			if (rel->attr_needed[attno] == NULL)
			{
				/*
				 * Variable not yet requested, so add to rel's targetlist.
				 *
				 * The value available at the rel's scan level has not been
				 * nulled by any outer join, so drop its varnullingrels.
				 * (We'll put those back as we climb up the join tree.)
				 */
				var = copyObject(var);
				var->varnullingrels = NULL;
				rel->reltarget->exprs = lappend(rel->reltarget->exprs, var);
				/* reltarget cost and width will be computed later */
			}
			rel->attr_needed[attno] = bms_add_members(rel->attr_needed[attno],
													  where_needed);
		}
		else if (IsA(node, PlaceHolderVar))
		{
			PlaceHolderVar *phv = (PlaceHolderVar *) node;
			PlaceHolderInfo *phinfo = find_placeholder_info(root, phv);

			phinfo->ph_needed = bms_add_members(phinfo->ph_needed,
												where_needed);
		}
		else
			elog(ERROR, "unrecognized node type: %d", (int) nodeTag(node));
	}
}
/* …（同文件无关代码省略）… */
 * 'qualscope' identifies what level of JOIN the qual came from syntactically.
 * 'ojscope' is needed if we decide to force the qual up to the outer-join
 * level, which will be ojscope not necessarily qualscope.
 *
 * At the time this is called, root->join_info_list must contain entries for
 * at least those special joins that are syntactically below this qual.
 * (We now need that only for detection of redundant IS NULL quals.)
 */
static void
distribute_qual_to_rels(PlannerInfo *root, Node *clause,
						JoinTreeItem *jtitem,
						SpecialJoinInfo *sjinfo,
						Index security_level,
						Relids qualscope,
						Relids ojscope,
						Relids outerjoin_nonnullable,
						Relids incompatible_relids,
						bool allow_equivalence,
						bool has_clone,
						bool is_clone,
						List **postponed_oj_qual_list)
{
	Relids		relids;
	bool		is_pushed_down;
	bool		pseudoconstant = false;
	bool		maybe_equivalence;
	bool		maybe_outer_join;
	RestrictInfo *restrictinfo;

	/*
	 * Retrieve all relids mentioned within the clause.
	 */
	relids = pull_varnos(root, clause);

	/*
	 * In ordinary SQL, a WHERE or JOIN/ON clause can't reference any rels
	 * that aren't within its syntactic scope; however, if we pulled up a
	 * LATERAL subquery then we might find such references in quals that have
	 * been pulled up.  We need to treat such quals as belonging to the join
	 * level that includes every rel they reference.  Although we could make
	 * pull_up_subqueries() place such quals correctly to begin with, it's
	 * easier to handle it here.  When we find a clause that contains Vars
	 * outside its syntactic scope, locate the nearest parent join level that
	 * includes all the required rels and add the clause to that level's
	 * lateral_clauses list.  We'll process it when we reach that join level.
	 */
	if (!bms_is_subset(relids, qualscope))
	{
		JoinTreeItem *pitem;

		Assert(root->hasLateralRTEs);	/* shouldn't happen otherwise */
		Assert(sjinfo == NULL); /* mustn't postpone past outer join */
		for (pitem = jtitem->jti_parent; pitem; pitem = pitem->jti_parent)
		{
			if (bms_is_subset(relids, pitem->qualscope))
			{
				pitem->lateral_clauses = lappend(pitem->lateral_clauses,
												 clause);
				return;
			}

			/*
			 * We should not be postponing any quals past an outer join.  If
			 * this Assert fires, pull_up_subqueries() messed up.
			 */
			Assert(pitem->sjinfo == NULL);
		}
		elog(ERROR, "failed to postpone qual containing lateral reference");
	}

	/*
	 * If it's an outer-join clause, also check that relids is a subset of
	 * ojscope.  (This should not fail if the syntactic scope check passed.)
	 */
	if (ojscope && !bms_is_subset(relids, ojscope))
		elog(ERROR, "JOIN qualification cannot refer to other relations");

	/*
	 * If the clause is variable-free, our normal heuristic for pushing it
	 * down to just the mentioned rels doesn't work, because there are none.
	 *
	 * If the clause is an outer-join clause, we must force it to the OJ's
	 * semantic level to preserve semantics.
	 *
	 * Otherwise, when the clause contains volatile functions, we force it to
	 * be evaluated at its original syntactic level.  This preserves the
	 * expected semantics.
	 *
	 * When the clause contains no volatile functions either, it is actually a
	 * pseudoconstant clause that will not change value during any one
	 * execution of the plan, and hence can be used as a one-time qual in a
	 * gating Result plan node.  We put such a clause into the regular
	 * RestrictInfo lists for the moment, but eventually createplan.c will
	 * pull it out and make a gating Result node immediately above whatever
	 * plan node the pseudoconstant clause is assigned to.  It's usually best
	 * to put a gating node as high in the plan tree as possible.
	 */
	if (bms_is_empty(relids))
	{
		if (ojscope)
		{
			/* clause is attached to outer join, eval it there */
			relids = bms_copy(ojscope);
			/* mustn't use as gating qual, so don't mark pseudoconstant */
		}
		else if (contain_volatile_functions(clause))
		{
			/* eval at original syntactic level */
			relids = bms_copy(qualscope);
			/* again, can't mark pseudoconstant */
		}
		else
		{
			/*
			 * If we are in the top-level join domain, we can push the qual to
			 * the top of the plan tree.  Otherwise, be conservative and eval
			 * it at original syntactic level.  (Ideally we'd push it to the
			 * top of the current join domain in all cases, but that causes
			 * problems if we later rearrange outer-join evaluation order.
			 * Pseudoconstant quals below the top level are a pretty odd case,
			 * so it's not clear that it's worth working hard on.)
			 */
			if (jtitem->jdomain == (JoinDomain *) linitial(root->join_domains))
				relids = bms_copy(jtitem->jdomain->jd_relids);
			else
				relids = bms_copy(qualscope);
			/* mark as gating qual */
			pseudoconstant = true;
			/* tell createplan.c to check for gating quals */
			root->hasPseudoConstantQuals = true;
		}
	}

	/*----------
	 * Check to see if clause application must be delayed by outer-join
	 * considerations.
	 *
	 * A word about is_pushed_down: we mark the qual as "pushed down" if
	 * it is (potentially) applicable at a level different from its original
	 * syntactic level.  This flag is used to distinguish OUTER JOIN ON quals
	 * from other quals pushed down to the same joinrel.  The rules are:
	 *		WHERE quals and INNER JOIN quals: is_pushed_down = true.
	 *		Non-degenerate OUTER JOIN quals: is_pushed_down = false.
	 *		Degenerate OUTER JOIN quals: is_pushed_down = true.
	 * A "degenerate" OUTER JOIN qual is one that doesn't mention the
	 * non-nullable side, and hence can be pushed down into the nullable side
	 * without changing the join result.  It is correct to treat it as a
	 * regular filter condition at the level where it is evaluated.
	 *
	 * Note: it is not immediately obvious that a simple boolean is enough
	 * for this: if for some reason we were to attach a degenerate qual to
	 * its original join level, it would need to be treated as an outer join
	 * qual there.  However, this cannot happen, because all the rels the
	 * clause mentions must be in the outer join's min_righthand, therefore
	 * the join it needs must be formed before the outer join; and we always
	 * attach quals to the lowest level where they can be evaluated.  But
	 * if we were ever to re-introduce a mechanism for delaying evaluation
	 * of "expensive" quals, this area would need work.
	 *
	 * Note: generally, use of is_pushed_down has to go through the macro
	 * RINFO_IS_PUSHED_DOWN, because that flag alone is not always sufficient
	 * to tell whether a clause must be treated as pushed-down in context.
	 * This seems like another reason why it should perhaps be rethought.
	 *----------
	 */
	if (bms_overlap(relids, outerjoin_nonnullable))
	{
		/*
		 * The qual is attached to an outer join and mentions (some of the)
		 * rels on the nonnullable side, so it's not degenerate.  If the
		 * caller wants to postpone handling such clauses, just add it to
		 * postponed_oj_qual_list and return.  (The work we've done up to here
		 * will have to be redone later, but there's not much of it.)
		 */
		if (postponed_oj_qual_list != NULL)
		{
			*postponed_oj_qual_list = lappend(*postponed_oj_qual_list, clause);
			return;
		}

		/*
		 * We can't use such a clause to deduce equivalence (the left and
		 * right sides might be unequal above the join because one of them has
		 * gone to NULL) ... but we might be able to use it for more limited
		 * deductions, if it is mergejoinable.  So consider adding it to the
		 * lists of set-aside outer-join clauses.
		 */
		is_pushed_down = false;
		maybe_equivalence = false;
		maybe_outer_join = true;

		/*
		 * Now force the qual to be evaluated exactly at the level of joining
		 * corresponding to the outer join.  We cannot let it get pushed down
		 * into the nonnullable side, since then we'd produce no output rows,
		 * rather than the intended single null-extended row, for any
		 * nonnullable-side rows failing the qual.
		 */
		Assert(ojscope);
		relids = ojscope;
		Assert(!pseudoconstant);
	}
	else
	{
		/*
		 * Normal qual clause or degenerate outer-join clause.  Either way, we
		 * can mark it as pushed-down.
		 */
		is_pushed_down = true;

		/*
		 * It's possible that this is an IS NULL clause that's redundant with
		 * a lower antijoin; if so we can just discard it.  We need not test
		 * in any of the other cases, because this will only be possible for
		 * pushed-down clauses.
		 */
		if (check_redundant_nullability_qual(root, clause))
			return;

		/* Feed qual to the equivalence machinery, if allowed by caller */
		maybe_equivalence = allow_equivalence;

		/*
		 * Since it doesn't mention the LHS, it's certainly not useful as a
		 * set-aside OJ clause, even if it's in an OJ.
		 */
		maybe_outer_join = false;
	}

	/*
	 * Build the RestrictInfo node itself.
	 */
	restrictinfo = make_restrictinfo(root,
									 (Expr *) clause,
									 is_pushed_down,
									 has_clone,
									 is_clone,
									 pseudoconstant,
									 security_level,
									 relids,
									 incompatible_relids,
									 outerjoin_nonnullable);

	/*
	 * If it's a join clause, add vars used in the clause to targetlists of
	 * their relations, so that they will be emitted by the plan nodes that
	 * scan those relations (else they won't be available at the join node!).
	 *
	 * Normally we mark the vars as needed at the join identified by "relids".
	 * However, if this is a clone clause then ignore the outer-join relids in
	 * that set.  Otherwise, vars appearing in a cloned clause would end up
	 * marked as having to propagate to the highest one of the commuting
	 * joins, which would often be an overestimate.  For such clauses, correct
	 * var propagation is ensured by making ojscope include input rels from
	 * both sides of the join.
	 *
	 * Note: if the clause gets absorbed into an EquivalenceClass then this
	 * may be unnecessary, but for now we have to do it to cover the case
	 * where the EC becomes ec_broken and we end up reinserting the original
	 * clauses into the plan.
	 */
	if (bms_membership(relids) == BMS_MULTIPLE)
	{
		List	   *vars = pull_var_clause(clause,
										   PVC_RECURSE_AGGREGATES |
										   PVC_RECURSE_WINDOWFUNCS |
										   PVC_INCLUDE_PLACEHOLDERS);
		Relids		where_needed;

		if (is_clone)
			where_needed = bms_intersect(relids, root->all_baserels);
		else
			where_needed = relids;
		add_vars_to_targetlist(root, vars, where_needed);
		list_free(vars);
	}

	/*
	 * We check "mergejoinability" of every clause, not only join clauses,
	 * because we want to know about equivalences between vars of the same
	 * relation, or between vars and consts.
	 */
	check_mergejoinable(restrictinfo);

	/*
	 * If it is a true equivalence clause, send it to the EquivalenceClass
	 * machinery.  We do *not* attach it directly to any restriction or join
	 * lists.  The EC code will propagate it to the appropriate places later.
	 *
	 * If the clause has a mergejoinable operator, yet isn't an equivalence
	 * because it is an outer-join clause, the EC code may still be able to do
	 * something with it.  We add it to appropriate lists for further
	 * consideration later.  Specifically:
	 *
	 * If it is a left or right outer-join qualification that relates the two
	 * sides of the outer join (no funny business like leftvar1 = leftvar2 +
	 * rightvar), we add it to root->left_join_clauses or
	 * root->right_join_clauses according to which side the nonnullable
	 * variable appears on.
	 *
	 * If it is a full outer-join qualification, we add it to
	 * root->full_join_clauses.  (Ideally we'd discard cases that aren't
	 * leftvar = rightvar, as we do for left/right joins, but this routine
	 * doesn't have the info needed to do that; and the current usage of the
	 * full_join_clauses list doesn't require that, so it's not currently
	 * worth complicating this routine's API to make it possible.)
	 *
	 * If none of the above hold, pass it off to
	 * distribute_restrictinfo_to_rels().
	 *
	 * In all cases, it's important to initialize the left_ec and right_ec
	 * fields of a mergejoinable clause, so that all possibly mergejoinable
	 * expressions have representations in EquivalenceClasses.  If
	 * process_equivalence is successful, it will take care of that;
	 * otherwise, we have to call initialize_mergeclause_eclasses to do it.
	 */
	if (restrictinfo->mergeopfamilies)
	{
		if (maybe_equivalence)
		{
			if (process_equivalence(root, &restrictinfo, jtitem->jdomain))
				return;
			/* EC rejected it, so set left_ec/right_ec the hard way ... */
			if (restrictinfo->mergeopfamilies)	/* EC might have changed this */
				initialize_mergeclause_eclasses(root, restrictinfo);
			/* ... and fall through to distribute_restrictinfo_to_rels */
		}
		else if (maybe_outer_join && restrictinfo->can_join)
		{
			/* we need to set up left_ec/right_ec the hard way */
			initialize_mergeclause_eclasses(root, restrictinfo);
			/* now see if it should go to any outer-join lists */
			Assert(sjinfo != NULL);
			if (bms_is_subset(restrictinfo->left_relids,
							  outerjoin_nonnullable) &&
				!bms_overlap(restrictinfo->right_relids,
							 outerjoin_nonnullable))
			{
				/* we have outervar = innervar */
				OuterJoinClauseInfo *ojcinfo = makeNode(OuterJoinClauseInfo);

				ojcinfo->rinfo = restrictinfo;
				ojcinfo->sjinfo = sjinfo;
				root->left_join_clauses = lappend(root->left_join_clauses,
												  ojcinfo);
				return;
			}
			if (bms_is_subset(restrictinfo->right_relids,
							  outerjoin_nonnullable) &&
				!bms_overlap(restrictinfo->left_relids,
							 outerjoin_nonnullable))
			{
				/* we have innervar = outervar */
				OuterJoinClauseInfo *ojcinfo = makeNode(OuterJoinClauseInfo);

				ojcinfo->rinfo = restrictinfo;
				ojcinfo->sjinfo = sjinfo;
				root->right_join_clauses = lappend(root->right_join_clauses,
												   ojcinfo);
				return;
			}
			if (sjinfo->jointype == JOIN_FULL)
			{
				/* FULL JOIN (above tests cannot match in this case) */
				OuterJoinClauseInfo *ojcinfo = makeNode(OuterJoinClauseInfo);

				ojcinfo->rinfo = restrictinfo;
				ojcinfo->sjinfo = sjinfo;
				root->full_join_clauses = lappend(root->full_join_clauses,
												  ojcinfo);
				return;
			}
			/* nope, so fall through to distribute_restrictinfo_to_rels */
		}
		else
		{
			/* we still need to set up left_ec/right_ec */
			initialize_mergeclause_eclasses(root, restrictinfo);
		}
	}

	/* No EC special case applies, so push it into the clause lists */
	distribute_restrictinfo_to_rels(root, restrictinfo);
}

/*
 * check_redundant_nullability_qual
 *	  Check to see if the qual is an IS NULL qual that is redundant with
 *	  a lower JOIN_ANTI join.
 *
 * We want to suppress redundant IS NULL quals, not so much to save cycles
 * as to avoid generating bogus selectivity estimates for them.  So if
 * redundancy is detected here, distribute_qual_to_rels() just throws away
 * the qual.
 */
static bool
check_redundant_nullability_qual(PlannerInfo *root, Node *clause)
{
	Var		   *forced_null_var;
	ListCell   *lc;

	/* Check for IS NULL, and identify the Var forced to NULL */
	forced_null_var = find_forced_null_var(clause);
	if (forced_null_var == NULL)
		return false;

	/*
	 * If the Var comes from the nullable side of a lower antijoin, the IS
	 * NULL condition is necessarily true.  If it's not nulled by anything,
	 * there is no point in searching the join_info_list.  Otherwise, we need
	 * to find out whether the nulling rel is an antijoin.
	 */
	if (forced_null_var->varnullingrels == NULL)
		return false;

	foreach(lc, root->join_info_list)
	{
		SpecialJoinInfo *sjinfo = (SpecialJoinInfo *) lfirst(lc);

		/*
		 * This test will not succeed if sjinfo->ojrelid is zero, which is
		 * possible for an antijoin that was converted from a semijoin; but in
		 * such a case the Var couldn't have come from its nullable side.
		 */
		if (sjinfo->jointype == JOIN_ANTI && sjinfo->ojrelid != 0 &&
			bms_is_member(sjinfo->ojrelid, forced_null_var->varnullingrels))
			return true;
	}

	return false;
}

/*
 * add_base_clause_to_rel
 *		Add 'restrictinfo' as a baserestrictinfo to the base relation denoted
/* …（同文件无关代码省略）… */
static void
add_base_clause_to_rel(PlannerInfo *root, Index relid,
					   RestrictInfo *restrictinfo)
{
	RelOptInfo *rel = find_base_rel(root, relid);
	RangeTblEntry *rte = root->simple_rte_array[relid];

	Assert(bms_membership(restrictinfo->required_relids) == BMS_SINGLETON);

	/*
	 * For inheritance parent tables, we must always record the RestrictInfo
	 * in baserestrictinfo as is.  If we were to transform or skip adding it,
	 * then the original wouldn't be available in apply_child_basequals. Since
	 * there are two RangeTblEntries for inheritance parents, one with
	 * inh==true and the other with inh==false, we're still able to apply this
	 * optimization to the inh==false one.  The inh==true one is what
	 * apply_child_basequals() sees, whereas the inh==false one is what's used
	 * for the scan node in the final plan.
	 *
	 * We make an exception to this for partitioned tables.  For these, we
	 * always apply the constant-TRUE and constant-FALSE transformations.  A
	 * qual which is either of these for a partitioned table must also be that
	 * for all of its child partitions.
	 */
	if (!rte->inh || rte->relkind == RELKIND_PARTITIONED_TABLE)
	{
		/* Don't add the clause if it is always true */
		if (restriction_is_always_true(root, restrictinfo))
			return;

		/*
		 * Substitute the origin qual with constant-FALSE if it is provably
		 * always false.
		 *
		 * Note that we need to keep the same rinfo_serial, since it is in
		 * practice the same condition.  We also need to reset the
		 * last_rinfo_serial counter, which is essential to ensure that the
		 * RestrictInfos for the "same" qual condition get identical serial
		 * numbers (see deconstruct_distribute_oj_quals).
		 */
		if (restriction_is_always_false(root, restrictinfo))
		{
			int			save_rinfo_serial = restrictinfo->rinfo_serial;
			int			save_last_rinfo_serial = root->last_rinfo_serial;

			restrictinfo = make_restrictinfo(root,
											 (Expr *) makeBoolConst(false, false),
											 restrictinfo->is_pushed_down,
											 restrictinfo->has_clone,
											 restrictinfo->is_clone,
											 restrictinfo->pseudoconstant,
											 0, /* security_level */
											 restrictinfo->required_relids,
											 restrictinfo->incompatible_relids,
											 restrictinfo->outer_relids);
			restrictinfo->rinfo_serial = save_rinfo_serial;
			root->last_rinfo_serial = save_last_rinfo_serial;
		}
	}

	/* Add clause to rel's restriction list */
	rel->baserestrictinfo = lappend(rel->baserestrictinfo, restrictinfo);

	/* Update security level info */
	rel->baserestrict_min_security = Min(rel->baserestrict_min_security,
										 restrictinfo->security_level);
}
/* …（同文件无关代码省略）… */
bool
restriction_is_always_true(PlannerInfo *root,
						   RestrictInfo *restrictinfo)
{
	/*
	 * For a clone clause, we don't have a reliable way to determine if the
	 * input expression of a NullTest is non-nullable: nullingrel bits in
	 * clone clauses may not reflect reality, so we dare not draw conclusions
	 * from clones about whether Vars are guaranteed not-null.
	 */
	if (restrictinfo->has_clone || restrictinfo->is_clone)
		return false;

	/* Check for NullTest qual */
	if (IsA(restrictinfo->clause, NullTest))
	{
		NullTest   *nulltest = (NullTest *) restrictinfo->clause;

		/* is this NullTest an IS_NOT_NULL qual? */
		if (nulltest->nulltesttype != IS_NOT_NULL)
			return false;

		/*
		 * Empty rows can appear NULL in some contexts and NOT NULL in others,
		 * so avoid this optimization for row expressions.
		 */
		if (nulltest->argisrow)
			return false;

		return expr_is_nonnullable(root, nulltest->arg, NOTNULL_SOURCE_RELOPT);
	}

	/* If it's an OR, check its sub-clauses */
	if (restriction_is_or_clause(restrictinfo))
	{
		ListCell   *lc;

		Assert(is_orclause(restrictinfo->orclause));

		/*
		 * if any of the given OR branches is provably always true then the
		 * entire condition is true.
		 */
		foreach(lc, ((BoolExpr *) restrictinfo->orclause)->args)
		{
			Node	   *orarg = (Node *) lfirst(lc);

			if (!IsA(orarg, RestrictInfo))
				continue;

			if (restriction_is_always_true(root, (RestrictInfo *) orarg))
				return true;
		}
	}

	return false;
}
/* …（同文件无关代码省略）… */
bool
restriction_is_always_false(PlannerInfo *root,
							RestrictInfo *restrictinfo)
{
	/*
	 * For a clone clause, we don't have a reliable way to determine if the
	 * input expression of a NullTest is non-nullable: nullingrel bits in
	 * clone clauses may not reflect reality, so we dare not draw conclusions
	 * from clones about whether Vars are guaranteed not-null.
	 */
	if (restrictinfo->has_clone || restrictinfo->is_clone)
		return false;

	/* Check for NullTest qual */
	if (IsA(restrictinfo->clause, NullTest))
	{
		NullTest   *nulltest = (NullTest *) restrictinfo->clause;

		/* is this NullTest an IS_NULL qual? */
		if (nulltest->nulltesttype != IS_NULL)
			return false;

		/*
		 * Empty rows can appear NULL in some contexts and NOT NULL in others,
		 * so avoid this optimization for row expressions.
		 */
		if (nulltest->argisrow)
			return false;

		return expr_is_nonnullable(root, nulltest->arg, NOTNULL_SOURCE_RELOPT);
	}

	/* If it's an OR, check its sub-clauses */
	if (restriction_is_or_clause(restrictinfo))
	{
		ListCell   *lc;

		Assert(is_orclause(restrictinfo->orclause));

		/*
		 * Currently, when processing OR expressions, we only return true when
		 * all of the OR branches are always false.  This could perhaps be
		 * expanded to remove OR branches that are provably false.  This may
		 * be a useful thing to do as it could result in the OR being left
		 * with a single arg.  That's useful as it would allow the OR
		 * condition to be replaced with its single argument which may allow
		 * use of an index for faster filtering on the remaining condition.
		 */
		foreach(lc, ((BoolExpr *) restrictinfo->orclause)->args)
		{
			Node	   *orarg = (Node *) lfirst(lc);

			if (!IsA(orarg, RestrictInfo) ||
				!restriction_is_always_false(root, (RestrictInfo *) orarg))
				return false;
		}
		return true;
	}

	return false;
}
/* …（同文件无关代码省略）… */
void
distribute_restrictinfo_to_rels(PlannerInfo *root,
								RestrictInfo *restrictinfo)
{
	Relids		relids = restrictinfo->required_relids;

	if (!bms_is_empty(relids))
	{
		int			relid;

		if (bms_get_singleton_member(relids, &relid))
		{
			/*
			 * There is only one relation participating in the clause, so it
			 * is a restriction clause for that relation.
			 */
			add_base_clause_to_rel(root, relid, restrictinfo);
		}
		else
		{
			/*
			 * The clause is a join clause, since there is more than one rel
			 * in its relid set.
			 */

			/*
			 * Check for hashjoinable operators.  (We don't bother setting the
			 * hashjoin info except in true join clauses.)
			 */
			check_hashjoinable(restrictinfo);

			/*
			 * Likewise, check if the clause is suitable to be used with a
			 * Memoize node to cache inner tuples during a parameterized
			 * nested loop.
			 */
			check_memoizable(restrictinfo);

			/*
			 * Add clause to the join lists of all the relevant relations.
			 */
			add_join_clause_to_rels(root, restrictinfo, relids);
		}
	}
	else
	{
		/*
		 * clause references no rels, and therefore we have no place to attach
		 * it.  Shouldn't get here if callers are working properly.
		 */
		elog(ERROR, "cannot cope with variable-free clause");
	}
}
/* …（同文件无关代码省略）… */
static Relids
get_join_domain_min_rels(PlannerInfo *root, Relids domain_relids)
{
	Relids		result = bms_copy(domain_relids);
	ListCell   *lc;

	/* Top-level join domain? */
	if (bms_equal(result, root->all_query_rels))
		return result;

	/* Nope, look for lower outer joins that could potentially commute out */
	foreach(lc, root->join_info_list)
	{
		SpecialJoinInfo *sjinfo = (SpecialJoinInfo *) lfirst(lc);

		if (sjinfo->jointype == JOIN_LEFT &&
			bms_is_member(sjinfo->ojrelid, result))
		{
			result = bms_del_member(result, sjinfo->ojrelid);
			result = bms_del_members(result, sjinfo->syn_righthand);
		}
	}
	return result;
}
/* …（同文件无关代码省略）… */
static void
check_mergejoinable(RestrictInfo *restrictinfo)
{
	Expr	   *clause = restrictinfo->clause;
	Oid			opno;
	Node	   *leftarg;

	if (restrictinfo->pseudoconstant)
		return;
	if (!is_opclause(clause))
		return;
	if (list_length(((OpExpr *) clause)->args) != 2)
		return;

	opno = ((OpExpr *) clause)->opno;
	leftarg = linitial(((OpExpr *) clause)->args);

	if (op_mergejoinable(opno, exprType(leftarg)) &&
		!contain_volatile_functions((Node *) restrictinfo))
		restrictinfo->mergeopfamilies = get_mergejoin_opfamilies(opno);

	/*
	 * Note: op_mergejoinable is just a hint; if we fail to find the operator
	 * in any btree opfamilies, mergeopfamilies remains NIL and so the clause
	 * is not treated as mergejoinable.
	 */
}
/* …（同文件无关代码省略）… */
static void
check_hashjoinable(RestrictInfo *restrictinfo)
{
	Expr	   *clause = restrictinfo->clause;
	Oid			opno;
	Node	   *leftarg;

	if (restrictinfo->pseudoconstant)
		return;
	if (!is_opclause(clause))
		return;
	if (list_length(((OpExpr *) clause)->args) != 2)
		return;

	opno = ((OpExpr *) clause)->opno;
	leftarg = linitial(((OpExpr *) clause)->args);

	if (op_hashjoinable(opno, exprType(leftarg)) &&
		!contain_volatile_functions((Node *) restrictinfo))
		restrictinfo->hashjoinoperator = opno;
}
/* …（同文件无关代码省略）… */
static void
check_memoizable(RestrictInfo *restrictinfo)
{
	TypeCacheEntry *typentry;
	Expr	   *clause = restrictinfo->clause;
	Oid			lefttype;
	Oid			righttype;

	if (restrictinfo->pseudoconstant)
		return;
	if (!is_opclause(clause))
		return;
	if (list_length(((OpExpr *) clause)->args) != 2)
		return;

	lefttype = exprType(linitial(((OpExpr *) clause)->args));

	typentry = lookup_type_cache(lefttype, TYPECACHE_HASH_PROC |
								 TYPECACHE_EQ_OPR);

	if (OidIsValid(typentry->hash_proc) && OidIsValid(typentry->eq_opr))
		restrictinfo->left_hasheqoperator = typentry->eq_opr;

	righttype = exprType(lsecond(((OpExpr *) clause)->args));

	/*
	 * Lookup the right type, unless it's the same as the left type, in which
	 * case typentry is already pointing to the required TypeCacheEntry.
	 */
	if (lefttype != righttype)
		typentry = lookup_type_cache(righttype, TYPECACHE_HASH_PROC |
									 TYPECACHE_EQ_OPR);

	if (OidIsValid(typentry->hash_proc) && OidIsValid(typentry->eq_opr))
		restrictinfo->right_hasheqoperator = typentry->eq_opr;
}
