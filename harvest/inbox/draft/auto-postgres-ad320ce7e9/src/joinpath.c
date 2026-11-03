// AUTO-DRAFT from postgres/postgres PR #45ff0b4a317f14539567a3500eb41ec8b06cde3d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	foreach(lc, root->join_info_list)
	{
		SpecialJoinInfo *sjinfo2 = (SpecialJoinInfo *) lfirst(lc);

		/*
		 * SJ is relevant to this join if we have some part of its RHS
		 * (possibly not all of it), and haven't yet joined to its LHS.  (This
		 * test is pretty simplistic, but should be sufficient considering the
		 * join has already been proven legal.)  If the SJ is relevant, it
		 * presents constraints for joining to anything not in its RHS.
		 */
		if (bms_overlap(joinrelids, sjinfo2->min_righthand) &&
			!bms_overlap(joinrelids, sjinfo2->min_lefthand))
			extra.param_source_rels = bms_join(extra.param_source_rels,
											   bms_difference(root->all_baserels,
															  sjinfo2->min_righthand));

		/* full joins constrain both sides symmetrically */
		if (sjinfo2->jointype == JOIN_FULL &&
			bms_overlap(joinrelids, sjinfo2->min_lefthand) &&
			!bms_overlap(joinrelids, sjinfo2->min_righthand))
			extra.param_source_rels = bms_join(extra.param_source_rels,
											   bms_difference(root->all_baserels,
															  sjinfo2->min_lefthand));
	}
/* …（同文件无关代码省略）… */
{
	List	   *lateral_vars;
	List	   *params;
	ListCell   *lc;

	*param_exprs = NIL;
/* …（同文件无关代码省略）… */
	 * key of the Param by itself so that the cached results survive.  This is
	 * only an optimization, so we needn't track down every last such Param,
	 * nor refuse to memoize when one isn't hashable.
	 */
	params = NIL;
	(void) pull_exec_params_walker((Node *) *param_exprs, &params);
	foreach(lc, innerrel->baserestrictinfo)
/* …（同文件无关代码省略）… */
	}

	foreach(lc, params)
		(void) memoize_add_cache_key((Node *) lfirst(lc), param_exprs,
									 operators, binary_mode);

	/* We're okay to use memoize */
	return true;
/* …（同文件无关代码省略）… */
static bool
memoize_add_cache_key(Node *expr, List **param_exprs, List **operators,
					  bool *binary_mode)
{
	/*
	 * 'expr' may already have been added as a cache key, in which case it has
	 * been through the checks below already.
	 */
	if (!list_member(*param_exprs, expr))
	{
		TypeCacheEntry *typentry;

		typentry = lookup_type_cache(exprType(expr),
									 TYPECACHE_HASH_PROC | TYPECACHE_EQ_OPR);

		/* can't use memoize without a valid hash proc and equals operator */
		if (!OidIsValid(typentry->hash_proc) || !OidIsValid(typentry->eq_opr))
			return false;

		*operators = lappend_oid(*operators, typentry->eq_opr);
		*param_exprs = lappend(*param_exprs, expr);
	}

	/*
	 * We must go into binary mode as we don't have too much of an idea of how
	 * 'expr' is being used.  See the comment above where we set *binary_mode
	 * for the join clause case.  This could be relaxed a bit if we knew the
	 * operators being used, however for cases like Vars that are arguments to
	 * functions, or Params appearing in quals we've not examined, we must
	 * operate in binary mode as we've no visibility into what's done with the
	 * value.
	 */
	*binary_mode = true;

	return true;
}
/* …（同文件无关代码省略）… */
static bool
pull_exec_params_walker(Node *node, List **params)
{
	if (node == NULL)
		return false;
	if (IsA(node, Param))
	{
		Param	   *param = (Param *) node;

		if (param->paramkind == PARAM_EXEC &&
			!list_member(*params, param))
			*params = lappend(*params, param);
		return false;
	}
	return expression_tree_walker(node, pull_exec_params_walker, params);
}
