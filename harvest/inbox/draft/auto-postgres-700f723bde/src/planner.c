// AUTO-DRAFT from postgres/postgres PR #852fd5b86e1a2ac1139c716eb4c96c09c03a632e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define EXPRKIND_QUAL				0
#define EXPRKIND_TARGET				1
#define EXPRKIND_RTFUNC				2
/* …（同文件无关代码省略）… */
#define EXPRKIND_VALUES				4
/* …（同文件无关代码省略）… */
#define EXPRKIND_APPINFO			7
#define EXPRKIND_PHV				8
#define EXPRKIND_TABLESAMPLE		9
#define EXPRKIND_ARBITER_ELEM		10
#define EXPRKIND_TABLEFUNC			11
#define EXPRKIND_TABLEFUNC_LATERAL	12
#define EXPRKIND_GROUPEXPR			13

/*
 * Data specific to grouping sets
/* …（同文件无关代码省略）… */
typedef struct
{
	PlannerInfo *root;
	int			sublevels_up;
} preprocess_subquery_phvs_context;
/* …（同文件无关代码省略）… */
		if (rte->rtekind == RTE_RELATION)
		{
			if (rte->tablesample)
				rte->tablesample = (TableSampleClause *)
					preprocess_expression(root,
										  (Node *) rte->tablesample,
										  EXPRKIND_TABLESAMPLE);
		}
		else if (rte->rtekind == RTE_SUBQUERY)
		{
/* …（同文件无关代码省略）… */
static Node *
preprocess_expression(PlannerInfo *root, Node *expr, int kind)
{
	/*
	 * Fall out quickly if expression is empty.  This occurs often enough to
	 * be worth checking.  Note that null->null is the correct conversion for
	 * implicit-AND result format, too.
	 */
	if (expr == NULL)
		return NULL;

	/*
	 * If the query has any join RTEs, replace join alias variables with
	 * base-relation variables.  We must do this first, since any expressions
	 * we may extract from the joinaliasvars lists have not been preprocessed.
	 * For example, if we did this after sublink processing, sublinks expanded
	 * out from join aliases would not get processed.  But we can skip this in
	 * non-lateral RTE functions, VALUES lists, and TABLESAMPLE clauses, since
	 * they can't contain any Vars of the current query level.
	 */
	if (root->hasJoinRTEs &&
		!(kind == EXPRKIND_RTFUNC ||
		  kind == EXPRKIND_VALUES ||
		  kind == EXPRKIND_TABLESAMPLE ||
		  kind == EXPRKIND_TABLEFUNC))
		expr = flatten_join_alias_vars(root, root->parse, expr);

	/*
	 * Simplify constant expressions.  For function RTEs, this was already
	 * done by preprocess_function_rtes.  (But note we must do it again for
	 * EXPRKIND_RTFUNC_LATERAL, because those might by now contain
	 * un-simplified subexpressions inserted by flattening of subqueries or
	 * join alias variables.)
	 *
	 * Note: an essential effect of this is to convert named-argument function
	 * calls to positional notation and insert the current actual values of
	 * any default arguments for functions.  To ensure that happens, we *must*
	 * process all expressions here.  Previous PG versions sometimes skipped
	 * const-simplification if it didn't seem worth the trouble, but we can't
	 * do that anymore.
	 *
	 * Note: this also flattens nested AND and OR expressions into N-argument
	 * form.  All processing of a qual expression after this point must be
	 * careful to maintain AND/OR flatness --- that is, do not generate a tree
	 * with AND directly under AND, nor OR directly under OR.
	 */
	if (kind != EXPRKIND_RTFUNC)
		expr = eval_const_expressions(root, expr);

	/*
	 * If it's a qual or havingQual, canonicalize it.
	 */
	if (kind == EXPRKIND_QUAL)
	{
		expr = (Node *) canonicalize_qual((Expr *) expr, false);

#ifdef OPTIMIZER_DEBUG
		printf("After canonicalize_qual()\n");
		pprint(expr);
#endif
	}

	/*
	 * Check for ANY ScalarArrayOpExpr with Const arrays and set the
	 * hashfuncid of any that might execute more quickly by using hash lookups
	 * instead of a linear search.
	 */
	if (kind == EXPRKIND_QUAL || kind == EXPRKIND_TARGET)
	{
		convert_saop_to_hashed_saop(expr);
	}

	/*
	 * Preprocess any copies of our PlaceHolderVars within SubLink subselects.
	 * This must be done after join alias expansion, which can insert such
	 * copies, and before the SubLinks are turned into SubPlans, which collect
	 * those copies as SubPlan arguments.
	 */
	if (root->parse->hasSubLinks && root->glob->lastPHId != 0)
		preprocess_subquery_phvs(root, expr);

	/* Expand SubLinks to SubPlans */
	if (root->parse->hasSubLinks)
		expr = SS_process_sublinks(root, expr, (kind == EXPRKIND_QUAL));

	/*
	 * XXX do not insert anything here unless you have grokked the comments in
	 * SS_replace_correlation_vars ...
	 */

	/* Replace uplevel vars with Param nodes (this IS possible in VALUES) */
	if (root->query_level > 1)
		expr = SS_replace_correlation_vars(root, expr);

	/*
	 * If it's a qual or havingQual, convert it to implicit-AND format. (We
	 * don't want to do this before eval_const_expressions, since the latter
	 * would be unable to simplify a top-level AND correctly. Also,
	 * SS_process_sublinks expects explicit-AND format.)
	 */
	if (kind == EXPRKIND_QUAL)
		expr = (Node *) make_ands_implicit((Expr *) expr);

	return expr;
}
/* …（同文件无关代码省略）… */
static void
preprocess_subquery_phvs(PlannerInfo *root, Node *node)
{
	preprocess_subquery_phvs_context context;

	context.root = root;
	context.sublevels_up = 0;
	(void) preprocess_subquery_phvs_walker(node, &context);
}
/* …（同文件无关代码省略）… */
static bool
preprocess_subquery_phvs_walker(Node *node,
								preprocess_subquery_phvs_context *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Query))
	{
		bool		result;

		context->sublevels_up++;
		result = query_tree_walker((Query *) node,
								   preprocess_subquery_phvs_walker,
								   context, 0);
		context->sublevels_up--;
		return result;
	}
	if (IsA(node, PlaceHolderVar))
	{
		PlaceHolderVar *phv = (PlaceHolderVar *) node;

		/* A PHV of an upper level can't contain anything of our level */
		if (phv->phlevelsup > context->sublevels_up)
			return false;

		/*
		 * Is this a copy of one of our PHVs that is pushed down into a
		 * subquery?
		 */
		if (context->sublevels_up > 0 &&
			phv->phlevelsup == context->sublevels_up)
		{
			int			levelsup = phv->phlevelsup;
			Node	   *expr;

			/* Adjust the expression to our level, preprocess, adjust back */
			expr = copyObject((Node *) phv->phexpr);
			IncrementVarSublevelsUp(expr, -levelsup, 0);
			
