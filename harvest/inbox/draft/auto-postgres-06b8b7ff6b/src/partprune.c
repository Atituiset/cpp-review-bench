// AUTO-DRAFT from postgres/postgres PR #eacee6b618718f9e5298df0629d9720fa4dd5e45
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "optimizer/appendinfo.h"
#include "optimizer/cost.h"
#include "optimizer/optimizer.h"
#include "optimizer/pathnode.h"
/* …（同文件无关代码省略）… */

			/*
			 * See if there are any exec Params.  If so, we can only use this
			 * expression during per-scan pruning.
			 */
			paramids = pull_exec_paramids(expr);
			if (!bms_is_empty(paramids))
/* …（同文件无关代码省略）… */
				if (context->target != PARTTARGET_EXEC)
					return PARTCLAUSE_UNSUPPORTED;
			}
			else
			{
				/* It's potentially usable, but mutable */
/* …（同文件无关代码省略）… */
static Bitmapset *
pull_exec_paramids(Expr *expr)
{
	Bitmapset  *result = NULL;

	(void) pull_exec_paramids_walker((Node *) expr, &result);

	return result;
}
/* …（同文件无关代码省略）… */
static bool
pull_exec_paramids_walker(Node *node, Bitmapset **context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Param))
	{
		Param	   *param = (Param *) node;

		if (param->paramkind == PARAM_EXEC)
			*context = bms_add_member(*context, param->paramid);
		return false;
	}
	return expression_tree_walker(node, pull_exec_paramids_walker, context);
}
