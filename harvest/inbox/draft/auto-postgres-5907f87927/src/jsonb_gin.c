// AUTO-DRAFT from postgres/postgres PR #b28a2b2bfa50cb7aa3fcea069f0afad0ebcb6491
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
typedef enum JsonPathGinNodeType
{
	JSP_GIN_OR,
	JSP_GIN_AND,
	JSP_GIN_ENTRY,
} JsonPathGinNodeType;
/* …（同文件无关代码省略）… */
typedef struct JsonPathGinNode JsonPathGinNode;
/* …（同文件无关代码省略）… */
typedef struct JsonPathGinPathItem
{
	struct JsonPathGinPathItem *parent;
	Datum		keyName;		/* key name (for '.key' path item) or NULL */
	JsonPathItemType type;		/* type of jsonpath item */
} JsonPathGinPathItem;
/* …（同文件无关代码省略）… */
typedef union JsonPathGinPath
{
	JsonPathGinPathItem *items; /* list of path items (jsonb_ops) */
	uint32		hash;			/* hash of the path (jsonb_path_ops) */
} JsonPathGinPath;
/* …（同文件无关代码省略）… */
typedef struct JsonPathGinContext JsonPathGinContext;
/* …（同文件无关代码省略）… */
static JsonPathGinNode *
make_jsp_expr_node(JsonPathGinNodeType type, int nargs)
{
	JsonPathGinNode *node = palloc(offsetof(JsonPathGinNode, args) +
								   sizeof(node->args[0]) * nargs);

	node->type = type;
	node->val.nargs = nargs;

	return node;
}
/* …（同文件无关代码省略）… */
static JsonPathGinNode *
make_jsp_expr_node_args(JsonPathGinNodeType type, List *args)
{
	JsonPathGinNode *node = make_jsp_expr_node(type, list_length(args));
	ListCell   *lc;
	int			i = 0;

	foreach(lc, args)
		node->args[i++] = lfirst(lc);

	return node;
}
/* …（同文件无关代码省略）… */
static JsonPathGinNode *
make_jsp_expr_node_binary(JsonPathGinNodeType type,
						  JsonPathGinNode *arg1, JsonPathGinNode *arg2)
{
	JsonPathGinNode *node = make_jsp_expr_node(type, 2);

	node->args[0] = arg1;
	node->args[1] = arg2;

	return node;
}
/* …（同文件无关代码省略）… */
static List *
extract_jsp_path_expr_nodes(JsonPathGinContext *cxt, JsonPathGinPath path,
							JsonPathItem *jsp, JsonbValue *scalar)
{
	JsonPathItem next;
	List	   *nodes = NIL;

	for (;;)
	{
		switch (jsp->type)
		{
			case jpiCurrent:
				break;

			case jpiFilter:
				{
					JsonPathItem arg;
					JsonPathGinNode *filter;

					jspGetArg(jsp, &arg);

					filter = extract_jsp_bool_expr(cxt, path, &arg, false);

					if (filter)
						nodes = lappend(nodes, filter);

					break;
				}

			default:
				if (!cxt->add_path_item(&path, jsp))

					/*
					 * Path is not supported by the index opclass, return only
					 * the extracted filter nodes.
					 */
					return nodes;
				break;
		}

		if (!jspGetNext(jsp, &next))
			break;

		jsp = &next;
	}

	/*
	 * Append nodes from the path expression itself to the already extracted
	 * list of filter nodes.
	 */
	return cxt->extract_nodes(cxt, path, scalar, nodes);
}
/* …（同文件无关代码省略）… */
 * Extract an expression node from one of following jsonpath path expressions:
 *   EXISTS(jsp)    (when 'scalar' is NULL)
 *   jsp == scalar  (when 'scalar' is not NULL).
 *
 * The current path (@) is passed in 'path'.
 */
static JsonPathGinNode *
extract_jsp_path_expr(JsonPathGinContext *cxt, JsonPathGinPath path,
					  JsonPathItem *jsp, JsonbValue *scalar)
{
	/* extract a list of nodes to be AND-ed */
	List	   *nodes = extract_jsp_path_expr_nodes(cxt, path, jsp, scalar);

	if (nodes == NIL)
		/* no nodes were extracted => full scan is needed for this path */
		return NULL;

	if (list_length(nodes) == 1)
		return linitial(nodes); /* avoid extra AND-node */

	/* construct AND-node for path with filters */
	return make_jsp_expr_node_args(JSP_GIN_AND, nodes);
}

/* Recursively extract nodes from the boolean jsonpath expression. */
static JsonPathGinNode *
extract_jsp_bool_expr(JsonPathGinContext *cxt, JsonPathGinPath path,
					  JsonPathItem *jsp, bool not)
{
	check_stack_depth();

	switch (jsp->type)
	{
		case jpiAnd:			/* expr && expr */
		case jpiOr:				/* expr || expr */
			{
				JsonPathItem arg;
				JsonPathGinNode *larg;
				JsonPathGinNode *rarg;
				JsonPathGinNodeType type;

				jspGetLeftArg(jsp, &arg);
				larg = extract_jsp_bool_expr(cxt, path, &arg, not);

				jspGetRightArg(jsp, &arg);
				rarg = extract_jsp_bool_expr(cxt, path, &arg, not);

				if (!larg || !rarg)
				{
					if (jsp->type == jpiOr)
						return NULL;

					return larg ? larg : rarg;
				}

				type = not ^ (jsp->type == jpiAnd) ? JSP_GIN_AND : JSP_GIN_OR;

				return make_jsp_expr_node_binary(type, larg, rarg);
			}

		case jpiNot:			/* !expr  */
			{
				JsonPathItem arg;

				jspGetArg(jsp, &arg);

				/* extract child expression inverting 'not' flag */
				return extract_jsp_bool_expr(cxt, path, &arg, !not);
			}

		case jpiExists:			/* EXISTS(path) */
			{
				JsonPathItem arg;

				if (not)
					return NULL;	/* NOT EXISTS is not supported */

				jspGetArg(jsp, &arg);

				return extract_jsp_path_expr(cxt, path, &arg, NULL);
			}

		case jpiNotEqual:

			/*
			 * 'not' == true case is not supported here because '!(path !=
			 * scalar)' is not equivalent to 'path == scalar' in the general
			 * case because of sequence comparison semantics: 'path == scalar'
			 * === 'EXISTS (path, @ == scalar)', '!(path != scalar)' ===
			 * 'FOR_ALL(path, @ == scalar)'. So, we should translate '!(path
			 * != scalar)' into GIN query 'path == scalar || EMPTY(path)', but
			 * 'EMPTY(path)' queries are not supported by the both jsonb
			 * opclasses.  However in strict mode we could omit 'EMPTY(path)'
			 * part if the path can return exactly one item (it does not
			 * contain wildcard accessors or item methods like .keyvalue()
			 * etc.).
			 */
			return NULL;

		case jpiEqual:			/* path == scalar */
			{
				JsonPathItem left_item;
				JsonPathItem right_item;
				JsonPathItem *path_item;
				JsonPathItem *scalar_item;
				JsonbValue	scalar;

				if (not)
					return NULL;

				jspGetLeftArg(jsp, &left_item);
				jspGetRightArg(jsp, &right_item);

				if (jspIsScalar(left_item.type))
				{
					scalar_item = &left_item;
					path_item = &right_item;
				}
				else if (jspIsScalar(right_item.type))
				{
					scalar_item = &right_item;
					path_item = &left_item;
				}
				else
					return NULL;	/* at least one operand should be a scalar */

				switch (scalar_item->type)
				{
					case jpiNull:
				
