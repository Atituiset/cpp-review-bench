// AUTO-DRAFT from postgres/postgres PR #0d6103e537e1299046e5906149f9a7d2108be2cf
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct inline_cte_walker_context
{
	const char *ctename;		/* name and relative level of target CTE */
	int			levelsup;
	Query	   *ctequery;		/* query to substitute */
} inline_cte_walker_context;
/* …（同文件无关代码省略）… */
static void inline_cte(PlannerInfo *root, CommonTableExpr *cte);
static bool inline_cte_walker(Node *node, inline_cte_walker_context *context);
static bool sublink_testexpr_is_not_nullable(PlannerInfo *root, SubLink *sublink);
static bool simplify_EXISTS_query(PlannerInfo *root, Query *query);
static Query *convert_EXISTS_to_ANY(PlannerInfo *root, Query *subselect,
									Node **testexpr, List **paramIds);
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
static void
inline_cte(PlannerInfo *root, CommonTableExpr *cte)
{
	struct inline_cte_walker_context context;

	context.ctename = cte->ctename;
	/* Start at levelsup = -1 because we'll immediately increment it */
	context.levelsup = -1;
	context.ctequery = castNode(Query, cte->ctequery);

	(void) inline_cte_walker((Node *) root->parse, &context);
}
/* …（同文件无关代码省略）… */
static bool
inline_cte_walker(Node *node, inline_cte_walker_context *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Query))
	{
		Query	   *query = (Query *) node;

		context->levelsup++;

		/*
		 * Visit the query's RTE nodes after their contents; otherwise
		 * query_tree_walker would descend into the newly inlined CTE query,
		 * which we don't want.
		 */
		(void) query_tree_walker(query, inline_cte_walker, context,
								 QTW_EXAMINE_RTES_AFTER);

		context->levelsup--;

		return false;
	}
	else if (IsA(node, RangeTblEntry))
	{
		RangeTblEntry *rte = (RangeTblEntry *) node;

		if (rte->rtekind == RTE_CTE &&
			strcmp(rte->ctename, context->ctename) == 0 &&
			rte->ctelevelsup == context->levelsup)
		{
			/*
			 * Found a reference to replace.  Generate a copy of the CTE query
			 * with appropriate level adjustment for outer references (e.g.,
			 * to other CTEs).
			 */
			Query	   *newquery = copyObject(context->ctequery);

			if (context->levelsup > 0)
				IncrementVarSublevelsUp((Node *) newquery, context->levelsup, 1);

			/*
			 * Convert the RTE_CTE RTE into a RTE_SUBQUERY.
			 *
			 * Historically, a FOR UPDATE clause has been treated as extending
			 * into views and subqueries, but not into CTEs.  We preserve this
			 * distinction by not trying to push rowmarks into the new
			 * subquery.
			 */
			rte->rtekind = RTE_SUBQUERY;
			rte->subquery = newquery;
			rte->security_barrier = false;

			/* Zero out CTE-specific fields */
			rte->ctename = NULL;
			rte->ctelevelsup = 0;
			rte->self_reference = false;
			rte->coltypes = NIL;
			rte->coltypmods = NIL;
			rte->colcollations = NIL;
		}

		return false;
	}

	return expression_tree_walker(node, inline_cte_walker, context);
}
/* …（同文件无关代码省略）… */
	result->join_using_alias = NULL;
	result->quals = quals;
	result->alias = NULL;
	result->rtindex = 0;		/* we don't need an RTE for it */

	return result;
}
/* …（同文件无关代码省略）… */
static bool
sublink_testexpr_is_not_nullable(PlannerInfo *root, SubLink *sublink)
{
	Node	   *testexpr = sublink->testexpr;
	List	   *outer_exprs = NIL;

	/* Punt if sublink is not in the expected format */
	if (sublink->subLinkType != ANY_SUBLINK || testexpr == NULL)
		return false;

	if (IsA(testexpr, OpExpr))
	{
		/* single-column comparison */
		OpExpr	   *opexpr = (OpExpr *) testexpr;

		/* standard ANY structure should be op(outer_var, param) */
		if (list_length(opexpr->args) != 2)
			return false;

		/*
		 * We rely on membership in a B-tree or Hash operator family as a
		 * guarantee that the operator acts as a proper boolean comparison and
		 * does not yield NULL for valid non-null inputs.
		 */
		if (!op_is_safe_index_member(opexpr->opno))
			return false;

		outer_exprs = lappend(outer_exprs, linitial(opexpr->args));
	}
	else if (is_andclause(testexpr) || is_orclause(testexpr))
	{
		/* multi-column equality or inequality checks */
		BoolExpr   *bexpr = (BoolExpr *) testexpr;

		foreach_ptr(OpExpr, opexpr, bexpr->args)
		{
			if (!IsA(opexpr, OpExpr))
				return false;

			/* standard ANY structure should be op(outer_var, param) */
			if (list_length(opexpr->args) != 2)
				return false;

			/* verify operator safety; see comment above */
			if (!op_is_safe_index_member(opexpr->opno))
				return false;

			outer_exprs = lappend(outer_exprs, linitial(opexpr->args));
		}
	}
	else if (IsA(testexpr, RowCompareExpr))
	{
		/* multi-column ordering checks */
		RowCompareExpr *rcexpr = (RowCompareExpr *) testexpr;

		foreach_oid(opno, rcexpr->opnos)
		{
			/* verify operator safety; see comment above */
			if (!op_is_safe_index_member(opno))
				return false;
		}

		outer_exprs = list_concat(outer_exprs, rcexpr->largs);
	}
	else
	{
		/* Punt if other node types */
		return false;
	}

	/*
	 * Since the query hasn't yet been through expression preprocessing, we
	 * must apply flatten_join_alias_vars to the outer expressions to avoid
	 * being fooled by join aliases.
	 *
	 * We 
