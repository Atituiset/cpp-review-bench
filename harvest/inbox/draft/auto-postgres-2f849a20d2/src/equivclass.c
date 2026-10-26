// AUTO-DRAFT from postgres/postgres PR #022a80fe6c1d82f7777bd52f9d3e5f9952533109
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static EquivalenceMember *
make_eq_member(EquivalenceClass *ec, Expr *expr, Relids relids,
			   JoinDomain *jdomain, EquivalenceMember *parent, Oid datatype)
{
	EquivalenceMember *em = makeNode(EquivalenceMember);

	em->em_expr = expr;
	em->em_relids = relids;
	em->em_is_const = false;
	em->em_is_child = (parent != NULL);
	em->em_datatype = datatype;
	em->em_jdomain = jdomain;
	em->em_parent = parent;

	if (bms_is_empty(relids))
	{
		/*
		 * No Vars, assume it's a pseudoconstant.  This is correct for entries
		 * generated from process_equivalence(), because a WHERE clause can't
		 * contain aggregates or SRFs, and non-volatility was checked before
		 * process_equivalence() ever got called.  But
		 * get_eclass_for_sort_expr() has to work harder.  We put the tests
		 * there not here to save cycles in the equivalence case.
		 */
		Assert(!parent);
		em->em_is_const = true;
		ec->ec_has_const = true;
		/* it can't affect ec_relids */
	}

	return em;
}
/* …（同文件无关代码省略）… */
static EquivalenceMember *
add_child_eq_member(PlannerInfo *root, EquivalenceClass *ec, int ec_index,
					Expr *expr, Relids relids, JoinDomain *jdomain,
					EquivalenceMember *parent_em, Oid datatype,
					Index child_relid)
{
	EquivalenceMember *em;

	Assert(parent_em != NULL);

	/*
	 * Allocate the array to store child members; an array of Lists indexed by
	 * relid, or expand the existing one, if necessary.
	 */
	if (unlikely(ec->ec_childmembers_size < root->simple_rel_array_size))
	{
		if (ec->ec_childmembers == NULL)
			ec->ec_childmembers = palloc0_array(List *, root->simple_rel_array_size);
		else
			ec->ec_childmembers = repalloc0_array(ec->ec_childmembers, List *,
												  ec->ec_childmembers_size,
												  root->simple_rel_array_size);

		ec->ec_childmembers_size = root->simple_rel_array_size;
	}

	em = make_eq_member(ec, expr, relids, jdomain, parent_em, datatype);

	/* add member to the ec_childmembers List for the given child_relid */
	ec->ec_childmembers[child_relid] = lappend(ec->ec_childmembers[child_relid], em);

	/* Record this EC index for the child rel */
	if (ec_index >= 0)
	{
		RelOptInfo *child_rel = root->simple_rel_array[child_relid];

		child_rel->eclass_indexes =
			bms_add_member(child_rel->eclass_indexes, ec_index);
	}

	return em;
}
/* …（同文件无关代码省略）… */
			{
				/* OK, generate transformed child version */
				Expr	   *child_expr;
				Relids		new_relids;

				if (parent_rel->reloptkind == RELOPT_BASEREL)
				{
/* …（同文件无关代码省略）… */
				}

				/*
				 * Transform em_relids to match.  Note we do *not* do
				 * pull_varnos(child_expr) here, as for example the
				 * transformation might have substituted a constant, but we
				 * don't want the child member to be marked as constant.
				 */
				new_relids = bms_difference(cur_em->em_relids,
											top_parent_relids);
				new_relids = bms_add_members(new_relids, child_relids);

				add_child_eq_member(root,
									cur_ec,
									i,
									child_expr,
									new_relids,
									cur_em->em_jdomain,
									cur_em,
									cur_em->em_datatype,
/* …（同文件无关代码省略）… */
			if (bms_membership(cur_em->em_relids) != BMS_MULTIPLE)
				continue;

			/* Does this member reference child's topmost parent rel? */
			if (bms_overlap(cur_em->em_relids, top_parent_relids))
			{
				/* Yes, generate transformed child version */
				Expr	   *child_expr;
				Relids		new_relids;

				if (parent_joinrel->reloptkind == RELOPT_JOINREL)
				{
/* …（同文件无关代码省略）… */
														  child_joinrel->top_parent);
				}

				/*
				 * Transform em_relids to match.  Note we do *not* do
				 * pull_varnos(child_expr) here, as for example the
				 * transformation might have substituted a constant, but we
				 * don't want the child member to be marked as constant.
				 */
				new_relids = bms_difference(cur_em->em_relids,
											top_parent_relids);
				new_relids = bms_add_members(new_relids, child_relids);

				/*
				 * Add new child member to the EquivalenceClass.  Because this
				 * is a RELOPT_OTHER_JOINREL which has multiple component
/* …（同文件无关代码省略）… */
				 * for all the component relids, then that would just result
				 * in eclass_member_iterator_next() finding the member
				 * multiple times, which is a waste of effort.
				 */
				add_child_eq_member(root,
									cur_ec,
									-1,
									child_expr,
									new_relids,
									cur_em->em_jdomain,
									cur_em,
									cur_em->em_datatype,
