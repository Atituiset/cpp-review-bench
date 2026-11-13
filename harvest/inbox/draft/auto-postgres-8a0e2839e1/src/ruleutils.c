// AUTO-DRAFT from postgres/postgres PR #8e27c9261c9dd6127e16ced998d37ed7a8cb2b7d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
typedef struct
{
	List	   *rtable;			/* List of RangeTblEntry nodes */
	List	   *rtable_names;	/* Parallel list of names for RTEs */
	List	   *rtable_columns; /* Parallel list of deparse_columns structs */
	List	   *subplans;		/* List of Plan trees for SubPlans */
	List	   *ctes;			/* List of CommonTableExpr nodes */
	AppendRelInfo **appendrels; /* Array of AppendRelInfo nodes, or NULL */
	char	   *ret_old_alias;	/* alias for OLD in RETURNING list */
	char	   *ret_new_alias;	/* alias for NEW in RETURNING list */
	/* Workspace for column alias assignment: */
	bool		unique_using;	/* Are we making USING names globally unique */
	List	   *using_names;	/* List of assigned names for USING columns */
	/* Remaining fields are used only when deparsing a Plan tree: */
	Plan	   *plan;			/* immediate parent of current expression */
	List	   *ancestors;		/* ancestors of plan */
	Plan	   *outer_plan;		/* outer subnode, or NULL if none */
	Plan	   *inner_plan;		/* inner subnode, or NULL if none */
	List	   *outer_tlist;	/* referent for OUTER_VAR Vars */
	List	   *inner_tlist;	/* referent for INNER_VAR Vars */
	List	   *index_tlist;	/* referent for INDEX_VAR Vars */
	/* Special namespace representing a function signature: */
	char	   *funcname;
	int			numargs;
	char	  **argnames;
} deparse_namespace;

/*
/* …（同文件无关代码省略）… */
static void
print_function_sqlbody(StringInfo buf, HeapTuple proctup)
{
	int			numargs;
	Oid		   *argtypes;
	char	  **argnames;
	char	   *argmodes;
	deparse_namespace dpns = {0};
	Datum		tmp;
	Node	   *n;

	dpns.funcname = pstrdup(NameStr(((Form_pg_proc) GETSTRUCT(proctup))->proname));
	numargs = get_func_arg_info(proctup,
								&argtypes, &argnames, &argmodes);
	dpns.numargs = numargs;
	dpns.argnames = argnames;

	tmp = SysCacheGetAttrNotNull(PROCOID, proctup, Anum_pg_proc_prosqlbody);
	n = stringToNode(TextDatumGetCString(tmp));
