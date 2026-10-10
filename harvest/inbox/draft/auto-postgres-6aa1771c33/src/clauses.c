// AUTO-DRAFT from postgres/postgres PR #d8ce0050a6cff3efefa5fce8abe3c6df7bd492ce
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
typedef struct
{
	ParamListInfo boundParams;
	PlannerInfo *root;
	List	   *active_fns;
	Node	   *case_val;
	bool		estimate;
} eval_const_expressions_context;
/* …（同文件无关代码省略）… */
typedef struct
{
	char	   *proname;
	char	   *prosrc;
} inline_error_callback_arg;
/* …（同文件无关代码省略）… */
bool
contain_subplans(Node *clause)
{
	return contain_subplans_walker(clause, NULL);
}
/* …（同文件无关代码省略）… */
bool
contain_mutable_functions(Node *clause)
{
	return contain_mutable_functions_walker(clause, NULL);
}
/* …（同文件无关代码省略）… */
static bool
contain_mutable_functions_checker(Oid func_id, void *context)
{
	return (func_volatile(func_id) != PROVOLATILE_IMMUTABLE);
}
/* …（同文件无关代码省略）… */
static bool
contain_mutable_functions_walker(Node *node, void *context)
{
	if (node == NULL)
		return false;
	/* Check for mutable functions in node itself */
	if (check_functions_in_node(node, contain_mutable_functions_checker,
								context))
		return true;

	if (IsA(node, JsonConstructorExpr))
	{
		const JsonConstructorExpr *ctor = (JsonConstructorExpr *) node;
		ListCell   *lc;
		bool		is_jsonb;

		is_jsonb = ctor->returning->format->format_type == JS_FORMAT_JSONB;

		/*
		 * Check argument_type => json[b] conversions specifically.  We still
		 * recurse to check 'args' below, but here we want to specifically
		 * check whether or not the emitted clause would fail to be immutable
		 * because of TimeZone, for example.
		 */
		foreach(lc, ctor->args)
		{
			Oid			typid = exprType(lfirst(lc));

			if (is_jsonb ?
				!to_jsonb_is_immutable(typid) :
				!to_json_is_immutable(typid))
				return true;
		}

		/* Check all subnodes */
	}

	if (IsA(node, JsonExpr))
	{
		JsonExpr   *jexpr = castNode(JsonExpr, node);
		Const	   *cnst;

		if (!IsA(jexpr->path_spec, Const))
			return true;

		cnst = castNode(Const, jexpr->path_spec);

		Assert(cnst->consttype == JSONPATHOID);
		if (cnst->constisnull)
			return false;

		if (jspIsMutable(DatumGetJsonPathP(cnst->constvalue),
						 jexpr->passing_names, jexpr->passing_values))
			return true;
	}

	if (IsA(node, SQLValueFunction))
	{
		/* all variants of SQLValueFunction are stable */
		return true;
	}

	if (IsA(node, XmlExpr))
	{
		/* some variants of XmlExpr are only stable */
		if (!xmlexpr_is_immutable((XmlExpr *) node))
			return true;
	}

	if (IsA(node, NextValueExpr))
	{
		/* NextValueExpr is volatile */
		return true;
	}

	/*
	 * It should be safe to treat MinMaxExpr as immutable, because it will
	 * depend on a non-cross-type btree comparison function, and those should
	 * always be immutable.  Treating CoerceToDomain as immutable is outright
	 * dangerous, but we have done so historically, and changing this would
	 * probably cause more problems than it would fix.  In practice, if you
	 * have a non-immutable domain constraint you are in for pain anyhow.
	 */

	/* Recurse to check arguments */
	if (IsA(node, Query))
	{
		/* Recurse into subselects */
		return query_tree_walker((Query *) node,
								 contain_mutable_functions_walker,
								 context, 0);
	}
	return expression_tree_walker(node, contain_mutable_functions_walker,
								  context);
}
/* …（同文件无关代码省略）… */
static bool
xmlexpr_is_immutable(XmlExpr *xexpr)
{
	switch (xexpr->op)
	{
		case IS_XMLCONCAT:
		case IS_XMLPARSE:
		case IS_XMLPI:
		case IS_XMLROOT:
		case IS_XMLSERIALIZE:
		case IS_DOCUMENT:
			/* These variants manipulate XML text in a self-contained way */
			return true;

		case IS_XMLELEMENT:
		case IS_XMLFOREST:

			/*
			 * These variants invoke I/O conversion functions for a wide range
			 * of data types, and have various special rules too, so in some
			 * cases they are only stable.  In principle we could analyze
			 * their behavior precisely, but keeping such code in sync with
			 * the actual implementation seems like more maintenance risk than
			 * it's worth.
			 */
			return false;

			/* There is intentionally no default: case here */
	}
	/* We shouldn't get here, but if we do, say "not immutable" */
	return false;
}
/* …（同文件无关代码省略）… */
bool
contain_volatile_functions(Node *clause)
{
	return contain_volatile_functions_walker(clause, NULL);
}
/* …（同文件无关代码省略）… */
bool
contain_nonstrict_functions(Node *clause)
{
	return contain_nonstrict_functions_walker(clause, NULL);
}
/* …（同文件无关代码省略）… */
static bool
contain_context_dependent_node(Node *clause)
{
	int			flags = 0;

	return contain_context_dependent_node_walker(clause, &flags);
}
/* …（同文件无关代码省略）… */
static bool
rowtype_field_matches(Oid rowtypeid, int fieldnum,
					  Oid expectedtype, int32 expectedtypmod,
					  Oid expectedcollation)
{
	TupleDesc	tupdesc;
	Form_pg_attribute attr;

	/* No issue for RECORD, since there is no way to ALTER such a type */
	if (rowtypeid == RECORDOID)
		return true;
	tupdesc = lookup_rowtype_tupdesc_domain(rowtypeid, -1, false);
	if (fieldnum <= 0 || fieldnum > tupdesc->natts)
	{
		ReleaseTupleDesc(tupdesc);
		return false;
	}
	attr = TupleDescAttr(tupdesc, fieldnum - 1);
	if (attr->attisdropped ||
		attr->atttypid != expectedtype ||
		attr->atttypmod != expectedtypmod ||
		attr->attcollation != expectedcollation)
	{
		ReleaseTupleDesc(tupdesc);
		return false;
	}
	ReleaseTupleDesc(tupdesc);
	return true;
}
/* …（同文件无关代码省略）… */
#define ece_generic_processing(node) \
	expression_tree_mutator((Node *) (node), eval_const_expressions_mutator, \
							context)
/* …（同文件无关代码省略）… */
#define ece_all_arguments_const(node) \
	(!expression_tree_walker((Node *) (node), contain_non_const_walker, NULL))
/* …（同文件无关代码省略）… */
#define ece_evaluate_expr(node) \
	((Node *) evaluate_expr((Expr *) (node), \
							exprType((Node *) (node)), \
							exprTypmod((Node *) (node)), \
							exprCollation((Node *) (node))))
/* …（同文件无关代码省略）… */
static Node *
eval_const_expressions_mutator(Node *node,
							   eval_const_expressions_context *context)
{

	/* since this function recurses, it could be driven to stack overflow */
	check_stack_depth();

	if (node == NULL)
		return NULL;
	switch (nodeTag(node))
	{
		case T_Param:
			{
				Param	   *param = (Param *) node;
				ParamListInfo paramLI = context->boundParams;

				/* Look to see if we've been given a value for this Param */
				if (param->paramkind == PARAM_EXTERN &&
					paramLI != NULL &&
					param->paramid > 0 &&
					param->paramid <= paramLI->numParams)
				{
					ParamExternData *prm;
					ParamExternData prmdata;

					/*
					 * Give hook a chance in case parameter is dynamic.  Tell
					 * it that this fetch is speculative, so it should avoid
					 * erroring out if parameter is unavailable.
					 */
					if (paramLI->paramFetch != NULL)
						prm = paramLI->paramFetch(paramLI, param->paramid,
												  true, &prmdata);
					else
						prm = &paramLI->params[param->paramid - 1];

					/*
					 * We don't just check OidIsValid, but insist that the
					 * fetched type match the Param, just in case the hook did
					 * something unexpected.  No need to throw an error here
					 * though; leave that for runtime.
					 */
					if (OidIsValid(prm->ptype) &&
						prm->ptype == param->paramtype)
					{
						/* OK to substitute parameter value? */
						if (context->estimate ||
							(prm->pflags & PARAM_FLAG_CONST))
						{
							/*
							 * Return a Const representing the param value.
							 * Must copy pass-by-ref datatypes, since the
							 * Param might be in a memory context
							 * shorter-lived than our output plan should be.
							 */
							int16		typLen;
							bool		typByVal;
							Datum		pval;
							Const	   *con;

							get_typlenbyval(param->paramtype,
											&typLen, &typByVal);
							if (prm->isnull || typByVal)
								pval = prm->value;
							else
								pval = datumCopy(prm->value, typByVal, typLen);
							con = makeConst(param->paramtype,
											param->paramtypmod,
											param->paramcollid,
											(int) typLen,
											pval,
											prm->isnull,
											typByVal);
							con->location = param->location;
							return (Node *) con;
						}
					}
				}

				/*
				 * Not replaceable, so just copy the Param (no need to
				 * recurse)
				 */
				return (Node *) copyObject(param);
			}
		case T_WindowFunc:
			{
				WindowFunc *expr = (WindowFunc *) node;
				Oid			funcid = expr->winfnoid;
				List	   *args;
				Expr	   *aggfilter;
				HeapTuple	func_tuple;
				WindowFunc *newexpr;

				/*
				 * We can't really simplify a WindowFunc node, but we mustn't
				 * just fall through to the default processing, because we
				 * have to apply expand_function_arguments to its argument
				 * list.  That takes care of inserting default arguments and
				 * expanding named-argument notation.
				 */
				func_tuple = SearchSysCache1(PROCOID, ObjectIdGetDatum(funcid));
				if (!HeapTupleIsValid(func_tuple))
					elog(ERROR, "cache lookup failed for function %u", funcid);

				args = expand_function_arguments(expr->args,
												 false, expr->wintype,
												 func_tuple);

				ReleaseSysCache(func_tuple);

				/* Now, recursively simplify the args (which are a List) */
				args = (List *)
					expression_tree_mutator((Node *) args,
											eval_const_expressions_mutator,
											context);
				/* ... and the filter expression, which isn't */
				aggfilter = (Expr *)
					eval_const_expressions_mutator((Node *) expr->aggfilter,
												   context);

				/* And build the replacement WindowFunc node */
				newexpr = makeNode(WindowFunc);
				newexpr->winfnoid = expr->winfnoid;
				newexpr->wintype = expr->wintype;
				newexpr->wincollid = expr->wincollid;
				newexpr->inputcollid = expr->inputcollid;
				newexpr->args = args;
				newexpr->aggfilter = aggfilter;
				newexpr->runCondition = expr->runCondition;
				newexpr->winref = expr->winref;
				newexpr->winstar = expr->winstar;
				newexpr->winagg = expr->winagg;
				newexpr->ignore_nulls = expr->ignore_nulls;
				newexpr->location = expr->location;

				return (Node *) newexpr;
			}
		case T_FuncExpr:
			{
				FuncExpr   *expr = (FuncExpr *) node;
				List	   *args = expr->args;
				Expr	   *simple;
				FuncExpr   *newexpr;

				/*
				 * Code for op/func reduction is pretty bulky, so split it out
				 * as a separate function.  Note: exprTypmod normally returns
				 * -1 for a FuncExpr, but not when the node is recognizably a
				 * length coercion; we want to preserve the typmod in the
				 * eventual Const if so.
				 */
				simple = simplify_function(expr->funcid,
										   expr->funcresulttype,
										   exprTypmod(node),
										   expr->funccollid,
										   expr->inputcollid,
										   &args,
										   expr->funcvariadic,
										   true,
										   true,
										   context);
				if (simple)		/* successfully simplified it */
					return (Node *) simple;

				/*
				 * The expression cannot be simplified any further, so build
				 * and return a replacement FuncExpr node using the
				 * possibly-simplified arguments.  Note that we have also
				 * converted the argument list to positional notation.
				 */
				newexpr = makeNode(FuncExpr);
				newexpr->funcid = expr->funcid;
				newexpr->funcresulttype = expr->funcresulttype;
				newexpr->funcretset = expr->funcretset;
				newexpr->funcvariadic = expr->funcvariadic;
				newexpr->funcformat = expr->funcformat;
				newexpr->funccollid = expr->funccollid;
				newexpr->inputcollid = expr->inputcollid;
				newexpr->args = args;
				newexpr->location = expr->location;
				return (Node *) newexpr;
			}
		case T_Aggref:
			node = ece_generic_processing(node);
			if (context->root != NULL)
				return simplify_aggref((Aggref *) node, context);
			return node;
		case T_OpExpr:
			{
				OpExpr	   *expr = (OpExpr *) node;
				List	   *args = expr->args;
				Expr	   *simple;
				OpExpr	   *newexpr;

				/*
				 * Need to get OID of underlying function.  Okay to scribble
				 * on input to this extent.
				 */
				set_opfuncid(expr);

				/*
				 * Code for op/func reduction is pretty bulky, so split it out
				 * as a separate function.
				 */
				simple = simplify_function(expr->opfuncid,
										   expr->opresulttype, -1,
										   expr->opcollid,
										   expr->inputcollid,
										   &args,
										   false,
										   true,
										   true,
										   context);
				if (simple)		/* successfully simplified it */
					return (Node *) simple;

				/*
				 * If the operator is boolean equality or inequality, we know
				 * how to simplify cases involving one constant and one
				 * non-constant argument.
				 */
				if (expr->opno == BooleanEqualOperator ||
					expr->opno == BooleanNotEqualOperator)
				{
					simple = (Expr *) simplify_boolean_equality(expr->opno,
																args);
					if (simple) /* successfully simplified it */
						return (Node *) simple;
				}

				/*
				 * The expression cannot be simplified any further, so build
				 * and return a replacement OpExpr node using the
				 * possibly-simplified arguments.
				 */
				newexpr = makeNode(OpExpr);
				newexpr->opno = expr->opno;
				newexpr->opfuncid = expr->opfuncid;
				newexpr->opresulttype = expr->opresulttype;
				newexpr->opretset = expr->opretset;
				newexpr->opcollid = expr->opcollid;
				newexpr->inputcollid = expr->inputcollid;
				newexpr->args = args;
				newexpr->location = expr->location;
				return (Node *) newexpr;
			}
		case T_DistinctExpr:
			{
				DistinctExpr *expr = (DistinctExpr *) node;
				List	   *args;
				ListCell   *arg;
				bool		has_null_input = false;
				bool		all_null_input = true;
				bool		has_nonconst_input = false;
				bool		has_nullable_nonconst = false;
				Expr	   *simple;
				DistinctExpr *newexpr;

				/*
				 * Reduce constants in the DistinctExpr's arguments.  We know
				 * args is either NIL or a List node, so we can call
				 * expression_tree_mutator directly rather than recursing to
				 * self.
				 */
				args = (List *) expression_tree_mutator((Node *) expr->args,
														eval_const_expressions_mutator,
														context);

				/*
				 * We must do our own check for NULLs because DistinctExpr has
				 * different results for NULL input than the underlying
				 * operator does.  We also check if any non-constant input is
				 * potentially nullable.
				 */
				foreach(arg, args)
				{
					if (IsA(lfirst(arg), Const))
					{
						has_null_input |= ((Const *) lfirst(arg))->constisnull;
						all_null_input &= ((Const *) lfirst(arg))->constisnull;
					}
					else
					{
						has_nonconst_input = true;
						all_null_input = false;

						if (!has_nullable_nonconst &&
							!expr_is_nonnullable(context->root,
												 (Expr *) lfirst(arg),
												 NOTNULL_SOURCE_HASHTABLE))
							has_nullable_nonconst = true;
					}
				}

				if (!has_nonconst_input)
				{
					/*
					 * All inputs are constants.  We can optimize this out
					 * completely.
					 */

					/* all nulls? then not distinct */
					if (all_null_input)
						return makeBoolConst(false, false);

					/* one null? then distinct */
					if (has_null_input)
						return makeBoolConst(true, false);

					/* otherwise try to evaluate the '=' operator */
					/* (NOT okay to try to inline it, though!) */

					/*
					 * Need to get OID of underlying function.  Okay to
					 * scribble on input to this extent.
					 */
					set_opfuncid((OpExpr *) expr);	/* rely on struct
													 * equivalence */

					/*
					 * Code for op/func reduction is pretty bulky, so split it
					 * out as a separate function.
					 */
					simple = simplify_function(expr->opfuncid,
											   expr->opresulttype, -1,
											   expr->opcollid,
											   expr->inputcollid,
											   &args,
											   false,
											   false,
											   false,
											   context);
					if (simple) /* successfully simplified it */
					{
						/*
						 * Since the underlying operator is "=", must negate
						 * its result
						 */
						Const	   *csimple = castNode(Const, simple);

						csimple->constvalue =
							BoolGetDatum(!DatumGetBool(csimple->constvalue));
						return (Node *) csimple;
					}
				}
				else if (!has_nullable_nonconst)
				{
					/*
					 * There are non-constant inputs, but since all of them
					 * are proven non-nullable, "IS DISTINCT FROM" semantics
					 * are much simpler.
					 */

					OpExpr	   *eqexpr;

					/*
					 * If one input is an explicit NULL constant, and the
					 * other is a non-nullable expression, the result is
					 * always TRUE.
					 */
					if (has_null_input)
						return makeBoolConst(true, false);

					/*
					 * Otherwise, both inputs are known non-nullable.  In this
					 * case, "IS DISTINCT FROM" is equivalent to the standard
					 * inequality operator (usually "<>").  We convert this to
					 * an OpExpr, which is a more efficient representation for
					 * the planner.  It can enable the use of partial indexes
					 * and constraint exclusion.  Furthermore, if the clause
					 * is negated (ie, "IS NOT DISTINCT FROM"), the resulting
					 * "=" operator can allow the planner to use index scans,
					 * merge joins, hash joins, and EC-based qual deductions.
					 */
					eqexpr = makeNode(OpExpr);
					eqexpr->opno = expr->opno;
					eqexpr->opfuncid = expr->opfuncid;
					eqexpr->opresulttype = BOOLOID;
					eqexpr->opretset = expr->opretset;
					eqexpr->opcollid = expr->opcollid;
					eqexpr->inputcollid = expr->inputcollid;
					eqexpr->args = args;
					eqexpr->location = expr->location;

					return eval_const_expressions_mutator(negate_clause((Node *) eqexpr),
														  context);
				}
				else if (has_null_input)
				{
					/*
					 * One input is a nullable non-constant expression, and
					 * the other is an explicit NULL constant.  We can
					 * transform this to a NullTest with !argisrow, which is
					 * much more amenable to optimization.
					 */

					NullTest   *nt = makeNode(NullTest);

					nt->arg = (Expr *) (IsA(linitial(args), Const) ?
										lsecond(args) : linitial(args));
					nt->nulltesttype = IS_NOT_NULL;

					/*
					 * argisrow = false is correct whether or not arg is
					 * composite
					 */
					nt->argisrow = false;
					nt->location = expr->location;

					return eval_const_expressions_mutator((Node *) nt, context);
				}

				/*
				 * The expression cannot be simplified any further, so build
				 * and return a replacement DistinctExpr node using the
				 * possibly-simplified arguments.
				 */
				newexpr = makeNode(DistinctExpr);
				newexpr->opno = expr->opno;
				newexpr->opfuncid = expr->opfuncid;
				newexpr->opresulttype = expr->opresulttype;
				newexpr->opretset = expr->opretset;
				newexpr->opcollid = expr->opcollid;
				newexpr->inputcollid = expr->inputcollid;
				newexpr->args = args;
				newexpr->location = expr->location;
				return (Node *) newexpr;
			}
		case T_NullIfExpr:
			{
				NullIfExpr *expr;
				ListCell   *arg;
				bool		has_nonconst_input = false;

				/* Copy the node and const-simplify its arguments */
				expr = (NullIfExpr *) ece_generic_processing(node);

				/* If either argument is NULL they can't be equal */
				foreach(arg, expr->args)
				{
					if (!IsA(lfirst(arg), Const))
						has_nonconst_input = true;
					else if (((Const *) lfirst(arg))->constisnull)
						return (Node *) linitial(expr->args);
				}

				/*
				 * Need to get OID of underlying function before checking if
				 * the function is OK to evaluate.
				 */
				set_opfuncid((OpExpr *) expr);

				if (!has_nonconst_input &&
					ece_function_is_safe(expr->opfuncid, context))
					return ece_evaluate_expr(expr);

				return (Node *) expr;
			}
		case T_ScalarArrayOpExpr:
			{
				ScalarArrayOpExpr *saop;

				/* Copy the node and const-simplify its arguments */
				saop = (ScalarArrayOpExpr *) ece_generic_processing(node);

				/* Make sure we know underlying function */
				set_sa_opfuncid(saop);

				/*
				 * If all arguments are Consts, and it's a safe function, we
				 * can fold to a constant
				 */
				if (ece_all_arguments_const(saop) &&
					ece_function_is_safe(saop->opfuncid, context))
					return ece_evaluate_expr(saop);
				return (Node *) saop;
			}
		case T_BoolExpr:
			{
				BoolExpr   *expr = (BoolExpr *) node;

				switch (expr->boolop)
				{
					case OR_EXPR:
						{
							List	   *newargs;
							bool		haveNull = false;
							bool		forceTrue = false;

							newargs = simplify_or_arguments(expr->args,
															context,
															&haveNull,
															&forceTrue);
							if (forceTrue)
								return makeBoolConst(true, false);
							if (haveNull)
								newargs = lappend(newargs,
												  makeBoolConst(false, true));
							/* If all the inputs are FALSE, result is FALSE */
							if (newargs == NIL)
								return makeBoolConst(false, false);

							/*
							 * If only one nonconst-or-NULL input, it's the
							 * result
							 */
							if (list_length(newargs) == 1)
								return (Node *) linitial(newargs);
							/* Else we still need an OR node */
							return (Node *) make_orclause(newargs);
						}
					case AND_EXPR:
						{
							List	   *newargs;
							bool		haveNull = false;
							bool		forceFalse = false;

							newargs = simplify_and_arguments(expr->args,
															 context,
															 &haveNull,
															 &forceFalse);
							if (forceFalse)
								return makeBoolConst(false, false);
							if (haveNull)
								newargs = lappend(newargs,
												  makeBoolConst(false, true));
							/* If all the inputs are TRUE, result is TRUE */
							if (newargs == NIL)
								return makeBoolConst(true, false);

							/*
							 * If only one nonconst-or-NULL input, it's the
							 * result
							 */
							if (list_length(newargs) == 1)
								return (Node *) linitial(newargs);
							/* Else we still need an AND node */
							return (Node *) make_andclause(newargs);
						}
					case NOT_EXPR:
						{
							Node	   *arg;

							Assert(list_length(expr->args) == 1);
							arg = eval_const_expressions_mutator(linitial(expr->args),
																 context);

							/*
							 * Use negate_clause() to see if we can simplify
							 * away the NOT.
							 */
							return negate_clause(arg);
						}
					default:
						elog(ERROR, "unrecognized boolop: %d",
							 (int) expr->boolop);
						break;
				}
				break;
			}
		case T_JsonValueExpr:
			{
				JsonValueExpr *jve = (JsonValueExpr *) node;
				Node	   *raw_expr = (Node *) jve->raw_expr;
				Node	   *formatted_expr = (Node *) jve->formatted_expr;

				/*
				 * If we can fold formatted_expr to a constant, we can elide
				 * the JsonValueExpr altogether.  Otherwise we must process
				 * raw_expr too.  But JsonFormat is a flat node and requires
				 * no simplification, only copying.
				 */
				formatted_expr = eval_const_expressions_mutator(formatted_expr,
																context);
				if (formatted_expr && IsA(formatted_expr, Const))
					return formatted_expr;

				raw_expr = eval_const_expressions_mutator(raw_expr, context);

				return (Node *) makeJsonValueExpr((Expr *) raw_expr,
												  (Expr *) formatted_expr,
												  copyObject(jve->format));
			}
		case T_JsonConstructorExpr:
			{
				JsonConstructorExpr *jce = (JsonConstructorExpr *) node;
				JsonConstructorExpr *newjce;
				Node	   *save_case_val;

				/*
				 * JSCTOR_JSON_ARRAY_QUERY carries a pre-built executable form
				 * in its func field (a COALESCE-wrapped JSON_ARRAYAGG
				 * subquery, constructed during parse analysis).  Replace the
				 * node with that expression and continue simplifying.
				 */
				if (jce->type == JSCTOR_JSON_ARRAY_QUERY)
					return eval_const_expressions_mutator((Node *) jce->func,
														  context);

				/*
				 * Copy the node and const-simplify its arguments.  We can't
				 * use ece_generic_processing() here because we need to mess
				 * with case_val only while processing the coercion.
				 */
				newjce = makeNode(JsonConstructorExpr);
				memcpy(newjce, jce, sizeof(JsonConstructorExpr));
				newjce->args = (List *)
					eval_const_expressions_mutator((Node *) jce->args,
												   context);
				newjce->func = (Expr *)
					eval_const_expressions_mutator((Node *) jce->func,
												   context);

				/*
				 * Set up for the CaseTestExpr node contained in the coercion.
				 * We must prevent it from absorbing any outer CASE value.
				 */
				save_case_val = context->case_val;
				context->case_val = NULL;

				newjce->coercion = (Expr *)
					eval_const_expressions_mutator((Node *) jce->coercion,
												   context);

				context->case_val = save_case_val;

				return (Node *) newjce;
			}
		case T_SubPlan:
		case T_AlternativeSubPlan:

			/*
			 * Return a SubPlan unchanged --- too late to do anything with it.
			 * This can happen in estimation mode, which runs after SubPlans
			 * have been created.
			 */
			return node;
		case T_RelabelType:
			{
				RelabelType *relabel = (RelabelType *) node;
				Node	   *arg;

				/* Simplify the input ... */
				arg = eval_const_expressions_mutator((Node *) relabel->arg,
													 context);
				/* ... and attach a new RelabelType node, if needed */
				return applyRelabelType(arg,
										relabel->resulttype,
										relabel->resulttypmod,
										relabel->resultcollid,
										relabel->relabelformat,
										relabel->location,
										true);
			}
		case T_CoerceViaIO:
			{
				CoerceViaIO *expr = (CoerceViaIO *) node;
				List	   *args;
				Oid			outfunc;
				bool		outtypisvarlena;
				Oid			infunc;
				Oid			intypioparam;
				Expr	   *simple;
				CoerceViaIO *newexpr;

				/* Make a List so we can use simplify_function */
				args = list_make1(expr->arg);

				/*
				 * CoerceViaIO represents calling the source type's output
				 * function then the result type's input function.  So, try to
				 * simplify it as though it were a stack of two such function
				 * calls.  First we need to know what the functions are.
				 *
				 * Note that the coercion functions are assumed not to care
				 * about input collation, so we just pass InvalidOid for that.
				 */
				getTypeOutputInfo(exprType((Node *) expr->arg),
								  &outfunc, &outtypisvarlena);
				getTypeInputInfo(expr->resulttype,
								 &infunc, &intypioparam);

				simple = simplify_function(outfunc,
										   CSTRINGOID, -1,
										   InvalidOid,
										   InvalidOid,
										   &args,
										   false,
										   true,
										   true,
										   context);
				if (simple)		/* successfully simplified output fn */
				{
					/*
					 * Input functions may want 1 to 3 arguments.  We always
					 * supply all three, trusting that nothing downstream will
					 * complain.
					 */
					args = list_make3(simple,
									  makeConst(OIDOID,
												-1,
												InvalidOid,
												sizeof(Oid),
												ObjectIdGetDatum(intypioparam),
												false,
												true),
									  makeConst(INT4OID,
												-1,
												InvalidOid,
												sizeof(int32),
												Int32GetDatum(-1),
												false,
												true));

					simple = simplify_function(infunc,
											   expr->resulttype, -1,
											   expr->resultcollid,
											   InvalidOid,
											   &args,
											   false,
											   false,
											   true,
											   context);
					if (simple) /* successfully simplified input fn */
						return (Node *) simple;
				}

				/*
				 * The expression cannot be simplified any further, so build
				 * and return a replacement CoerceViaIO node using the
				 * possibly-simplified argument.
				 */
				newexpr = makeNode(CoerceViaIO);
				newexpr->arg = (Expr *) linitial(args);
				newexpr->resulttype = expr->resulttype;
				newexpr->resultcollid = expr->resultcollid;
				newexpr->coerceformat = expr->coerceformat;
				newexpr->location = expr->location;
				return (Node *) newexpr;
			}
		case T_ArrayCoerceExpr:
			{
				ArrayCoerceExpr *ac = makeNode(ArrayCoerceExpr);
				Node	   *save_case_val;

				/*
				 * Copy the node and const-simplify its arguments.  We can't
				 * use ece_generic_processing() here because we need to mess
				 * with case_val only while processing the elemexpr.
				 */
				memcpy(ac, node, sizeof(ArrayCoerceExpr));
				ac->arg = (Expr *)
					eval_const_expressions_mutator((Node *) ac->arg,
												   context);

				/*
				 * Set up for the CaseTestExpr node contained in the elemexpr.
				 * We must prevent it from absorbing any outer CASE value.
				 */
				save_case_val = context->case_val;
				context->case_val = NULL;

				ac->elemexpr = (Expr *)
					eval_const_expressions_mutator((Node *) ac->elemexpr,
												   context);

				context->case_val = save_case_val;

				/*
				 * If constant argument and the per-element expression is
				 * immutable, we can simplify the whole thing to a constant.
				 * Exception: although contain_mutable_functions considers
				 * CoerceToDomain immutable for historical reasons, let's not
				 * do so here; this ensures coercion to an array-over-domain
				 * does not apply the domain's constraints until runtime.
				 */
				if (ac->arg && IsA(ac->arg, Const) &&
					ac->elemexpr && !IsA(ac->elemexpr, CoerceToDomain) &&
					!contain_mutable_functions((Node *) ac->elemexpr))
					return ece_evaluate_expr(ac);

				return (Node *) ac;
			}
		case T_CollateExpr:
			{
				/*
				 * We replace CollateExpr with RelabelType, so as to improve
				 * uniformity of expression representation and thus simplify
				 * comparison of expressions.  Hence this looks very nearly
				 * the same as the RelabelType case, and we can apply the same
				 * optimizations to avoid unnecessary RelabelTypes.
				 */
				CollateExpr *collate = (CollateExpr *) node;
				Node	   *arg;

				/* Simplify the input ... */
				arg = eval_const_expressions_mutator((Node *) collate->arg,
													 context);
				/* ... and attach a new RelabelType node, if needed */
				return applyRelabelType(arg,
										exprType(arg),
										exprTypmod(arg),
										collate->collOid,
										COERCE_IMPLICIT_CAST,
										collate->location,
										true);
			}
		case T_CaseExpr:
			{
				/*----------
				 * CASE expressions can be simplified if there are constant
				 * condition clauses:
				 *		FALSE (or NULL): drop the alternative
				 *		TRUE: drop all remaining alternatives
				 * If the first non-FALSE alternative is a constant TRUE,
				 * we can simplify the entire CASE to that alternative's
				 * expression.  If there are no non-FALSE alternatives,
				 * we simplify the entire CASE to the default result (ELSE).
				 *
				 * If we have a simple-form CASE with constant test
				 * expression, we substitute the constant value for contained
				 * CaseTestExpr placeholder nodes, so that we have the
				 * opportunity to reduce constant test conditions.  For
				 * example this allows
				 *		CASE 0 WHEN 0 THEN 1 ELSE 1/0 END
				 * to reduce to 1 rather than drawing a divide-by-0 error.
				 * Note that when the test expression is constant, we don't
				 * have to include it in the resulting CASE; for example
				 *		CASE 0 WHEN x THEN y ELSE z END
				 * is transformed by the parser to
				 *		CASE 0 WHEN CaseTestExpr = x THEN y ELSE z END
				 * which we can simplify to
				 *		CASE WHEN 0 = x THEN y ELSE z END
				 * It is not necessary for the executor to evaluate the "arg"
				 * expression when executing the CASE, since any contained
				 * CaseTestExprs that might have referred to it will have been
				 * replaced by the constant.
				 *----------
				 */
				CaseExpr   *caseexpr = (CaseExpr *) node;
				CaseExpr   *newcase;
				Node	   *save_case_val;
				Node	   *newarg;
				List	   *newargs;
				bool		const_true_cond;
				Node	   *defresult = NULL;
				ListCell   *arg;

				/* Simplify the test expression, if any */
				newarg = eval_const_expressions_mutator((Node *) caseexpr->arg,
														context);

				/* Set up for contained CaseTestExpr nodes */
				save_case_val = context->case_val;
				if (newarg && IsA(newarg, Const))
				{
					context->case_val = newarg;
					newarg = NULL;	/* not needed anymore, see above */
				}
				else
					context->case_val = NULL;

				/* Simplify the WHEN clauses */
				newargs = NIL;
				const_true_cond = false;
				foreach(arg, caseexpr->args)
				{
					CaseWhen   *oldcasewhen = lfirst_node(CaseWhen, arg);
					Node	   *casecond;
					Node	   *caseresult;

					/* Simplify this alternative's test condition */
					casecond = eval_const_expressions_mutator((Node *) oldcasewhen->expr,
															  context);

					/*
					 * If the test condition is constant FALSE (or NULL), then
					 * drop this WHEN clause completely, without processing
					 * the result.
					 */
					if (casecond && IsA(casecond, Const))
					{
						Const	   *const_input = (Const *) casecond;

						if (const_input->constisnull ||
							!DatumGetBool(const_input->constvalue))
							continue;	/* drop alternative with FALSE cond */
						/* Else it's constant TRUE */
						const_true_cond = true;
					}

					/* Simplify this alternative's result value */
					caseresult = eval_const_expressions_mutator((Node *) oldcasewhen->result,
																context);

					/* If non-constant test condition, emit a new WHEN node */
					if (!const_true_cond)
					{
						CaseWhen   *newcasewhen = makeNode(CaseWhen);

						newcasewhen->expr = (Expr *) casecond;
						newcasewhen->result = (Expr *) caseresult;
						newcasewhen->location = oldcasewhen->location;
						newargs = lappend(newargs, newcasewhen);
						continue;
					}

					/*
					 * Found a TRUE condition, so none of the remaining
					 * alternatives can be reached.  We treat the result as
					 * the default result.
					 */
					defresult = caseresult;
					break;
				}

				/* Simplify the default result, unless we replaced it above */
				if (!const_true_cond)
					defresult = eval_const_expressions_mutator((Node *) caseexpr->defresult,
															   context);

				context->case_val = save_case_val;

				/*
				 * If no non-FALSE alternatives, CASE reduces to the default
				 * result
				 */
				if (newargs == NIL)
					return defresult;
				/* Otherwise we need a new CASE node */
				newcase = makeNode(CaseExpr);
				newcase->casetype = caseexpr->casetype;
				newcase->casecollid = caseexpr->casecollid;
				newcase->arg = (Expr *) newarg;
				newcase->args = newargs;
				newcase->defresult = (Expr *) defresult;
				newcase->location = caseexpr->location;
				return (Node *) newcase;
			}
		case T_CaseTestExpr:
			{
				/*
				 * If we know a constant test value for the current CASE
				 * construct, substitute it for the placeholder.  Else just
				 * return the placeholder as-is.
				 */
				if (context->case_val)
					return copyObject(context->case_val);
				else
					return copyObject(node);
			}
		case T_SubscriptingRef:
		case T_ArrayExpr:
		case T_RowExpr:
		case T_MinMaxExpr:
			{
				/*
				 * Generic handling for node types whose own processing is
				 * known to be immutable, and for which we need no smarts
				 * beyond "simplify if all inputs are constants".
				 *
				 * Treating SubscriptingRef this way assumes that subscripting
				 * fetch and assignment are both immutable.  This constrains
				 * type-specific subscripting implementations; maybe we should
				 * relax it someday.
				 *
				 * Treating MinMaxExpr this way amounts to assuming that the
				 * btree comparison function it calls is immutable; see the
				 * reasoning in contain_mutable_functions_walker.
				 */

				/* Copy the node and const-simplify its arguments */
				node = ece_generic_processing(node);
				/* If all arguments are Consts, we can fold to a constant */
				if (ece_all_arguments_const(node))
					return ece_evaluate_expr(node);
				return node;
			}
		case T_CoalesceExpr:
			{
				CoalesceExpr *coalesceexpr = (CoalesceExpr *) node;
				CoalesceExpr *newcoalesce;
				List	   *newargs;
				ListCell   *arg;

				newargs = NIL;
				foreach(arg, coalesceexpr->args)
				{
					Node	   *e;

					e = eval_const_expressions_mutator((Node *) lfirst(arg),
													   context);

					/*
					 * We can remove null constants from the list.  For a
					 * nonnullable expression, if it has not been preceded by
					 * any non-null-constant expressions then it is the
					 * result.  Otherwise, it's the next argument, but we can
					 * drop following arguments since they will never be
					 * reached.
					 */
					if (IsA(e, Const))
					{
						if (((Const *) e)->constisnull)
							continue;	/* drop null constant */
						if (newargs == NIL)
							return e;	/* first expr */
						newargs = lappend(newargs, e);
						break;
					}
					if (expr_is_nonnullable(context->root, (Expr *) e,
											NOTNULL_SOURCE_HASHTABLE))
					{
						if (newargs == NIL)
							return e;	/* first expr */
						newargs = lappend(newargs, e);
						break;
					}

					newargs = lappend(newargs, e);
				}

				/*
				 * If all the arguments were constant null, the result is just
				 * null
				 */
				if (newargs == NIL)
					return (Node *) makeNullConst(coalesceexpr->coalescetype,
												  -1,
												  coalesceexpr->coalescecollid);

				/*
				 * If there's exactly one surviving argument, we no longer
				 * need COALESCE at all: the result is that argument
				 */
				if (list_length(newargs) == 1)
					return (Node *) linitial(newargs);

				newcoalesce = makeNode(CoalesceExpr);
				newcoalesce->coalescetype = coalesceexpr->coalescetype;
				newcoalesce->coalescecollid = coalesceexpr->coalescecollid;
				newcoalesce->args = newargs;
				newcoalesce->location = coalesceexpr->location;
				return (Node *) newcoalesce;
			}
		case T_SQLValueFunction:
			{
				/*
				 * All variants of SQLValueFunction are stable, so if we are
				 * estimating the expression's value, we should evaluate the
				 * current function value.  Otherwise just copy.
				 */
				SQLValueFunction *svf = (SQLValueFunction *) node;

				if (context->estimate)
					return (Node *) evaluate_expr((Expr *) svf,
												  svf->type,
												  svf->typmod,
												  InvalidOid);
				else
					return copyObject((Node *) svf);
			}
		case T_XmlExpr:
			{
				/*
				 * Some variants of XmlExpr are immutable.  Others are only
				 * stable, but in estimation mode those are still fair game to
				 * simplify.
				 */
				node = ece_generic_processing(node);
				if ((context->estimate ||
					 xmlexpr_is_immutable((XmlExpr *) node)) &&
					ece_all_arguments_const(node))
					return ece_evaluate_expr(node);
				return node;
			}
		case T_FieldSelect:
			{
				/*
				 * We can optimize field selection from a whole-row Var into a
				 * simple Var.  (This case won't be generated directly by the
				 * parser, because ParseComplexProjection short-circuits it.
				 * But it can arise while simplifying functions.)  Also, we
				 * can optimize field selection from a RowExpr construct, or
				 * of course from a constant.
				 *
				 * However, replacing a whole-row Var in this way has a
				 * pitfall: if we've already built the rel targetlist for the
				 * source relation, then the whole-row Var is scheduled to be
				 * produced by the relation scan, but the simple Var probably
				 * isn't, which will lead to a failure in setrefs.c.  This is
				 * not a problem when handling simple single-level queries, in
				 * which expression simplification always happens first.  It
				 * is a risk for lateral references from subqueries, though.
				 * To avoid such failures, don't optimize uplevel references.
				 *
				 * We must also check that the declared type of the field is
				 * still the same as when the FieldSelect was created --- this
				 * can change if someone did ALTER COLUMN TYPE on the rowtype.
				 * If it isn't, we skip the optimization; the case will
				 * probably fail at runtime, but that's not our problem here.
				 */
				FieldSelect *fselect = (FieldSelect *) node;
				FieldSelect *newfselect;
				Node	   *arg;

				arg = eval_const_expressions_mutator((Node *) fselect->arg,
													 context);
				if (arg && IsA(arg, Var) &&
					((Var *) arg)->varattno == InvalidAttrNumber &&
					((Var *) arg)->varlevelsup == 0)
				{
					if (rowtype_field_matches(((Var *) arg)->vartype,
											  fselect->fieldnum,
											  fselect->resulttype,
											  fselect->resulttypmod,
											  fselect->resultcollid))
					{
						Var		   *newvar;

						newvar = makeVar(((Var *) arg)->varno,
										 fselect->fieldnum,
										 fselect->resulttype,
										 fselect->resulttypmod,
										 fselect->resultcollid,
										 ((Var *) arg)->varlevelsup);
						/* New Var has same OLD/NEW returning as old one */
						newvar->varreturningtype = ((Var *) arg)->varreturningtype;
						/* New Var is nullable by same rels as the old one */
						newvar->varnullingrels = ((Var *) arg)->varnullingrels;
						return (Node *) newvar;
					}
				}
				if (arg && IsA(arg, RowExpr))
				{
					RowExpr    *rowexpr = (RowExpr *) arg;

					if (fselect->fieldnum > 0 &&
						fselect->fieldnum <= list_length(rowexpr->args))
					{
						Node	   *fld = (Node *) list_nth(rowexpr->args,
															fselect->fieldnum - 1);

						if (rowtype_field_matches(rowexpr->row_typeid,
												  fselect->fieldnum,
												  fselect->resulttype,
												  fselect->resulttypmod,
												  fselect->resultcollid) &&
							fselect->resulttype == exprType(fld) &&
							fselect->resulttypmod == exprTypmod(fld) &&
							fselect->resultcollid == exprCollation(fld))
							return fld;
					}
				}
				newfselect = makeNode(FieldSelect);
				newfselect->arg = (Expr *) arg;
				newfselect->fieldnum = fselect->fieldnum;
				newfselect->resulttype = fselect->resulttype;
				newfselect->resulttypmod = fselect->resulttypmod;
				newfselect->resultcollid = fselect->resultcollid;
				if (arg && IsA(arg, Const))
				{
					Const	   *con = (Const *) arg;

					if (rowtype_field_matches(con->consttype,
											  newfselect->fieldnum,
											  newfselect->resulttype,
											  newfselect->resulttypmod,
											  newfselect->resultcollid))
						return ece_evaluate_expr(newfselect);
				}
				return (Node *) newfselect;
			}
		case T_NullTest:
			{
				NullTest   *ntest = (NullTest *) node;
				NullTest   *newntest;
				Node	   *arg;

				arg = eval_const_expressions_mutator((Node *) ntest->arg,
													 context);
				if (ntest->argisrow && arg && IsA(arg, RowExpr))
				{
					/*
					 * We break ROW(...) IS [NOT] NULL into separate tests on
					 * its component fields.  This form is usually more
					 * efficient to evaluate, as well as being more amenable
					 * to optimization.
					 */
					RowExpr    *rarg = (RowExpr *) arg;
					List	   *newargs = NIL;
					ListCell   *l;

					foreach(l, rarg->args)
					{
						Node	   *relem = (Node *) lfirst(l);

						/*
						 * A constant field refutes the whole NullTest if it's
						 * of the wrong nullness; else we can discard it.
						 */
						if (relem && IsA(relem, Const))
						{
							Const	   *carg = (Const *) relem;

							if (carg->constisnull ?
								(ntest->nulltesttype == IS_NOT_NULL) :
								(ntest->nulltesttype == IS_NULL))
								return makeBoolConst(false, false);
							continue;
						}

						/*
						 * A proven non-nullable field refutes the whole
						 * NullTest if the test is IS NULL; else we can
						 * discard it.
						 */
						if (relem &&
							expr_is_nonnullable(context->root, (Expr *) relem,
												NOTNULL_SOURCE_HASHTABLE))
						{
							if (ntest->nulltesttype == IS_NULL)
								return makeBoolConst(false, false);
							continue;
						}

						/*
						 * Else, make a scalar (argisrow == false) NullTest
						 * for this field.  Scalar semantics are required
						 * because IS [NOT] NULL doesn't recurse; see comments
						 * in ExecEvalRowNullInt().
						 */
						newntest = makeNode(NullTest);
						newntest->arg = (Expr *) relem;
						newntest->nulltesttype = ntest->nulltesttype;
						newntest->argisrow = false;
						newntest->location = ntest->location;
						newargs = lappend(newargs, newntest);
					}
					/* If all the inputs were constants, result is TRUE */
					if (newargs == NIL)
						return makeBoolConst(true, false);
					/* If only one nonconst input, it's the result */
					if (list_length(newargs) == 1)
						return (Node *) linitial(newargs);
					/* Else we need an AND node */
					return (Node *) make_andclause(newargs);
				}
				if (!ntest->argisrow && arg && IsA(arg, Const))
				{
					Const	   *carg = (Const *) arg;
					bool		result;

					switch (ntest->nulltesttype)
					{
						case IS_NULL:
							result = carg->constisnull;
							break;
						case IS_NOT_NULL:
							result = !carg->constisnull;
							break;
						default:
							elog(ERROR, "unrecognized nulltesttype: %d",
								 (int) ntest->nulltesttype);
							result = false; /* keep compiler quiet */
							break;
					}

					return makeBoolConst(result, false);
				}
				if (!ntest->argisrow && arg &&
					expr_is_nonnullable(context->root, (Expr *) arg,
										NOTNULL_SOURCE_HASHTABLE))
				{
					bool		result;

					switch (ntest->nulltesttype)
					{
						case IS_NULL:
							result = false;
							break;
						case IS_NOT_NULL:
							result = true;
							break;
						default:
							elog(ERROR, "unrecognized nulltesttype: %d",
								 (int) ntest->nulltesttype);
							result = false; /* keep compiler quiet */
							break;
					}

					return makeBoolConst(result, false);
				}

				newntest = makeNode(NullTest);
				newntest->arg = (Expr *) arg;
				newntest->nulltesttype = ntest->nulltesttype;
				newntest->argisrow = ntest->argisrow;
				newntest->location = ntest->location;
				return (Node *) newntest;
			}
		case T_BooleanTest:
			{
				/*
				 * This case could be folded into the generic handling used
				 * for ArrayExpr etc.  But because the simplification logic is
				 * so trivial, applying evaluate_expr() to perform it would be
				 * a heavy overhead.  BooleanTest is probably common enough to
				 * justify keeping this bespoke implementation.
				 */
				BooleanTest *btest = (BooleanTest *) node;
				BooleanTest *newbtest;
				Node	   *arg;

				arg = eval_const_expressions_mutator((Node *) btest->arg,
													 context);
				if (arg && IsA(arg, Const))
				{
					/*
					 * If arg is Const, simplify to constant.
					 */
					Const	   *carg = (Const *) arg;
					bool		result;

					switch (btest->booltesttype)
					{
						case IS_TRUE:
							result = (!carg->constisnull &&
									  DatumGetBool(carg->constvalue));
							break;
						case IS_NOT_TRUE:
							result = (carg->constisnull ||
									  !DatumGetBool(carg->constvalue));
							break;
						case IS_FALSE:
							result = (!carg->constisnull &&
									  !DatumGetBool(carg->constvalue));
							break;
						case IS_NOT_FALSE:
							result = (carg->constisnull ||
									  DatumGetBool(carg->constvalue));
							break;
						case IS_UNKNOWN:
							result = carg->constisnull;
							break;
						case IS_NOT_UNKNOWN:
							result = !carg->constisnull;
							break;
						default:
							elog(ERROR, "unrecognized booltesttype: %d",
								 (int) btest->booltesttype);
							result = false; /* keep compiler quiet */
							break;
					}

					return makeBoolConst(result, false);
				}
				if (arg &&
					expr_is_nonnullable(context->root, (Expr *) arg,
										NOTNULL_SOURCE_HASHTABLE))
				{
					/*
					 * If arg is proven non-nullable, simplify to boolean
					 * expression or constant.
					 */
					switch (btest->booltesttype)
					{
						case IS_TRUE:
						case IS_NOT_FALSE:
							return arg;

						case IS_FALSE:
						case IS_NOT_TRUE:
							return (Node *) make_notclause((Expr *) arg);

						case IS_UNKNOWN:
							return makeBoolConst(false, false);

						case IS_NOT_UNKNOWN:
							return makeBoolConst(true, false);

						default:
							elog(ERROR, "unrecognized booltesttype: %d",
								 (int) btest->booltesttype);
							break;
					}
				}

				newbtest = makeNode(BooleanTest);
				newbtest->arg = (Expr *) arg;
				newbtest->booltesttype = btest->booltesttype;
				newbtest->location = btest->location;
				return (Node *) newbtest;
			}
		case T_CoerceToDomain:
			{
				/*
				 * If the domain currently has no constraints, we replace the
				 * CoerceToDomain node with a simple RelabelType, which is
				 * both far faster to execute and more amenable to later
				 * optimization.  We must then mark the plan as needing to be
				 * rebuilt if the domain's constraints change.
				 *
				 * Also, in estimation mode, always replace CoerceToDomain
				 * nodes, effectively assuming that the coercion will succeed.
				 */
				CoerceToDomain *cdomain = (CoerceToDomain *) node;
				CoerceToDomain *newcdomain;
				Node	   *arg;

				arg = eval_const_expressions_mutator((Node *) cdomain->arg,
													 context);
				if (context->estimate ||
					!DomainHasConstraints(cdomain->resulttype, NULL))
				{
					/* Record dependency, if this isn't estimation mode */
					if (context->root && !context->estimate)
						record_plan_type_dependency(context->root,
													cdomain->resulttype);

					/* Generate RelabelType to substitute for CoerceToDomain */
					return applyRelabelType(arg,
											cdomain->resulttype,
											cdomain->resulttypmod,
											cdomain->resultcollid,
											cdomain->coercionformat,
											cdomain->location,
											true);
				}

				newcdomain = makeNode(CoerceToDomain);
				newcdomain->arg = (Expr *) arg;
				newcdomain->resulttype = cdomain->resulttype;
				newcdomain->resulttypmod = cdomain->resulttypmod;
				newcdomain->resultcollid = cdomain->resultcollid;
				newcdomain->coercionformat = cdomain->coercionformat;
				newcdomain->location = cdomain->location;
				return (Node *) newcdomain;
			}
		case T_PlaceHolderVar:
			{
				PlaceHolderVar *phv = (PlaceHolderVar *) node;

				/*
				 * Leave a PHV of an upper query level alone: its expression
				 * belongs to that level, which has already preprocessed it.
				 * But we do copy the subtree, just to conform to this
				 * function's API spec.
				 */
				if (phv->phlevelsup > 0)
					return copyObject(node);

				/*
				 * In estimation mode, just strip the PlaceHolderVar node
				 * altogether; this amounts to estimating that the contained
				 * value won't be forced to null by an outer join.  In regular
				 * mode we just use the default behavior (ie, simplify the
				 * expression but leave the PlaceHolderVar node intact).
				 */
				if (context->estimate)
					return eval_const_expressions_mutator((Node *) phv->phexpr,
														  context);
			}
			break;
		case T_ConvertRowtypeExpr:
			{
				ConvertRowtypeExpr *cre = castNode(ConvertRowtypeExpr, node);
				Node	   *arg;
				ConvertRowtypeExpr *newcre;

				arg = eval_const_expressions_mutator((Node *) cre->arg,
													 context);

				newcre = makeNode(ConvertRowtypeExpr);
				newcre->resulttype = cre->resulttype;
				newcre->convertformat = cre->convertformat;
				newcre->location = cre->location;

				/*
				 * In case of a nested ConvertRowtypeExpr, we can convert the
				 * leaf row directly to the topmost row format without any
				 * intermediate conversions. (This works because
				 * ConvertRowtypeExpr is used only for child->parent
				 * conversion in inheritance trees, which works by exact match
				 * of column name, and a column absent in an intermediate
				 * result can't be present in the final result.)
				 *
				 * No need to check more than one level deep, because the
				 * above recursion will have flattened anything else.
				 */
				if (arg != NULL && IsA(arg, ConvertRowtypeExpr))
				{
					ConvertRowtypeExpr *argcre = (ConvertRowtypeExpr *) arg;

					arg = (Node *) argcre->arg;

					/*
					 * Make sure an outer implicit conversion can't hide an
					 * inner explicit one.
					 */
					if (newcre->convertformat == COERCE_IMPLICIT_CAST)
						newcre->convertformat = argcre->convertformat;
				}

				newcre->arg = (Expr *) arg;

				if (arg != NULL && IsA(arg, Const))
					return ece_evaluate_expr((Node *) newcre);
				return (Node *) newcre;
			}
		default:
			break;
	}

	/*
	 * For any node type not handled above, copy the node unchanged but
	 * const-simplify its subexpressions.  This is the correct thing for node
	 * types whose behavior might change between planning and execution, such
	 * as CurrentOfExpr.  It's also a safe default for new node types not
	 * known to this routine.
	 */
	return ece_generic_processing(node);
}
/* …（同文件无关代码省略）… */
static bool
contain_non_const_walker(Node *node, void *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Const))
		return false;
	if (IsA(node, List))
		return expression_tree_walker(node, contain_non_const_walker, context);
	/* Otherwise, abort the tree traversal and return true */
	return true;
}
/* …（同文件无关代码省略）… */
static bool
ece_function_is_safe(Oid funcid, eval_const_expressions_context *context)
{
	char		provolatile = func_volatile(funcid);

	/*
	 * Ordinarily we are only allowed to simplify immutable functions. But for
	 * purposes of estimation, we consider it okay to simplify functions that
	 * are merely stable; the risk that the result might change from planning
	 * time to execution time is worth taking in preference to not being able
	 * to estimate the value at all.
	 */
	if (provolatile == PROVOLATILE_IMMUTABLE)
		return true;
	if (context->estimate && provolatile == PROVOLATILE_STABLE)
		return true;
	return false;
}
/* …（同文件无关代码省略）… */
static List *
simplify_or_arguments(List *args,
					  eval_const_expressions_context *context,
					  bool *haveNull, bool *forceTrue)
{
	List	   *newargs = NIL;
	List	   *unprocessed_args;

	/*
	 * We want to ensure that any OR immediately beneath another OR gets
	 * flattened into a single OR-list, so as to simplify later reasoning.
	 *
	 * To avoid stack overflow from recursion of eval_const_expressions, we
	 * resort to some tenseness here: we keep a list of not-yet-processed
	 * inputs, and handle flattening of nested ORs by prepending to the to-do
	 * list instead of recursing.  Now that the parser generates N-argument
	 * ORs from simple lists, this complexity is probably less necessary than
	 * it once was, but we might as well keep the logic.
	 */
	unprocessed_args = list_copy(args);
	while (unprocessed_args)
	{
		Node	   *arg = (Node *) linitial(unprocessed_args);

		unprocessed_args = list_delete_first(unprocessed_args);

		/* flatten nested ORs as per above comment */
		if (is_orclause(arg))
		{
			List	   *subargs = ((BoolExpr *) arg)->args;
			List	   *oldlist = unprocessed_args;

			unprocessed_args = list_concat_copy(subargs, unprocessed_args);
			/* perhaps-overly-tense code to avoid leaking old lists */
			list_free(oldlist);
			continue;
		}

		/* If it's not an OR, simplify it */
		arg = eval_const_expressions_mutator(arg, context);

		/*
		 * It is unlikely but not impossible for simplification of a non-OR
		 * clause to produce an OR.  Recheck, but don't be too tense about it
		 * since it's not a mainstream case.  In particular we don't worry
		 * about const-simplifying the input twice, nor about list leakage.
		 */
		if (is_orclause(arg))
		{
			List	   *subargs = ((BoolExpr *) arg)->args;

			unprocessed_args = list_concat_copy(subargs, unprocessed_args);
			continue;
		}

		/*
		 * OK, we have a const-simplified non-OR argument.  Process it per
		 * comments above.
		 */
		if (IsA(arg, Const))
		{
			Const	   *const_input = (Const *) arg;

			if (const_input->constisnull)
				*haveNull = true;
			else if (DatumGetBool(const_input->constvalue))
			{
				*forceTrue = true;

				/*
				 * Once we detect a TRUE result we can just exit the loop
				 * immediately.  However, if we ever add a notion of
				 * non-removable functions, we'd need to keep scanning.
				 */
				return NIL;
			}
			/* otherwise, we can drop the constant-false input */
			continue;
		}

		/* else emit the simplified arg into the result list */
		newargs = lappend(newargs, arg);
	}

	return newargs;
}
/* …（同文件无关代码省略）… */
static List *
simplify_and_arguments(List *args,
					   eval_const_expressions_context *context,
					   bool *haveNull, bool *forceFalse)
{
	List	   *newargs = NIL;
	List	   *unprocessed_args;

	/* See comments in simplify_or_arguments */
	unprocessed_args = list_copy(args);
	while (unprocessed_args)
	{
		Node	   *arg = (Node *) linitial(unprocessed_args);

		unprocessed_args = list_delete_first(unprocessed_args);

		/* flatten nested ANDs as per above comment */
		if (is_andclause(arg))
		{
			List	   *subargs = ((BoolExpr *) arg)->args;
			List	   *oldlist = unprocessed_args;

			unprocessed_args = list_concat_copy(subargs, unprocessed_args);
			/* perhaps-overly-tense code to avoid leaking old lists */
			list_free(oldlist);
			continue;
		}

		/* If it's not an AND, simplify it */
		arg = eval_const_expressions_mutator(arg, context);

		/*
		 * It is unlikely but not impossible for simplification of a non-AND
		 * clause to produce an AND.  Recheck, but don't be too tense about it
		 * since it's not a mainstream case.  In particular we don't worry
		 * about const-simplifying the input twice, nor about list leakage.
		 */
		if (is_andclause(arg))
		{
			List	   *subargs = ((BoolExpr *) arg)->args;

			unprocessed_args = list_concat_copy(subargs, unprocessed_args);
			continue;
		}

		/*
		 * OK, we have a const-simplified non-AND argument.  Process it per
		 * comments above.
		 */
		if (IsA(arg, Const))
		{
			Const	   *const_input = (Const *) arg;

			if (const_input->constisnull)
				*haveNull = true;
			else if (!DatumGetBool(const_input->constvalue))
			{
				*forceFalse = true;

				/*
				 * Once we detect a FALSE result we can just exit the loop
				 * immediately.  However, if we ever add a notion of
				 * non-removable functions, we'd need to keep scanning.
				 */
				return NIL;
			}
			/* otherwise, we can drop the constant-true input */
			continue;
		}

		/* else emit the simplified arg into the result list */
		newargs = lappend(newargs, arg);
	}

	return newargs;
}
/* …（同文件无关代码省略）… */
static Node *
simplify_boolean_equality(Oid opno, List *args)
{
	Node	   *leftop;
	Node	   *rightop;

	Assert(list_length(args) == 2);
	leftop = linitial(args);
	rightop = lsecond(args);
	if (leftop && IsA(leftop, Const))
	{
		Assert(!((Const *) leftop)->constisnull);
		if (opno == BooleanEqualOperator)
		{
			if (DatumGetBool(((Const *) leftop)->constvalue))
				return rightop; /* true = foo */
			else
				return negate_clause(rightop);	/* false = foo */
		}
		else
		{
			if (DatumGetBool(((Const *) leftop)->constvalue))
				return negate_clause(rightop);	/* true <> foo */
			else
				return rightop; /* false <> foo */
		}
	}
	if (rightop && IsA(rightop, Const))
	{
		Assert(!((Const *) rightop)->constisnull);
		if (opno == BooleanEqualOperator)
		{
			if (DatumGetBool(((Const *) rightop)->constvalue))
				return leftop;	/* foo = true */
			else
				return negate_clause(leftop);	/* foo = false */
		}
		else
		{
			if (DatumGetBool(((Const *) rightop)->constvalue))
				return negate_clause(leftop);	/* foo <> true */
			else
				return leftop;	/* foo <> false */
		}
	}
	return NULL;
}
/* …（同文件无关代码省略）… */
static Expr *
simplify_function(Oid funcid, Oid result_type, int32 result_typmod,
				  Oid result_collid, Oid input_collid, List **args_p,
				  bool funcvariadic, bool process_args, bool allow_non_const,
				  eval_const_expressions_context *context)
{
	List	   *args = *args_p;
	HeapTuple	func_tuple;
	Form_pg_proc func_form;
	Expr	   *newexpr;

	/*
	 * We have three strategies for simplification: execute the function to
	 * deliver a constant result, use a transform function to generate a
	 * substitute node tree, or expand in-line the body of the function
	 * definition (which only works for simple SQL-language functions, but
	 * that is a common case).  Each case needs access to the function's
	 * pg_proc tuple, so fetch it just once.
	 *
	 * Note: the allow_non_const flag suppresses both the second and third
	 * strategies; so if !allow_non_const, simplify_function can only return a
	 * Const or NULL.  Argument-list rewriting happens anyway, though.
	 */
	func_tuple = SearchSysCache1(PROCOID, ObjectIdGetDatum(funcid));
	if (!HeapTupleIsValid(func_tuple))
		elog(ERROR, "cache lookup failed for function %u", funcid);
	func_form = (Form_pg_proc) GETSTRUCT(func_tuple);

	/*
	 * Process the function arguments, unless the caller did it already.
	 *
	 * Here we must deal with named or defaulted arguments, and then
	 * recursively apply eval_const_expressions to the whole argument list.
	 */
	if (process_args)
	{
		args = expand_function_arguments(args, false, result_type, func_tuple);
		args = (List *) expression_tree_mutator((Node *) args,
												eval_const_expressions_mutator,
												context);
		/* Argument processing done, give it back to the caller */
		*args_p = args;
	}

	/* Now attempt simplification of the function call proper. */

	newexpr = evaluate_function(funcid, result_type, result_typmod,
								result_collid, input_collid,
								args, funcvariadic,
								func_tuple, context);

	if (!newexpr && allow_non_const && OidIsValid(func_form->prosupport))
	{
		/*
		 * Build a SupportRequestSimplify node to pass to the support
		 * function, pointing to a dummy FuncExpr node containing the
		 * simplified arg list.  We use this approach to present a uniform
		 * interface to the support function regardless of how the target
		 * function is actually being invoked.
		 */
		SupportRequestSimplify req;
		FuncExpr	fexpr;

		fexpr.xpr.type = T_FuncExpr;
		fexpr.funcid = funcid;
		fexpr.funcresulttype = result_type;
		fexpr.funcretset = func_form->proretset;
		fexpr.funcvariadic = funcvariadic;
		fexpr.funcformat = COERCE_EXPLICIT_CALL;
		fexpr.funccollid = result_collid;
		fexpr.inputcollid = input_collid;
		fexpr.args = args;
		fexpr.location = -1;

		req.type = T_SupportRequestSimplify;
		req.root = context->root;
		req.fcall = &fexpr;

		newexpr = (Expr *)
			DatumGetPointer(OidFunctionCall1(func_form->prosupport,
											 PointerGetDatum(&req)));

		/* catch a possible API misunderstanding */
		Assert(newexpr != (Expr *) &fexpr);
	}

	if (!newexpr && allow_non_const)
		newexpr = inline_function(funcid, result_type, result_collid,
								  input_collid, args, funcvariadic,
								  func_tuple, context);

	ReleaseSysCache(func_tuple);

	return newexpr;
}
/* …（同文件无关代码省略）… */
static Node *
simplify_aggref(Aggref *aggref, eval_const_expressions_context *context)
{
	Oid			prosupport = get_func_support(aggref->aggfnoid);

	if (OidIsValid(prosupport))
	{
		SupportRequestSimplifyAggref req;
		Node	   *newnode;

		/*
		 * Build a SupportRequestSimplifyAggref node to pass to the support
		 * function.
		 */
		req.type = T_SupportRequestSimplifyAggref;
		req.root = context->root;
		req.aggref = aggref;

		newnode = (Node *) DatumGetPointer(OidFunctionCall1(prosupport,
															PointerGetDatum(&req)));

		/*
		 * We expect the support function to return either a new Node or NULL
		 * (when simplification isn't possible).
		 */
		Assert(newnode != (Node *) aggref || newnode == NULL);

		if (newnode != NULL)
			return newnode;
	}

	return (Node *) aggref;
}
/* …（同文件无关代码省略）… */
bool
var_is_nonnullable(PlannerInfo *root, Var *var, NotNullSource source)
{
	Assert(IsA(var, Var));

	/* skip upper-level Vars */
	if (var->varlevelsup != 0)
		return false;

	/* could the Var be nulled by any outer joins or grouping sets? */
	if (!bms_is_empty(var->varnullingrels))
		return false;

	/*
	 * If the Var has a non-default returning type, it could be NULL
	 * regardless of any NOT NULL constraint.  For example, OLD.col is NULL
	 * for INSERT, and NEW.col is NULL for DELETE.
	 */
	if (var->varreturningtype != VAR_RETURNING_DEFAULT)
		return false;

	/* system columns cannot be NULL */
	if (var->varattno < 0)
		return true;

	/* we don't trust whole-row Vars */
	if (var->varattno == 0)
		return false;

	/* Check if the Var is defined as NOT NULL. */
	switch (source)
	{
		case NOTNULL_SOURCE_RELOPT:
			{
				/*
				 * We retrieve the column NOT NULL constraint information from
				 * the corresponding RelOptInfo.
				 */
				RelOptInfo *rel;
				Bitmapset  *notnullattnums;

				rel = find_base_rel(root, var->varno);
				notnullattnums = rel->notnullattnums;

				return bms_is_member(var->varattno, notnullattnums);
			}
		case NOTNULL_SOURCE_HASHTABLE:
			{
				/*
				 * We retrieve the column NOT NULL constraint information from
				 * the hash table.
				 */
				RangeTblEntry *rte;
				Bitmapset  *notnullattnums;

				rte = planner_rt_fetch(var->varno, root);

				/* We can only reason about ordinary relations */
				if (rte->rtekind != RTE_RELATION)
					return false;

				/*
				 * We must skip inheritance parent tables, as some child
				 * tables may have a NOT NULL constraint for a column while
				 * others may not.  This cannot happen with partitioned
				 * tables, though.
				 */
				if (rte->inh && rte->relkind != RELKIND_PARTITIONED_TABLE)
					return false;

				notnullattnums = find_relation_notnullatts(root, rte->relid);

				return bms_is_member(var->varattno, notnullattnums);
			}
		case NOTNULL_SOURCE_CATALOG:
			{
				/*
				 * We check the attnullability field in the tuple descriptor.
				 * This is necessary rather than checking the attnotnull field
				 * from the attribute relation, because attnotnull is also set
				 * for invalid (NOT VALID) NOT NULL constraints, which do not
				 * guarantee the absence of NULLs.
				 */
				RangeTblEntry *rte;
				Relation	rel;
				CompactAttribute *attr;
				bool		result;

				rte = planner_rt_fetch(var->varno, root);

				/* We can only reason about ordinary relations */
				if (rte->rtekind != RTE_RELATION)
					return false;

				/*
				 * We must skip inheritance parent tables, as some child
				 * tables may have a NOT NULL constraint for a column while
				 * others may not.  This cannot happen with partitioned
				 * tables, though.
				 *
				 * Note that we need to check if the relation actually has any
				 * children, as we might not have done that yet.
				 */
				if (rte->inh && has_subclass(rte->relid) &&
					rte->relkind != RELKIND_PARTITIONED_TABLE)
					return false;

				/* We need not lock the relation since it was already locked */
				rel = table_open(rte->relid, NoLock);
				attr = TupleDescCompactAttr(RelationGetDescr(rel),
											var->varattno - 1);
				result = (attr->attnullability == ATTNULLABLE_VALID);
				table_close(rel, NoLock);

				return result;
			}
		default:
			elog(ERROR, "unrecognized NotNullSource: %d",
				 (int) source);
			break;
	}

	return false;
}
/* …（同文件无关代码省略）… */
bool
expr_is_nonnullable(PlannerInfo *root, Expr *expr, NotNullSource source)
{
	/* since this function recurses, it could be driven to stack overflow */
	check_stack_depth();

	switch (nodeTag(expr))
	{
		case T_Var:
			{
				if (root)
					return var_is_nonnullable(root, (Var *) expr, source);
			}
			break;
		case T_Const:
			return !((Const *) expr)->constisnull;
		case T_CoalesceExpr:
			{
				/*
				 * A CoalesceExpr returns NULL if and only if all its
				 * arguments are NULL.  Therefore, we can determine that a
				 * CoalesceExpr cannot be NULL if at least one of its
				 * arguments can be proven non-nullable.
				 */
				CoalesceExpr *coalesceexpr = (CoalesceExpr *) expr;

				foreach_ptr(Expr, arg, coalesceexpr->args)
				{
					if (expr_is_nonnullable(root, arg, source))
						return true;
				}
			}
			break;
		case T_MinMaxExpr:
			{
				/*
				 * Like CoalesceExpr, a MinMaxExpr returns NULL only if all
				 * its arguments evaluate to NULL.
				 */
				MinMaxExpr *minmaxexpr = (MinMaxExpr *) expr;

				foreach_ptr(Expr, arg, minmaxexpr->args)
				{
					if (expr_is_nonnullable(root, arg, source))
						return true;
				}
			}
			break;
		case T_CaseExpr:
			{
				/*
				 * A CASE expression is non-nullable if all branch results are
				 * non-nullable.  We must also verify that the default result
				 * (ELSE) exists and is non-nullable.
				 */
				CaseExpr   *caseexpr = (CaseExpr *) expr;

				/* The default result must be present and non-nullable */
				if (caseexpr->defresult == NULL ||
					!expr_is_nonnullable(root, caseexpr->defresult, source))
					return false;

				/* All branch results must be non-nullable */
				foreach_ptr(CaseWhen, casewhen, caseexpr->args)
				{
					if (!expr_is_nonnullable(root, casewhen->result, source))
						return false;
				}

				return true;
			}
			break;
		case T_ArrayExpr:
			{
				/*
				 * An ARRAY[] expression always returns a valid Array object,
				 * even if it is empty (ARRAY[]) or contains NULLs
				 * (ARRAY[NULL]).  It never evaluates to a SQL NULL.
				 */
				return true;
			}
		case T_NullTest:
			{
				/*
				 * An IS NULL / IS NOT NULL expression always returns a
				 * boolean value.  It never returns SQL NULL.
				 */
				return true;
			}
		case T_BooleanTest:
			{
				/*
				 * A BooleanTest expression always evaluates to a boolean
				 * value.  It never returns SQL NULL.
				 */
				return true;
			}
		case T_DistinctExpr:
			{
				/*
				 * IS DISTINCT FROM never returns NULL, effectively acting as
				 * though NULL were a normal data value.
				 */
				return true;
			}
		case T_RelabelType:
			{
				/*
				 * RelabelType does not change the nullability of the data.
				 * The result is non-nullable if and only if the argument is
				 * non-nullable.
				 */
				return expr_is_nonnullable(root, ((RelabelType *) expr)->arg,
										   source);
			}
		default:
			break;
	}

	return false;
}
/* …（同文件无关代码省略）… */
List *
expand_function_arguments(List *args, bool include_out_arguments,
						  Oid result_type, HeapTuple func_tuple)
{
	Form_pg_proc funcform = (Form_pg_proc) GETSTRUCT(func_tuple);
	Oid		   *proargtypes = funcform->proargtypes.values;
	int			pronargs = funcform->pronargs;
	bool		has_named_args = false;
	ListCell   *lc;

	/*
	 * If we are asked to match to OUT arguments, then use the proallargtypes
	 * array (which includes those); otherwise use proargtypes (which
	 * doesn't).  Of course, if proallargtypes is null, we always use
	 * proargtypes.  (Fetching proallargtypes is annoyingly expensive
	 * considering that we may have nothing to do here, but fortunately the
	 * common case is include_out_arguments == false.)
	 */
	if (include_out_arguments)
	{
		Datum		proallargtypes;
		bool		isNull;

		proallargtypes = SysCacheGetAttr(PROCOID, func_tuple,
										 Anum_pg_proc_proallargtypes,
										 &isNull);
		if (!isNull)
		{
			ArrayType  *arr = DatumGetArrayTypeP(proallargtypes);

			pronargs = ARR_DIMS(arr)[0];
			if (ARR_NDIM(arr) != 1 ||
				pronargs < 0 ||
				ARR_HASNULL(arr) ||
				ARR_ELEMTYPE(arr) != OIDOID)
				elog(ERROR, "proallargtypes is not a 1-D Oid array or it contains nulls");
			Assert(pronargs >= funcform->pronargs);
			proargtypes = (Oid *) ARR_DATA_PTR(arr);
		}
	}

	/* Do we have any named arguments? */
	foreach(lc, args)
	{
		Node	   *arg = (Node *) lfirst(lc);

		if (IsA(arg, NamedArgExpr))
		{
			has_named_args = true;
			break;
		}
	}

	/* If so, we must apply reorder_function_arguments */
	if (has_named_args)
	{
		args = reorder_function_arguments(args, pronargs, func_tuple);
		/* Recheck argument types and add casts if needed */
		recheck_cast_function_args(args, result_type,
								   proargtypes, pronargs,
								   func_tuple);
	}
	else if (list_length(args) < pronargs)
	{
		/* No named args, but we seem to be short some defaults */
		args = add_function_defaults(args, pronargs, func_tuple);
		/* Recheck argument types and add casts if needed */
		recheck_cast_function_args(args, result_type,
								   proargtypes, pronargs,
								   func_tuple);
	}

	return args;
}
/* …（同文件无关代码省略）… */
static List *
reorder_function_arguments(List *args, int pronargs, HeapTuple func_tuple)
{
	Form_pg_proc funcform = (Form_pg_proc) GETSTRUCT(func_tuple);
	int			nargsprovided = list_length(args);
	Node	   *argarray[FUNC_MAX_ARGS];
	ListCell   *lc;
	int			i;

	Assert(nargsprovided <= pronargs);
	if (pronargs < 0 || pronargs > FUNC_MAX_ARGS)
		elog(ERROR, "too many function arguments");
	memset(argarray, 0, pronargs * sizeof(Node *));

	/* Deconstruct the argument list into an array indexed by argnumber */
	i = 0;
	foreach(lc, args)
	{
		Node	   *arg = (Node *) lfirst(lc);

		if (!IsA(arg, NamedArgExpr))
		{
			/* positional argument, assumed to precede all named args */
			Assert(argarray[i] == NULL);
			argarray[i++] = arg;
		}
		else
		{
			NamedArgExpr *na = (NamedArgExpr *) arg;

			Assert(na->argnumber >= 0 && na->argnumber < pronargs);
			Assert(argarray[na->argnumber] == NULL);
			argarray[na->argnumber] = (Node *) na->arg;
		}
	}

	/*
	 * Fetch default expressions, if needed, and insert into array at proper
	 * locations (they aren't necessarily consecutive or all used)
	 */
	if (nargsprovided < pronargs)
	{
		List	   *defaults = fetch_function_defaults(func_tuple);

		i = pronargs - funcform->pronargdefaults;
		foreach(lc, defaults)
		{
			if (argarray[i] == NULL)
				argarray[i] = (Node *) lfirst(lc);
			i++;
		}
	}

	/* Now reconstruct the args list in proper order */
	args = NIL;
	for (i = 0; i < pronargs; i++)
	{
		Assert(argarray[i] != NULL);
		args = lappend(args, argarray[i]);
	}

	return args;
}
/* …（同文件无关代码省略）… */
static List *
add_function_defaults(List *args, int pronargs, HeapTuple func_tuple)
{
	int			nargsprovided = list_length(args);
	List	   *defaults;
	int			ndelete;

	/* Get all the default expressions from the pg_proc tuple */
	defaults = fetch_function_defaults(func_tuple);

	/* Delete any unused defaults from the list */
	ndelete = nargsprovided + list_length(defaults) - pronargs;
	if (ndelete < 0)
		elog(ERROR, "not enough default arguments");
	if (ndelete > 0)
		defaults = list_delete_first_n(defaults, ndelete);

	/* And form the combined argument list, not modifying the input list */
	return list_concat_copy(args, defaults);
}
/* …（同文件无关代码省略）… */
static List *
fetch_function_defaults(HeapTuple func_tuple)
{
	List	   *defaults;
	Datum		proargdefaults;
	char	   *str;

	proargdefaults = SysCacheGetAttrNotNull(PROCOID, func_tuple,
											Anum_pg_proc_proargdefaults);
	str = TextDatumGetCString(proargdefaults);
	defaults = castNode(List, stringToNode(str));
	pfree(str);
	return defaults;
}
/* …（同文件无关代码省略）… */
static void
recheck_cast_function_args(List *args, Oid result_type,
						   Oid *proargtypes, int pronargs,
						   HeapTuple func_tuple)
{
	Form_pg_proc funcform = (Form_pg_proc) GETSTRUCT(func_tuple);
	int			nargs;
	Oid			actual_arg_types[FUNC_MAX_ARGS] = {0};
	Oid			declared_arg_types[FUNC_MAX_ARGS];
	Oid			rettype;
	ListCell   *lc;

	if (list_length(args) > FUNC_MAX_ARGS)
		elog(ERROR, "too many function arguments");
	nargs = 0;
	foreach(lc, args)
	{
		actual_arg_types[nargs++] = exprType((Node *) lfirst(lc));
	}
	Assert(nargs == pronargs);
	memcpy(declared_arg_types, proargtypes, pronargs * sizeof(Oid));
	rettype = enforce_generic_type_consistency(actual_arg_types,
											   declared_arg_types,
											   nargs,
											   funcform->prorettype,
											   false);
	/* let's just check we got the same answer as the parser did ... */
	if (rettype != result_type)
		elog(ERROR, "function's resolved result type changed during planning");

	/* perform any necessary typecasting of arguments */
	make_fn_arguments(NULL, args, actual_arg_types, declared_arg_types);
}
/* …（同文件无关代码省略）… */
static Expr *
evaluate_function(Oid funcid, Oid result_type, int32 result_typmod,
				  Oid result_collid, Oid input_collid, List *args,
				  bool funcvariadic,
				  HeapTuple func_tuple,
				  eval_const_expressions_context *context)
{
	Form_pg_proc funcform = (Form_pg_proc) GETSTRUCT(func_tuple);
	bool		has_nonconst_input = false;
	bool		has_null_input = false;
	ListCell   *arg;
	FuncExpr   *newexpr;

	/*
	 * Can't simplify if it returns a set.
	 */
	if (funcform->proretset)
		return NULL;

	/*
	 * Can't simplify if it returns RECORD.  The immediate problem is that it
	 * will be needing an expected tupdesc which we can't supply here.
	 *
	 * In the case where it has OUT parameters, we could build an expected
	 * tupdesc from those, but there may be other gotchas lurking.  In
	 * particular, if the function were to return NULL, we would produce a
	 * null constant with no remaining indication of which concrete record
	 * type it is.  For now, seems best to leave the function call unreduced.
	 */
	if (funcform->prorettype == RECORDOID)
		return NULL;

	/*
	 * Check for constant inputs and especially constant-NULL inputs.
	 */
	foreach(arg, args)
	{
		if (IsA(lfirst(arg), Const))
			has_null_input |= ((Const *) lfirst(arg))->constisnull;
		else
			has_nonconst_input = true;
	}

	/*
	 * If the function is strict and has a constant-NULL input, it will never
	 * be called at all, so we can replace the call by a NULL constant, even
	 * if there are other inputs that aren't constant, and even if the
	 * function is not otherwise immutable.
	 */
	if (funcform->proisstrict && has_null_input)
		return (Expr *) makeNullConst(result_type, result_typmod,
									  result_collid);

	/*
	 * Otherwise, can simplify only if all inputs are constants. (For a
	 * non-strict function, constant NULL inputs are treated the same as
	 * constant non-NULL inputs.)
	 */
	if (has_nonconst_input)
		return NULL;

	/*
	 * Ordinarily we are only allowed to simplify immutable functions. But for
	 * purposes of estimation, we consider it okay to simplify functions that
	 * are merely stable; the risk that the result might change from planning
	 * time to execution time is worth taking in preference to not being able
	 * to estimate the value at all.
	 */
	if (funcform->provolatile == PROVOLATILE_IMMUTABLE)
		 /* okay */ ;
	else if (context->estimate && funcform->provolatile == PROVOLATILE_STABLE)
		 /* okay */ ;
	else
		return NULL;

	/*
	 * OK, looks like we can simplify this operator/function.
	 *
	 * Build a new FuncExpr node containing the already-simplified arguments.
	 */
	newexpr = makeNode(FuncExpr);
	newexpr->funcid = funcid;
	newexpr->funcresulttype = result_type;
	newexpr->funcretset = false;
	newexpr->funcvariadic = funcvariadic;
	newexpr->funcformat = COERCE_EXPLICIT_CALL; /* doesn't matter */
	newexpr->funccollid = result_collid;	/* doesn't matter */
	newexpr->inputcollid = input_collid;
	newexpr->args = args;
	newexpr->location = -1;

	return evaluate_expr((Expr *) newexpr, result_type, result_typmod,
						 result_collid);
}
/* …（同文件无关代码省略）… */
static Expr *
inline_function(Oid funcid, Oid result_type, Oid result_collid,
				Oid input_collid, List *args,
				bool funcvariadic,
				HeapTuple func_tuple,
				eval_const_expressions_context *context)
{
	Form_pg_proc funcform = (Form_pg_proc) GETSTRUCT(func_tuple);
	char	   *src;
	Datum		tmp;
	bool		isNull;
	MemoryContext oldcxt;
	MemoryContext mycxt;
	inline_error_callback_arg callback_arg;
	ErrorContextCallback sqlerrcontext;
	FuncExpr   *fexpr;
	SQLFunctionParseInfoPtr pinfo;
	TupleDesc	rettupdesc;
	ParseState *pstate;
	List	   *raw_parsetree_list;
	List	   *querytree_list;
	Query	   *querytree;
	Node	   *newexpr;
	int		   *usecounts;
	ListCell   *arg;
	int			i;

	/*
	 * Forget it if the function is not SQL-language or has other showstopper
	 * properties.  (The prokind and nargs checks are just paranoia.)
	 */
	if (funcform->prolang != SQLlanguageId ||
		funcform->prokind != PROKIND_FUNCTION ||
		funcform->prosecdef ||
		funcform->proretset ||
		funcform->prorettype == RECORDOID ||
		!heap_attisnull(func_tuple, Anum_pg_proc_proconfig, NULL) ||
		funcform->pronargs != list_length(args))
		return NULL;

	/* Check for recursive function, and give up trying to expand if so */
	if (list_member_oid(context->active_fns, funcid))
		return NULL;

	/* Check permission to call function (fail later, if not) */
	if (object_aclcheck(ProcedureRelationId, funcid, GetUserId(), ACL_EXECUTE) != ACLCHECK_OK)
		return NULL;

	/* Check whether a plugin wants to hook function entry/exit */
	if (FmgrHookIsNeeded(funcid))
		return NULL;

	/*
	 * Make a temporary memory context, so that we don't leak all the stuff
	 * that parsing might create.
	 */
	mycxt = AllocSetContextCreate(CurrentMemoryContext,
								  "inline_function",
								  ALLOCSET_DEFAULT_SIZES);
	oldcxt = MemoryContextSwitchTo(mycxt);

	/*
	 * We need a dummy FuncExpr node containing the already-simplified
	 * arguments.  (In some cases we don't really need it, but building it is
	 * cheap enough that it's not worth contortions to avoid.)
	 */
	fexpr = makeNode(FuncExpr);
	fexpr->funcid = funcid;
	fexpr->funcresulttype = result_type;
	fexpr->funcretset = false;
	fexpr->funcvariadic = funcvariadic;
	fexpr->funcformat = COERCE_EXPLICIT_CALL;	/* doesn't matter */
	fexpr->funccollid = result_collid;	/* doesn't matter */
	fexpr->inputcollid = input_collid;
	fexpr->args = args;
	fexpr->location = -1;

	/* Fetch the function body */
	tmp = SysCacheGetAttrNotNull(PROCOID, func_tuple, Anum_pg_proc_prosrc);
	src = TextDatumGetCString(tmp);

	/*
	 * Setup error traceback support for ereport().  This is so that we can
	 * finger the function that bad information came from.
	 */
	callback_arg.proname = NameStr(funcform->proname);
	callback_arg.prosrc = src;

	sqlerrcontext.callback = sql_inline_error_callback;
	sqlerrcontext.arg = &callback_arg;
	sqlerrcontext.previous = error_context_stack;
	error_context_stack = &sqlerrcontext;

	/* If we have prosqlbody, pay attention to that not prosrc */
	tmp = SysCacheGetAttr(PROCOID,
						  func_tuple,
						  Anum_pg_proc_prosqlbody,
						  &isNull);
	if (!isNull)
	{
		Node	   *n;
		List	   *query_list;

		n = stringToNode(TextDatumGetCString(tmp));
		if (IsA(n, List))
			query_list = linitial_node(List, castNode(List, n));
		else
			query_list = list_make1(n);
		if (list_length(query_list) != 1)
			goto fail;
		querytree = linitial(query_list);

		/*
		 * Because we'll insist below that the querytree have an empty rtable
		 * and no sublinks, it cannot have any relation references that need
		 * to be locked or rewritten.  So we can omit those steps.
		 */
	}
	else
	{
		/* Set up to handle parameters while parsing the function body. */
		pinfo = prepare_sql_fn_parse_info(func_tuple,
										  (Node *) fexpr,
										  input_collid);

		/*
		 * We just do parsing and parse analysis, not rewriting, because
		 * rewriting will not affect table-free-SELECT-only queries, which is
		 * all that we care about.  Also, we can punt as soon as we detect
		 * more than one command in the function body.
		 */
		raw_parsetree_list = pg_parse_query(src);
		if (list_length(raw_parsetree_list) != 1)
			goto fail;

		pstate = make_parsestate(NULL);
		pstate->p_sourcetext = src;
		sql_fn_parser_setup(pstate, pinfo);

		querytree = transformTopLevelStmt(pstate, linitial(raw_parsetree_list));

		free_parsestate(pstate);
	}

	/*
	 * The single command must be a simple "SELECT expression".
	 *
	 * Note: if you change the tests involved in this, see also plpgsql's
	 * exec_simple_check_plan().  That generally needs to have the same idea
	 * of what's a "simple expression", so that inlining a function that
	 * previously wasn't inlined won't change plpgsql's conclusion.
	 */
	if (!IsA(querytree, Query) ||
		querytree->commandType != CMD_SELECT ||
		querytree->hasAggs ||
		querytree->hasWindowFuncs ||
		querytree->hasTargetSRFs ||
		querytree->hasSubLinks ||
		querytree->cteList ||
		querytree->rtable ||
		querytree->jointree->fromlist ||
		querytree->jointree->quals ||
		querytree->groupClause ||
		querytree->groupingSets ||
		querytree->havingQual ||
		querytree->windowClause ||
		querytree->distinctClause ||
		querytree->sortClause ||
		querytree->limitOffset ||
		querytree->limitCount ||
		querytree->setOperations ||
		list_length(querytree->targetList) != 1)
		goto fail;

	/* If the function result is composite, resolve it */
	(void) get_expr_result_type((Node *) fexpr,
								NULL,
								&rettupdesc);

	/*
	 * Make sure the function (still) returns what it's declared to.  This
	 * will raise an error if wrong, but that's okay since the function would
	 * fail at runtime anyway.  Note that check_sql_fn_retval will also insert
	 * a coercion if needed to make the tlist expression match the declared
	 * type of the function.
	 *
	 * Note: we do not try this until we have verified that no rewriting was
	 * needed; that's probably not important, but let's be careful.
	 */
	querytree_list = list_make1(querytree);
	if (check_sql_fn_retval(list_make1(querytree_list),
							result_type, rettupdesc,
							funcform->prokind,
							false))
		goto fail;				/* reject whole-tuple-result cases */

	/*
	 * Given the tests above, check_sql_fn_retval shouldn't have decided to
	 * inject a projection step, but let's just make sure.
	 */
	if (querytree != linitial(querytree_list))
		goto fail;

	/* Now we can grab the tlist expression */
	newexpr = (Node *) ((TargetEntry *) linitial(querytree->targetList))->expr;

	/*
	 * If the SQL function returns VOID, we can only inline it if it is a
	 * SELECT of an expression returning VOID (ie, it's just a redirection to
	 * another VOID-returning function).  In all non-VOID-returning cases,
	 * check_sql_fn_retval should ensure that newexpr returns the function's
	 * declared result type, so this test shouldn't fail otherwise; but we may
	 * as well cope gracefully if it does.
	 */
	if (exprType(newexpr) != result_type)
		goto fail;

	/*
	 * Additional validity checks on the expression.  It mustn't be more
	 * volatile than the surrounding function (this is to avoid breaking hacks
	 * that involve pretending a function is immutable when it really ain't).
	 * If the surrounding function is declared strict, then the expression
	 * must contain only strict constructs and must use all of the function
	 * parameters (this is overkill, but an exact analysis is hard).
	 */
	if (funcform->provolatile == PROVOLATILE_IMMUTABLE &&
		contain_mutable_functions(newexpr))
		goto fail;
	else if (funcform->provolatile == PROVOLATILE_STABLE &&
			 contain_volatile_functions(newexpr))
		goto fail;

	if (funcform->proisstrict &&
		contain_nonstrict_functions(newexpr))
		goto fail;

	/*
	 * If any parameter expression contains a context-dependent node, we can't
	 * inline, for fear of putting such a node into the wrong context.
	 */
	if (contain_context_dependent_node((Node *) args))
		goto fail;

	/*
	 * We may be able to do it; there are still checks on parameter usage to
	 * make, but those are most easily done in combination with the actual
	 * substitution of the inputs.  So start building expression with inputs
	 * substituted.
	 */
	usecounts = palloc0_array(int, funcform->pronargs);
	newexpr = substitute_actual_parameters(newexpr, funcform->pronargs,
										   args, usecounts);

	/* Now check for parameter usage */
	i = 0;
	foreach(arg, args)
	{
		Node	   *param = lfirst(arg);

		if (usecounts[i] == 0)
		{
			/* Param not used at all: uncool if func is strict */
			if (funcform->proisstrict)
				goto fail;
		}
		else if (usecounts[i] != 1)
		{
			/* Param used multiple times: uncool if expensive or volatile */
			QualCost	eval_cost;

			/*
			 * We define "expensive" as "contains any subplan or more than 10
			 * operators".  Note that the subplan search has to be done
			 * explicitly, since cost_qual_eval() will barf on unplanned
			 * subselects.
			 */
			if (contain_subplans(param))
				goto fail;
			cost_qual_eval(&eval_cost, list_make1(param), NULL);
			if (eval_cost.startup + eval_cost.per_tuple >
				10 * cpu_operator_cost)
				goto fail;

			/*
			 * Check volatility last since this is more expensive than the
			 * above tests
			 */
			if (contain_volatile_functions(param))
				goto fail;
		}
		i++;
	}

	/*
	 * Whew --- we can make the substitution.  Copy the modified expression
	 * out of the temporary memory context, and clean up.
	 */
	MemoryContextSwitchTo(oldcxt);

	newexpr = copyObject(newexpr);

	MemoryContextDelete(mycxt);

	/*
	 * If the result is of a collatable type, force the result to expose the
	 * correct collation.  In most cases this does not matter, but it's
	 * possible that the function result is used directly as a sort key or in
	 * other places where we expect exprCollation() to tell the truth.
	 */
	if (OidIsValid(result_collid))
	{
		Oid			exprcoll = exprCollation(newexpr);

		if (OidIsValid(exprcoll) && exprcoll != result_collid)
		{
			CollateExpr *newnode = makeNode(CollateExpr);

			newnode->arg = (Expr *) newexpr;
			newnode->collOid = result_collid;
			newnode->location = -1;

			newexpr = (Node *) newnode;
		}
	}

	/*
	 * Since there is now no trace of the function in the plan tree, we must
	 * explicitly record the plan's dependency on the function.
	 */
	if (context->root)
		record_plan_function_dependency(context->root, funcid);

	/*
	 * Recursively try to simplify the modified expression.  Here we must add
	 * the current function to the context list of active functions.
	 */
	context->active_fns = lappend_oid(context->active_fns, funcid);
	newexpr = eval_const_expressions_mutator(newexpr, context);
	context->active_fns = list_delete_last(context->active_fns);

	error_context_stack = sqlerrcontext.previous;

	return (Expr *) newexpr;

	/* Here if func is not inlinable: release temp memory and return NULL */
fail:
	MemoryContextSwitchTo(oldcxt);
	MemoryContextDelete(mycxt);
	error_context_stack = sqlerrcontext.previous;

	return NULL;
}
/* …（同文件无关代码省略）… */
static Node *
substitute_actual_parameters(Node *expr, int nargs, List *args,
							 int *usecounts)
{
	substitute_actual_parameters_context context;

	context.nargs = nargs;
	context.args = args;
	context.usecounts = usecounts;

	return substitute_actual_parameters_mutator(expr, &context);
}
/* …（同文件无关代码省略）… */
static void
sql_inline_error_callback(void *arg)
{
	inline_error_callback_arg *callback_arg = (inline_error_callback_arg *) arg;
	int			syntaxerrposition;

	/* If it's a syntax error, convert to internal syntax error report */
	syntaxerrposition = geterrposition();
	if (syntaxerrposition > 0)
	{
		errposition(0);
		internalerrposition(syntaxerrposition);
		internalerrquery(callback_arg->prosrc);
	}

	errcontext("SQL function \"%s\" during inlining", callback_arg->proname);
}
/* …（同文件无关代码省略）… */
Expr *
evaluate_expr(Expr *expr, Oid result_type, int32 result_typmod,
			  Oid result_collation)
{
	EState	   *estate;
	ExprState  *exprstate;
	MemoryContext oldcontext;
	Datum		const_val;
	bool		const_is_null;
	int16		resultTypLen;
	bool		resultTypByVal;

	/*
	 * To use the executor, we need an EState.
	 */
	estate = CreateExecutorState();

	/* We can use the estate's working context to avoid memory leaks. */
	oldcontext = MemoryContextSwitchTo(estate->es_query_cxt);

	/* Make sure any opfuncids are filled in. */
	fix_opfuncids((Node *) expr);

	/*
	 * Prepare expr for execution.  (Note: we can't use ExecPrepareExpr
	 * because it'd result in recursively invoking eval_const_expressions.)
	 */
	exprstate = ExecInitExpr(expr, NULL);

	/*
	 * And evaluate it.
	 *
	 * It is OK to use a default econtext because none of the ExecEvalExpr()
	 * code used in this situation will use econtext.  That might seem
	 * fortuitous, but it's not so unreasonable --- a constant expression does
	 * not depend on context, by definition, n'est ce pas?
	 */
	const_val = ExecEvalExprSwitchContext(exprstate,
										  GetPerTupleExprContext(estate),
										  &const_is_null);

	/* Get info needed about result datatype */
	get_typlenbyval(result_type, &resultTypLen, &resultTypByVal);

	/* Get back to outer memory context */
	MemoryContextSwitchTo(oldcontext);

	/*
	 * Must copy result out of sub-context used by expression eval.
	 *
	 * Also, if it's varlena, forcibly detoast it.  This protects us against
	 * storing TOAST pointers into plans that might outlive the referenced
	 * data.  (makeConst would handle detoasting anyway, but it's worth a few
	 * extra lines here so that we can do the copy and detoast in one step.)
	 */
	if (!const_is_null)
	{
		if (resultTypLen == -1)
			const_val = PointerGetDatum(PG_DETOAST_DATUM_COPY(const_val));
		else
			const_val = datumCopy(const_val, resultTypByVal, resultTypLen);
	}

	/* Release all the junk we just created */
	FreeExecutorState(estate);

	/*
	 * Make the constant result node.
	 */
	return (Expr *) makeConst(result_type, result_typmod, result_collation,
							  resultTypLen,
							  const_val, const_is_null,
							  resultTypByVal);
}
