// AUTO-DRAFT from postgres/postgres PR #708eb515812bf23ebc36a2c436eabe701b3c2f5d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>


/*
 * pull_paramids
 *		Returns a Bitmapset containing the paramids of all Params in 'expr'.
 */
Bitmapset *
pull_paramids(Expr *expr)
/* …（同文件无关代码省略）… */
static bool
pull_paramids_walker(Node *node, Bitmapset **context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Param))
	{
		Param	   *param = (Param *) node;

		*context = bms_add_member(*context, param->paramid);
		return false;
	}
	return expression_tree_walker(node, pull_paramids_walker, context);
}
