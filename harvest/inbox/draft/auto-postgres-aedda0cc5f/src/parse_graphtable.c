// AUTO-DRAFT from postgres/postgres PR #792094a5ce89f9953d913c835e4530e56b63026a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include "catalog/pg_propgraph_property.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "parser/parse_collate.h"
#include "parser/parse_expr.h"
#include "parser/parse_graphtable.h"
/* …（同文件无关代码省略）… */
static const char *
get_gep_kind_name(GraphElementPatternKind gepkind)
{
	switch (gepkind)
	{
		case VERTEX_PATTERN:
			return "vertex";
		case EDGE_PATTERN_LEFT:
			return "edge pointing left";
		case EDGE_PATTERN_RIGHT:
			return "edge pointing right";
		case EDGE_PATTERN_ANY:
			return "edge pointing any direction";
		case PAREN_EXPR:
			return "nested path pattern";
	}

	/*
	 * When a GraphElementPattern is constructed by the parser, it will set a
	 * value from the GraphElementPatternKind enum. But we may get here if the
	 * GraphElementPatternKind value stored in a catalog is corrupted.
	 */
	return "unknown";
}
/* …（同文件无关代码省略）… */
static Node *
transformLabelExpr(GraphTableParseState *gpstate, Node *labelexpr)
{
	Node	   *result;

	if (labelexpr == NULL)
		return NULL;

	check_stack_depth();

	switch (nodeTag(labelexpr))
	{
		case T_ColumnRef:
			{
				ColumnRef  *cref = (ColumnRef *) labelexpr;
				const char *labelname;
				Oid			labelid;
				GraphLabelRef *lref;

				Assert(list_length(cref->fields) == 1);
				labelname = strVal(linitial(cref->fields));

				labelid = GetSysCacheOid2(PROPGRAPHLABELNAME, Anum_pg_propgraph_label_oid, ObjectIdGetDatum(gpstate->graphid), CStringGetDatum(labelname));
				if (!labelid)
					ereport(ERROR,
							errcode(ERRCODE_UNDEFINED_OBJECT),
							errmsg("label \"%s\" does not exist in property graph \"%s\"", labelname, get_rel_name(gpstate->graphid)));

				lref = makeNode(GraphLabelRef);
				lref->labelid = labelid;
				lref->location = cref->location;

				result = (Node *) lref;
				break;
			}

		case T_BoolExpr:
			{
				BoolExpr   *be = (BoolExpr *) labelexpr;
				ListCell   *lc;
				List	   *args = NIL;

				foreach(lc, be->args)
				{
					Node	   *arg = (Node *) lfirst(lc);

					arg = transformLabelExpr(gpstate, arg);
					args = lappend(args, arg);
				}

				result = (Node *) makeBoolExpr(be->boolop, args, be->location);
				break;
			}

		default:
			/* should not reach here */
			elog(ERROR, "unsupported label expression node: %d", (int) nodeTag(labelexpr));
			result = NULL;		/* keep compiler quiet */
			break;
	}

	return result;
}
/* …（同文件无关代码省略）… */
static Node *
transformGraphElementPattern(ParseState *pstate, GraphElementPattern *gep)
{
	GraphTableParseState *gpstate = pstate->p_graph_table_pstate;

	if (gep->quantifier)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("element pattern quantifier is not supported")));

	Assert(!gpstate->cur_gep);

	gpstate->cur_gep = gep;

	gep->labelexpr = transformLabelExpr(gpstate, gep->labelexpr);

	gep->whereClause = transformExpr(pstate, gep->whereClause, EXPR_KIND_WHERE);

	/*
	 * Assign collations here for the reason mentioned in the prologue of
	 * transformGraphPattern().
	 */
	assign_expr_collations(pstate, gep->whereClause);

	gpstate->cur_gep = NULL;

	return (Node *) gep;
}
/* …（同文件无关代码省略）… */
static Node *
transformPathTerm(ParseState *pstate, List *path_term)
{
	List	   *result = NIL;
	GraphElementPattern *prev_gep = NULL;

	foreach_node(GraphElementPattern, gep, path_term)
	{
		if (gep->kind != VERTEX_PATTERN && !IS_EDGE_PATTERN(gep->kind))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("unsupported element pattern kind: \"%s\"", get_gep_kind_name(gep->kind)),
					 parser_errposition(pstate, gep->location)));

		if (IS_EDGE_PATTERN(gep->kind))
		{
			if (!prev_gep)
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("path pattern cannot start with an edge pattern"),
						 parser_errposition(pstate, gep->location)));
			else if (prev_gep->kind != VERTEX_PATTERN)
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("edge pattern must be preceded by a vertex pattern"),
						 parser_errposition(pstate, gep->location)));
		}
		else
		{
			if (prev_gep && !IS_EDGE_PATTERN(prev_gep->kind))
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("adjacent vertex patterns are not supported"),
						 parser_errposition(pstate, gep->location)));
		}

		result = lappend(result,
						 transformGraphElementPattern(pstate, gep));
		prev_gep = gep;
	}

	/* Path pattern should have at least one element pattern. */
	Assert(prev_gep);

	if (IS_EDGE_PATTERN(prev_gep->kind))
	{
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("path pattern cannot end with an edge pattern"),
				 parser_errposition(pstate, prev_gep->location)));
	}

	return (Node *) result;
}
/* …（同文件无关代码省略）… */
static Node *
transformPathPatternList(ParseState *pstate, List *path_pattern)
{
	List	   *result = NIL;
	GraphTableParseState *gpstate = pstate->p_graph_table_pstate;

	Assert(gpstate);

	/* Grammar doesn't allow empty path pattern list */
	Assert(list_length(path_pattern) > 0);

	/*
	 * We do not support multiple path patterns in one GRAPH_TABLE clause
	 * right now. But we may do so in future.
	 */
	if (list_length(path_pattern) != 1)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("multiple path patterns in one GRAPH_TABLE clause not supported")));

	/*
	 * Collect all the variables in the path pattern into the
	 * GraphTableParseState so that we can detect any non-local element
	 * variable references. We need to do this before transforming the path
	 * pattern so as to detect forward references to element variables in the
	 * WHERE clause of an element pattern.
	 */
	foreach_node(List, path_term, path_pattern)
	{
		foreach_node(GraphElementPattern, gep, path_term)
		{
			if (gep->variable)
				gpstate->variables = list_append_unique(gpstate->variables, makeString(pstrdup(gep->variable)));
		}
	}

	foreach_node(List, path_term, path_pattern)
		re
