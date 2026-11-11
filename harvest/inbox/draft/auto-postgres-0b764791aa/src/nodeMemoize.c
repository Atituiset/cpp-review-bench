// AUTO-DRAFT from postgres/postgres PR #76c0df7381e5e4d3fba06abddcb51216821c02bc
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>
  // <<< BUG ANCHOR
												 &TTSOpsMinimalTuple);
	mstate->probeslot = MakeSingleTupleTableSlot(mstate->hashkeydesc,
												 &TTSOpsVirtual);

	mstate->param_exprs = palloc_array(ExprState *, nkeys);
	mstate->collations = node->collations;	/* Just point directly to the plan
											 * data */
	mstate->hashfunctions = palloc_array(FmgrInfo, nkeys);

	eqfuncoids = palloc_array(Oid, nkeys);

	for (i = 0; i < nkeys; i++)
	{
		Oid			hashop = node->hashOperators[i];
		Oid			left_hashfn;
		Oid			right_hashfn;
		Expr	   *param_expr = (Expr *) list_nth(node->param_exprs, i);

		if (!get_op_hash_functions(hashop, &left_hashfn, &right_hashfn))
			elog(ERROR, "could not find hash function for hash operator %u",
				 hashop);

		fmgr_info(left_hashfn, &mstate->hashfunctions[i]);

		mstate->param_exprs[i] = ExecInitExpr(param_expr, (PlanState *) mstate);
		eqfuncoids[i] = get_opcode(hashop);
	}

	mstate->cache_eq_expr = ExecBuildParamSetEqual(mstate->hashkeydesc,
												   &TTSOpsMinimalTuple,
												   &TTSOpsVirtual,
												   eqfuncoids,
												   node->collations,
												   node->param_exprs,
												   (PlanState *) mstate);

	pfree(eqfuncoids);
	mstate->mem_used = 0;

	/* Limit the total memory consumed by the cache to this */
/* …（同文件无关代码省略）… */
	mstate->singlerow = node->singlerow;
	mstate->keyparamids = node->keyparamids;

	/*
	 * Record if the cache keys should be compared bit by bit, or logically
	 * using the type's hash equality operator
	 */
	mstate->binary_mode = node->binary_mode;

	/* Zero the statistics counters */
	memset(&mstate->stats, 0, sizeof(MemoizeInstrumentation));
