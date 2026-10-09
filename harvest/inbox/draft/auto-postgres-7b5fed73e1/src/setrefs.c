// AUTO-DRAFT from postgres/postgres PR #0f7dfd7936b9e99f9aeca00f52ab1692898ffff4
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

typedef enum
{
	NRM_EQUAL,					/* expect exact match of nullingrels */
	NRM_SUPERSET,				/* actual Var may have a superset of input */
} NullingRelsMatch;
/* …（同文件无关代码省略）… */
typedef struct
{
	int			varno;			/* RT index of Var */
	AttrNumber	varattno;		/* attr number of Var */
	AttrNumber	resno;			/* TLE position of Var */
	Bitmapset  *varnullingrels; /* Var's varnullingrels */
} tlist_vinfo;
/* …（同文件无关代码省略）… */
typedef struct
{
	List	   *tlist;			/* underlying target list */
	int			num_vars;		/* number of plain Var tlist entries */
	bool		has_ph_vars;	/* are there PlaceHolderVar entries? */
	bool		has_non_vars;	/* are there other entries? */
	tlist_vinfo vars[FLEXIBLE_ARRAY_MEMBER];	/* has num_vars entries */
} indexed_tlist;
/* …（同文件无关代码省略）… */
typedef struct
{
	PlannerInfo *root;
	int			rtoffset;
	double		num_exec;
} fix_scan_expr_context;
/* …（同文件无关代码省略）… */
typedef struct
{
	PlannerInfo *root;
	indexed_tlist *subplan_itlist;
	int			newvarno;
	int			rtoffset;
	double		num_exec;
} fix_upper_expr_context;
/* …（同文件无关代码省略）… */
#define NUM_EXEC_QUAL(parentplan)   ((parentplan)->plan_rows * 2.0)
/* …（同文件无关代码省略）… */
#define ISREGCLASSCONST(con) \
	(((con)->consttype == REGCLASSOID || (con)->consttype == OIDOID) && \
	 !(con)->constisnull)
/* …（同文件无关代码省略）… */
										int rtoffset);
static void set_hash_references(PlannerInfo *root, Plan *plan, int rtoffset);
static Relids offset_relid_set(Relids relids, int rtoffset);
static Node *fix_dummy_setop_vars_mutator(Node *node, int *first_child_relid);
static Node *fix_scan_expr(PlannerInfo *root, Node *node,
						   int rtoffset, double num_exec);
/* …（同文件无关代码省略）… */
	set_dummy_tlist_references((Plan *) aplan, rtoffset);

	aplan->apprelids = offset_relid_set(aplan->apprelids, rtoffset);

	/*
	 * Add PartitionPruneInfo, if any, to PlannerGlobal and update the index.
/* …（同文件无关代码省略）… */
	set_dummy_tlist_references((Plan *) mplan, rtoffset);

	mplan->apprelids = offset_relid_set(mplan->apprelids, rtoffset);

	/*
	 * Add PartitionPruneInfo, if any, to PlannerGlobal and update the index.
/* …（同文件无关代码省略）… */
static void
set_hash_references(PlannerInfo *root, Plan *plan, int rtoffset)
{
	Hash	   *hplan = (Hash *) plan;
	Plan	   *outer_plan = plan->lefttree;
	indexed_tlist *outer_itlist;

	/*
	 * Hash's hashkeys are used when feeding tuples into the hashtable,
	 * therefore have them reference Hash's outer plan (which itself is the
	 * inner plan of the HashJoin).
	 */
	outer_itlist = build_tlist_index(outer_plan->targetlist);
	hplan->hashkeys = (List *)
		fix_upper_expr(root,
					   (Node *) hplan->hashkeys,
					   outer_itlist,
					   OUTER_VAR,
					   rtoffset,
					   NUM_EXEC_QUAL(plan));

	/* Hash doesn't project */
	set_dummy_tlist_references(plan, rtoffset);

	/* Hash nodes don't have their own quals */
	Assert(plan->qual == NIL);
}
/* …（同文件无关代码省略）… */
static Relids
offset_relid_set(Relids relids, int rtoffset)
{
	/* If there's no offset to apply, we needn't make another set */
	if (rtoffset == 0)
		return relids;
	return bms_offset_members(relids, rtoffset);
}

/*
 * copyVar
 *		Copy a Var node.
/* …（同文件无关代码省略）… */
static inline Var *
copyVar(Var *var)
{
	Var		   *newvar = palloc_object(Var);

	*newvar = *var;
	return newvar;
}
/* …（同文件无关代码省略）… */
static void
fix_expr_common(PlannerInfo *root, Node *node)
{
	/* We assume callers won't call us on a NULL pointer */
	if (IsA(node, Aggref))
	{
		record_plan_function_dependency(root,
										((Aggref *) node)->aggfnoid);
	}
	else if (IsA(node, WindowFunc))
	{
		record_plan_function_dependency(root,
										((WindowFunc *) node)->winfnoid);
	}
	else if (IsA(node, FuncExpr))
	{
		record_plan_function_dependency(root,
										((FuncExpr *) node)->funcid);
	}
	else if (IsA(node, OpExpr))
	{
		set_opfuncid((OpExpr *) node);
		record_plan_function_dependency(root,
										((OpExpr *) node)->opfuncid);
	}
	else if (IsA(node, DistinctExpr))
	{
		set_opfuncid((OpExpr *) node);	/* rely on struct equivalence */
		record_plan_function_dependency(root,
										((DistinctExpr *) node)->opfuncid);
	}
	else if (IsA(node, NullIfExpr))
	{
		set_opfuncid((OpExpr *) node);	/* rely on struct equivalence */
		record_plan_function_dependency(root,
										((NullIfExpr *) node)->opfuncid);
	}
	else if (IsA(node, ScalarArrayOpExpr))
	{
		ScalarArrayOpExpr *saop = (ScalarArrayOpExpr *) node;

		set_sa_opfuncid(saop);
		record_plan_function_dependency(root, saop->opfuncid);

		if (OidIsValid(saop->hashfuncid))
			record_plan_function_dependency(root, saop->hashfuncid);

		if (OidIsValid(saop->negfuncid))
			record_plan_function_dependency(root, saop->negfuncid);
	}
	else if (IsA(node, Const))
	{
		Const	   *con = (Const *) node;

		/* Check for regclass reference */
		if (ISREGCLASSCONST(con))
			root->glob->relationOids =
				lappend_oid(root->glob->relationOids,
							DatumGetObjectId(con->constvalue));
	}
	else if (IsA(node, GroupingFunc))
	{
		GroupingFunc *g = (GroupingFunc *) node;
		AttrNumber *grouping_map = root->grouping_map;

		/* If there are no grouping sets, we don't need this. */

		Assert(grouping_map || g->cols == NIL);

		if (grouping_map)
		{
			ListCell   *lc;
			List	   *cols = NIL;

			foreach(lc, g->refs)
			{
				cols = lappend_int(cols, grouping_map[lfirst_int(lc)]);
			}

			Assert(!g->cols || equal(cols, g->cols));

			if (!g->cols)
				g->cols = cols;
		}
	}
}
/* …（同文件无关代码省略）… */
static Node *
fix_param_node(PlannerInfo *root, Param *p)
{
	if (p->paramkind == PARAM_MULTIEXPR)
	{
		int			subqueryid = p->paramid >> 16;
		int			colno = p->paramid & 0xFFFF;
		List	   *params;

		if (subqueryid <= 0 ||
			subqueryid > list_length(root->multiexpr_params))
			elog(ERROR, "unexpected PARAM_MULTIEXPR ID: %d", p->paramid);
		params = (List *) list_nth(root->multiexpr_params, subqueryid - 1);
		if (colno <= 0 || colno > list_length(params))
			elog(ERROR, "unexpected PARAM_MULTIE
