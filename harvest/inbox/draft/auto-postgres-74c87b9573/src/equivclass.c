// AUTO-DRAFT from postgres/postgres PR #f400289946f7d0e66a4169cf4aa56079f1422a2b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

bool
process_equivalence(PlannerInfo *root,
					RestrictInfo **p_restrictinfo,
					JoinDomain *jdomain)
{
	RestrictInfo *restrictinfo = *p_restrictinfo;
	Expr	   *clause = restrictinfo->clause;
	Oid			opno,
				collation,
				item1_type,
				item2_type;
	Expr	   *item1;
	Expr	   *item2;
	Relids		item1_relids,
				item2_relids;
	List	   *opfamilies;
	EquivalenceClass *ec1,
			   *ec2;
	EquivalenceMember *em1,
			   *em2;
	ListCell   *lc1;
	int			ec2_idx;

	/* Should not already be marked as having generated an eclass */
	Assert(restrictinfo->left_ec == NULL);
	Assert(restrictinfo->right_ec == NULL);

	/* Reject if it is potentially postponable by security considerations */
	if (restrictinfo->security_level > 0 && !restrictinfo->leakproof)
		return false;

	/* Extract info from given clause */
	Assert(is_opclause(clause));
	opno = ((OpExpr *) clause)->opno;
	collation = ((OpExpr *) clause)->inputcollid;
	item1 = (Expr *) get_leftop(clause);
	item2 = (Expr *) get_rightop(clause);
	item1_relids = restrictinfo->left_relids;
	item2_relids = restrictinfo->right_relids;

	/*
	 * Ensure both input expressions expose the desired collation (their types
	 * should be OK already); see comments for canonicalize_ec_expression.
	 */
	item1 = canonicalize_ec_expression(item1,
									   exprType((Node *) item1),
									   collation);
	item2 = canonicalize_ec_expression(item2,
									   exprType((Node *) item2),
									   collation);

	/*
	 * Clauses of the form X=X cannot be translated into EquivalenceClasses.
	 * We'd either end up with a single-entry EC, losing the knowledge that
	 * the clause was present at all, or else make an EC with duplicate
	 * entries, causing other issues.
	 */
	if (equal(item1, item2))
	{
		/*
		 * If the operator is strict, then the clause can be treated as just
		 * "X IS NOT NULL".  (Since we know we are considering a top-level
		 * qual, we can ignore the difference between FALSE and NULL results.)
		 * It's worth making the conversion because we'll typically get a much
		 * better selectivity estimate than we would for X=X.
		 *
		 * If the operator is not strict, we can't be sure what it will do
		 * with NULLs, so don't attempt to optimize it.
		 */
		set_opfuncid((OpExpr *) clause);
		if (func_strict(((OpExpr *) clause)->opfuncid))
		{
			NullTest   *ntest = makeNode(NullTest);

			ntest->arg = item1;
			ntest->nulltesttype = IS_NOT_NULL;
			ntest->argisrow = false;	/* correct even if composite arg */
			ntest->location = -1;

			*p_restrictinfo =
				make_restrictinfo(root,
								  (Expr *) ntest,
								  restrictinfo->is_pushed_down,
								  restrictinfo->has_clone,
								  restrictinfo->is_clone,
								  restrictinfo->pseudoconstant,
								  restrictinfo->security_level,
								  NULL,
								  restrictinfo->incompatible_relids,
								  restrictinfo->outer_relids);
		}
		return false;
	}

	/*
	 * We use the declared input types of the operator, not exprType() of the
	 * inputs, as the nominal datatypes for opfamily lookup.  This presumes
	 * that btree operators are always registered with amoplefttype and
	 * amoprighttype equal to their declared input types.  We will need this
	 * info anyway to build EquivalenceMember nodes, and by extracting it now
	 * we can use type comparisons to short-circuit some equal() tests.
	 */
	op_input_types(opno, &item1_type, &item2_type);

	opfamilies = restrictinfo->mergeopfamilies;

	/*
	 * Sweep through the existing EquivalenceClasses looking for matches to
	 * item1 and item2.  These are the possible outcomes:
	 *
	 * 1. We find both in the same EC.  The equivalence is already known, so
	 * there's nothing to do.
	 *
	 * 2. We find both in different ECs.  Merge the two ECs together.
	 *
	 * 3. We find just one.  Add the other to its EC.
	 *
	 * 4. We find neither.  Make a new, two-entry EC.
	 *
	 * Note: since all ECs are built through this process or the similar
	 * search in get_eclass_for_sort_expr(), it's impossible that we'd match
	 * an item in more than one existing nonvolatile EC.  So it's okay to stop
	 * at the first match.
	 */
	ec1 = ec2 = NULL;
	em1 = em2 = NULL;
	ec2_idx = -1;
	foreach(lc1, root->eq_classes)
	{
		EquivalenceClass *cur_ec = (EquivalenceClass *) lfirst(lc1);
		ListCell   *lc2;

		/* Never match to a volatile EC */
		if (cur_ec->ec_has_volatile)
			continue;

		/*
		 * The collation has to match; check this first since it's cheaper
		 * than the opfamily comparison.
		 */
		if (collation != cur_ec->ec_collation)
			continue;

		/*
		 * A "match" requires matching sets of btree opfamilies.  Use of
		 * equal() for this test has implications discussed in the comments
		 * for get_mergejoin_opfamilies().
		 */
		if (!equal(opfamilies, cur_ec->ec_opfamilies))
			continue;

		/* We don't expect any children yet */
		Assert(cur_ec->ec_childmembers == NULL);

		foreach(lc2, cur_ec->ec_members)
		{
			EquivalenceMember *cur_em = (EquivalenceMember *) lfirst(lc2);

			/* Child members should not exist in ec_members */
			Assert(!cur_em->em_is_child);

			/*
			 * Match constants only within the same JoinDomain (see
			 * optimizer/README).
			 */
			if (cur_em->em_is_const && cur_em->em_jdomain != jdomain)
				continue;

			if (!ec1 &&
				item1_type == cur_em->em_datatype &&
				equal(item1, cur_em->em_expr))
			{
				ec1 = cur_ec;
				em1 = cur_em;
				if (ec2)
					break;
			}

			if (!ec2 &&
				item2_type == cur_em->em_datatype &&
				equal(item2, cur_em->em_expr))
			{
				ec2 = cur_ec;
				ec2_idx = foreach_current_index(lc1);
				em2 = cur_em;
				if (ec1)
					break;
			}
		}

		if (ec1 && ec2)
			break;
	}

	/* Sweep finished, what did we find? */

	if (ec1 && ec2)
	{
		/* If case 1, nothing to do, except add to sources */
		if (ec1 == ec2)
		{
			ec1->ec_sources = lappend(ec1->ec_sources, restrictinfo);
			ec1->ec_min_security = Min(ec1->ec_min_security,
									   restrictinfo->s
