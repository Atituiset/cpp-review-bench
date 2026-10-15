// AUTO-DRAFT from postgres/postgres PR #0fb258d1f2ca65b7299ba7c47045634df05efbff
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
static PgFdwRelationInfo *
init_func_stub_fpinfo(const PgFdwRelationInfo *fpinfo_foreign,
					  RelOptInfo *funcrel)
{
	PgFdwRelationInfo *stub = palloc0_object(PgFdwRelationInfo);

	stub->pushdown_safe = true;

	/* Server-level options, inherited from the foreign side. */
	stub->server = fpinfo_foreign->server;
	stub->shippable_extensions = fpinfo_foreign->shippable_extensions;
	stub->fdw_startup_cost = fpinfo_foreign->fdw_startup_cost;
	stub->fdw_tuple_cost = fpinfo_foreign->fdw_tuple_cost;
	stub->use_remote_estimate = fpinfo_foreign->use_remote_estimate;
	stub->fetch_size = fpinfo_foreign->fetch_size;
	stub->async_capable = fpinfo_foreign->async_capable;

	/* Function-side identity and estimates from the local planner. */
	stub->relation_name = psprintf("%u", funcrel->relid);
	stub->rows = funcrel->rows;
	stub->width = funcrel->reltarget->width;
	stub->retrieved_rows = funcrel->rows;

	/*
	 * The function is executed on the remote server, so these must carry its
	 * cost the same way a foreign rel's do: what producing the rows costs the
	 * far side, before connection setup and data transfer are added.  The
	 * local path for the same function is our best estimate of that.
	 */
	stub->rel_startup_cost = funcrel->cheapest_total_path->startup_cost;
	stub->rel_total_cost = funcrel->cheapest_total_path->total_cost;

	return stub;
}
/* …（同文件无关代码省略）… */
static bool
function_rte_pushdown_ok(PlannerInfo *root, RelOptInfo *rel,
						 RelOptInfo *fdwrel)
{
	RangeTblEntry *rte;
	ListCell   *lc;

	if (rel->rtekind != RTE_FUNCTION)
		return false;
	rte = planner_rt_fetch(rel->relid, root);

	/*
	 * build_simple_rel() copies rtekind straight from the RTE, so for a base
	 * rel rel->rtekind always matches the RTE's; the check above is therefore
	 * sufficient.
	 */
	Assert(rte->rtekind == RTE_FUNCTION);

	if (rte->funcordinality)
		return false;

	/*
	 * Reject up-front any function RTE that lateral-references another
	 * relation: foreign-join push-down would need to parameterise the remote
	 * query per outer row, which we don't support, and even considering the
	 * path is expensive on the planner side.  The surrounding lateral_relids
	 * check in postgresGetForeignJoinPaths() would normally bail out for the
	 * joinrel, but doing the check here avoids walking the function
	 * expression entirely.
	 *
	 * Note this rejects only lateral references to another relation at the
	 * same query level.  A function argument referencing an outer query level
	 * (e.g. f(outer.col) inside a subquery) is a different case: it becomes a
	 * PARAM_EXEC Param, the function rel's lateral_relids is empty, and
	 * foreign_expr_walker() intentionally treats PARAM_EXEC as shippable. The
	 * remote query then carries a parameter placeholder, and postgres_fdw's
	 * ordinary parameter machinery re-sends it on each rescan -- i.e. it
	 * works as a normal parameterized foreign scan, which is fine.
	 */
	if (!bms_is_empty(rel->lateral_relids))
		return false;

	Assert(list_length(rte->functions) >= 1);

	foreach(lc, rte->functions)
	{
		RangeTblFunction *rtfunc = (RangeTblFunction *) lfirst(lc);
		TypeFuncClass functypclass;
		Oid			funcrettype;
		TupleDesc	tupdesc;

		/* Refuse to deal with strange funcexprs */
		if (!IsA(rtfunc->funcexpr, FuncExpr))
			return false;

		if (!OidIsValid(((FuncExpr *) rtfunc->funcexpr)->funcid))
			return false;

		functypclass = get_expr_result_type(rtfunc->funcexpr,
											&funcrettype, &tupdesc);
		if (functypclass != TYPEFUNC_SCALAR)
			return false;
		if (!OidIsValid(funcrettype) ||
			funcrettype == RECORDOID ||
			funcrettype == VOIDOID)
			return false;

		if (contain_subplans(rtfunc->funcexpr))
			return false;
		if (!is_foreign_expr(root, fdwrel, fdwrel->fdw_private, (Expr *) rtfunc->funcexpr))
			return false;
	}

	return true;
}
/* …（同文件无关代码省略）… */
	PgFdwRelationInfo *fpinfo_i;
	ListCell   *lc;
	List	   *joinclauses;
	bool		outer_is_function = false;
	bool		inner_is_function = false;

	/*
	 * We support pushing down INNER, LEFT, RIGHT, FULL OUTER and SEMI joins.
/* …（同文件无关代码省略）… */
	 * function RTE can be absorbed into joins on multiple foreign servers
	 * (each call gets its own stub fpinfo and rechecks shippability for the
	 * specific server).
	 */
	fpinfo = (PgFdwRelationInfo *) joinrel->fdw_private;
	if (jointype == JOIN_INNER && innerrel->rtekind == RTE_FUNCTION &&
		(fpinfo_o = (PgFdwRelationInfo *) outerrel->fdw_private) &&
		fpinfo_o->pushdown_safe &&
		function_rte_pushdown_ok(root, innerrel, outerrel))
	{
		inner_is_function = true;
	}
	else if (jointype == JOIN_INNER && outerrel->rtekind == RTE_FUNCTION &&
			 (fpinfo_i = (PgFdwRelationInfo *) innerrel->fdw_private) &&
			 fpinfo_i->pushdown_safe &&
			 function_rte_pushdown_ok(root, outerrel, innerrel))
	{
		outer_is_function = true;
	}
	else
	{
		fpinfo_o = (PgFdwRelationInfo *) outerrel->fdw_private;
		fpinfo_i = (PgFdwRelationInfo *) innerrel->fdw_private;
		if (!fpinfo_o || !fpinfo_o->pushdown_safe ||
			!fpinfo_i || !fpinfo_i->pushdown_safe)
			return false;
	}

	/*
	 * If one side is a function RTE, allocate a stub fpinfo so the rest of
	 * this function and the cost estimator can treat it uniformly.  We hand
	 * the stub to the joinrel's deparser via the same path the foreign side
	 * uses, but we never permanently attach it to the function rel's
	 * fdw_private (different joinrels may pair the same function RTE with
	 * different foreign servers).
	 */
	if (inner_is_function)
	{
		fpinfo_i = init_func_stub_fpinfo(fpinfo_o, innerrel);

/* …（同文件无关代码省略）… */
						   &fpinfo_i->remote_conds, &fpinfo_i->local_conds);
		fpinfo->inner_func_fpinfo = fpinfo_i;
	}
	else if (outer_is_function)
	{
		fpinfo_o = init_func_stub_fpinfo(fpinfo_i, outerrel);

		/* See the comment in the inner_is_function branch above. */
		classifyConditions(root, outerrel, fpinfo_o, outerrel->baserestrictinfo,
						   &fpinfo_o->remote_conds, &fpinfo_o->local_conds);
		fpinfo->outer
