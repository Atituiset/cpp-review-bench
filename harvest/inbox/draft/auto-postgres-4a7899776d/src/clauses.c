// AUTO-DRAFT from postgres/postgres PR #264ddc9a678c41fca3435f26d7c581fd2b034cf8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
/*
 * Walker context for expression_has_grouping_conflict.  get_eqop is a callback
 * that returns the equality operator used for grouping.  cb_context is opaque
 * to the walker and is forwarded to get_eqop unchanged.
 */
typedef struct
{
	grouping_eqop_callback get_eqop;
	void	   *cb_context;
} grouping_walker_ctx;

static bool contain_agg_clause_walker(Node *node, void *context);
/* …（同文件无关代码省略）… */
static bool
contain_agg_clause_walker(Node *node, void *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Aggref))
	{
		Assert(((Aggref *) node)->agglevelsup == 0);
		return true;			/* abort the tree traversal and return true */
	}
	if (IsA(node, GroupingFunc))
	{
		Assert(((GroupingFunc *) node)->agglevelsup == 0);
		return true;			/* abort the tree traversal and return true */
	}
	Assert(!IsA(node, SubLink));
	return expression_tree_walker(node, contain_agg_clause_walker, context);
}
/* …（同文件无关代码省略）… */

	ctx.get_eqop = get_eqop;
	ctx.cb_context = context;

	return grouping_conflict_walker(expr, &ctx);
}
/* …（同文件无关代码省略）… */
 * member, and RowCompareExpr (one operator and collation per column).  A
 * simple CASE (CaseExpr with a non-NULL arg) is a comparison in disguise:
 * parse analysis builds each WHEN as "OpExpr(CaseTestExpr op val)", with the
 * CaseTestExpr standing in for the arg, so the arg is effectively an operand
 * of each WHEN's comparison.  Those WHEN operators are always the type-default
 * "=", matching the grouping eqop, so only a collation conflict is possible
 * there.
 */
static bool
grouping_conflict_walker(Node *node, grouping_walker_ctx *ctx)
{
	if (node == NULL)
		return false;

	if (IsA(node, Var))
	{
		Var		   *var = (Var *) node;

		/*
		 * A grouping column reaches here when it was not handled as a direct
		 * operand by a comparison node above (see the function header).  That
		 * is safe for a deterministic collation, but not for a
		 * nondeterministic one, where the reference may distinguish values
		 * the grouping considers equal.  A bare boolean qual is safe too:
		 * boolean is not collatable, so it takes the deterministic path here.
		 */
		if (OidIsValid(ctx->get_eqop(var, ctx->cb_context)) &&
			OidIsValid(var->varcollid) &&
			!get_collation_isdeterministic(var->varcollid))
			return true;
		return false;
	}
	else if (IsA(node, OpExpr))
	{
		OpExpr	   *opexpr = (OpExpr *) node;

		if (op_is_safe_index_member(opexpr->opno))
			return grouping_check_operands(opexpr->opno, opexpr->inputcollid,
										   opexpr->args, ctx);
		/* fall through */
	}
	else if (IsA(node, ScalarArrayOpExpr))
	{
		ScalarArrayOpExpr *saop = (ScalarArrayOpExpr *) node;

		if (op_is_safe_index_member(saop->opno))
			return grouping_check_operands(saop->opno, saop->inputcollid,
										   saop->args, ctx);
		/* fall through */
	}
	else if (IsA(node, RowCompareExpr))
	{
		RowCompareExpr *rcexpr = (RowCompareExpr *) node;
		ListCell   *lc_l;
		ListCell   *lc_r;
		ListCell   *lc_o;
		ListCell   *lc_c;

		/* Each column is compared under its own operator and inputcollid. */
		forfour(lc_l, rcexpr->largs,
				lc_r, rcexpr->rargs,
				lc_o, rcexpr->opnos,
				lc_c, rcexpr->inputcollids)
		{
			Oid			opno = lfirst_oid(lc_o);
			Oid			collid = lfirst_oid(lc_c);

			if (grouping_check_operand((Node *) lfirst(lc_l), opno, collid, ctx) ||
				grouping_check_operand((Node *) lfirst(lc_r), opno, collid, ctx))
				return true;
		}
		return false;
	}
	else if (IsA(node, CaseExpr) && ((CaseExpr *) node)->arg != NULL)
	{
		CaseExpr   *cexpr = (CaseExpr *) node;
		Node	   *arg = (Node *) cexpr->arg;

		/* Look through RelabelType to find a direct Var arg. */
		while (arg && IsA(arg, RelabelType))
			arg = (Node *) ((RelabelType *) arg)->arg;

		if (arg && IsA(arg, Var))
		{
			Var		   *var = (Var *) arg;

			/*
			 * The arg is a grouping column compared by every WHEN.  For a
			 * nondeterministic collation, reject if any WHEN applies a
			 * different collation.
			 */
			if (OidIsValid(ctx->get_eqop(var, ctx->cb_context)) &&
				OidIsValid(var->varcollid) &&
				!get_collation_isdeterministic(var->varcollid))
			{
				foreach_node(CaseWhen, cw, cexpr->args)
				{
					Oid			collid = exprInputCollation((Node *) cw->expr);

					if (OidIsValid(collid) && collid != var->varcollid)
						return true;
				}
			}
		}
		else if (grouping_conflict_walker((Node *) cexpr->arg, ctx))
		{
			/* arg is a complex expression; walked as a non-operand */
			return true;
		}

		/*
		 * Walk the WHEN conditions, their results, and the default result as
		 * non-operands.  The WHEN conditions hold a CaseTestExpr in place of
		 * the arg, so they contribute no grouping operand of their own, but
		 * the condition expression or the substitution result may reference
		 * another grouping column.
		 */
		foreach_node(CaseWhen, cw, cexpr->args)
		{
			if (grouping_conflict_walker((Node *) cw->expr, ctx) ||
				grouping_conflict_walker((Node *) cw->result, ctx))
				return true;
		}
		return grouping_conflict_walker((Node *) cexpr->defresult, ctx);
	}

	return expression_tree_walker(node, grouping_conflict_walker, ctx);
}
/* …（同文件无关代码省略）… */
static bool
grouping_check_operands(Oid opno, Oid inputcollid, List *args,
						grouping_walker_ctx *ctx)
{
	ListCell   *lc;

	foreach(lc, args)
	{
		if (grouping_check_operand((Node *) lfirst(lc), opno, inputcollid, ctx))
			return true;
	}
	return false;
}
/* …（同文件无关代码省略）… */
 *		Handle one operand 'arg' of a comparison with operator 'opno' and
 *		collation 'inputcollid'.
 *
 * If 'arg' is a grouping column (after looking through RelabelType), verify
 * that comparison's operator has equality semantics compatible with the
 * grouping eqop and, for a nondeterministic collation, that it uses the same
 * collation; such a direct operand is then fully handled and is not recursed
 * into.  Any other operand is walked normally, so a grouping column buried
 * inside it is seen 
