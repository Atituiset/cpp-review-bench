// AUTO-DRAFT from postgres/postgres PR #372761e05a62856d32fb6a9811edf1c51a286b9c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	Relids		varnos;
	PlannerInfo *root;
	int			sublevels_up;
} pull_varnos_context;
/* …（同文件无关代码省略）… */
typedef struct
{
	PlannerInfo *root;			/* could be NULL! */
	Query	   *query;			/* outer Query */
	int			sublevels_up;
	bool		possible_sublink;	/* could aliases include a SubLink? */
	bool		inserted_sublink;	/* have we inserted a SubLink? */
} flatten_join_alias_vars_context;
/* …（同文件无关代码省略）… */
Relids
pull_varnos_of_level(PlannerInfo *root, Node *node, int levelsup)
{
	pull_varnos_context context;

	context.varnos = NULL;
	context.root = root;
	context.sublevels_up = levelsup;

	/*
	 * Must be prepared to start with a Query or a bare expression tree; if
	 * it's a Query, we don't want to increment sublevels_up.
	 */
	query_or_expression_tree_walker(node,
									pull_varnos_walker,
									&context,
									0);

	return context.varnos;
}
/* …（同文件无关代码省略）… */
static bool
pull_varnos_walker(Node *node, pull_varnos_context *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Var))
	{
		Var		   *var = (Var *) node;

		if (var->varlevelsup == context->sublevels_up)
		{
			context->varnos = bms_add_member(context->varnos, var->varno);
			context->varnos = bms_add_members(context->varnos,
											  var->varnullingrels);
		}
		return false;
	}
	if (IsA(node, CurrentOfExpr))
	{
		CurrentOfExpr *cexpr = (CurrentOfExpr *) node;

		if (context->sublevels_up == 0)
			context->varnos = bms_add_member(context->varnos, cexpr->cvarno);
		return false;
	}
	if (IsA(node, PlaceHolderVar))
	{
		PlaceHolderVar *phv = (PlaceHolderVar *) node;

		/*
		 * If a PlaceHolderVar is not of the target query level, ignore it,
		 * instead recursing into its expression to see if it contains any
		 * vars that are of the target level.  We'll also do that when the
		 * caller doesn't pass a "root" pointer.  (We probably shouldn't see
		 * PlaceHolderVars at all in such cases, but if we do, this is a
		 * reasonable behavior.)
		 */
		if (phv->phlevelsup == context->sublevels_up &&
			context->root != NULL)
		{
			/*
			 * Ideally, the PHV's contribution to context->varnos is its
			 * ph_eval_at set.  However, this code can be invoked before
			 * that's been computed.  If we cannot find a PlaceHolderInfo,
			 * fall back to the conservative assumption that the PHV will be
			 * evaluated at its syntactic level (phv->phrels).
			 *
			 * Another problem is that a PlaceHolderVar can appear in quals or
			 * tlists that have been translated for use in a child appendrel.
			 * Typically such a PHV is a parameter expression sourced by some
			 * other relation, so that the translation from parent appendrel
			 * to child doesn't change its phrels, and we should still take
			 * ph_eval_at at face value.  But in corner cases, the PHV's
			 * original phrels can include the parent appendrel itself, in
			 * which case the translated PHV will have the child appendrel in
			 * phrels, and we must translate ph_eval_at to match.
			 */
			PlaceHolderInfo *phinfo = NULL;

			if (phv->phlevelsup == 0)
			{
				if (phv->phid < context->root->placeholder_array_size)
					phinfo = context->root->placeholder_array[phv->phid];
			}
			if (phinfo == NULL)
			{
				/* No PlaceHolderInfo yet, use phrels */
				context->varnos = bms_add_members(context->varnos,
												  phv->phrels);
			}
			else if (bms_equal(phv->phrels, phinfo->ph_var->phrels))
			{
				/* Normal case: use ph_eval_at */
				context->varnos = bms_add_members(context->varnos,
												  phinfo->ph_eval_at);
			}
			else
			{
				/* Translated PlaceHolderVar: translate ph_eval_at to match */
				Relids		newevalat,
							delta;

				/* remove what was removed from phv->phrels ... */
				delta = bms_difference(phinfo->ph_var->phrels, phv->phrels);
				newevalat = bms_difference(phinfo->ph_eval_at, delta);
				/* ... then if that was in fact part of ph_eval_at ... */
				if (!bms_equal(newevalat, phinfo->ph_eval_at))
				{
					/* ... add what was added */
					delta = bms_difference(phv->phrels, phinfo->ph_var->phrels);
					newevalat = bms_join(newevalat, delta);
				}
				context->varnos = bms_join(context->varnos,
										   newevalat);
			}

			/*
			 * In all three cases, include phnullingrels in the result.  We
			 * don't worry about possibly needing to translate it, because
			 * appendrels only translate varnos of baserels, not outer joins.
			 */
			context->varnos = bms_add_members(context->varnos,
											  phv->phnullingrels);
			return false;		/* don't recurse into expression */
		}
	}
	else if (IsA(node, Query))
	{
		/* Recurse into RTE subquery or not-yet-planned sublink subquery */
		bool		result;

		context->sublevels_up++;
		result = query_tree_walker((Query *) node, pull_varnos_walker,
								   context, 0);
		context->sublevels_up--;
		return result;
	}
	return expression_tree_walker(node, pull_varnos_walker, context);
}
/* …（同文件无关代码省略）… */
 * PlaceHolderVar or constructed from those, we can just add the
 * varnullingrels bits to the existing nullingrels field(s); otherwise
 * we have to add a PlaceHolderVar wrapper.
 */
Node *
flatten_join_alias_vars(PlannerInfo *root, Query *query, Node *node)
{
	flatten_join_alias_vars_context context;

	/*
	 * We do not expect this to be applied to the whole Query, only to
	 * expressions or LATERAL subqueries.  Hence, if the top node is a Query,
	 * it's okay to immediately increment sublevels_up.
	 */
	Assert(node != (Node *) query);

	context.root = root;
	context.query = query;
	context.sublevels_up = 0;
	/* flag whether join aliases could possibly contain SubLinks */
	context.possible_sublink = query->hasSubLinks;
	/* if hasSubLinks is already true, no need to work hard */
	context.inserted_sublink = query->hasSubLinks;

	return flatten_join_alias_vars_mutator(node, &context);
}
/* …（同文件无关代码省略）… */
 * PlaceHolderVars.  We can avoid making PlaceHolderVars in 
