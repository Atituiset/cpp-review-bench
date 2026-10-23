// AUTO-DRAFT from postgres/postgres PR #3c5a4a867f2a4c72885de39306242661427f6e4f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
static void
findoprnd_recurse(QueryItem *ptr, uint32 *pos, int nnodes, bool *needcleanup)
{
	/* since this function recurses, it could be driven to stack overflow. */
	check_stack_depth();

	if (*pos >= nnodes)
		elog(ERROR, "malformed tsquery: operand not found");

	if (ptr[*pos].type == QI_VAL)
	{
		(*pos)++;
	}
	else if (ptr[*pos].type == QI_VALSTOP)
	{
		*needcleanup = true;	/* we'll have to remove stop words */
		(*pos)++;
	}
	else
	{
		Assert(ptr[*pos].type == QI_OPR);

		if (ptr[*pos].qoperator.oper == OP_NOT)
		{
			ptr[*pos].qoperator.left = 1;	/* fixed offset */
			(*pos)++;

			/* process the only argument */
			findoprnd_recurse(ptr, pos, nnodes, needcleanup);
		}
		else
		{
			QueryOperator *curitem = &ptr[*pos].qoperator;
			int			tmp = *pos; /* save current position */

			Assert(curitem->oper == OP_AND ||
				   curitem->oper == OP_OR ||
				   curitem->oper == OP_PHRASE);

			(*pos)++;

			/* process RIGHT argument */
			findoprnd_recurse(ptr, pos, nnodes, needcleanup);

			curitem->left = *pos - tmp; /* set LEFT arg's offset */

			/* process LEFT argument */
			findoprnd_recurse(ptr, pos, nnodes, needcleanup);
		}
	}
}
/* …（同文件无关代码省略）… */
static void
findoprnd(QueryItem *ptr, int size, bool *needcleanup)
{
	uint32		pos;

	*needcleanup = false;
	pos = 0;
	findoprnd_recurse(ptr, &pos, size, needcleanup);

	if (pos != size)
		elog(ERROR, "malformed tsquery: extra nodes");
}
/* …（同文件无关代码省略）… */
	 * Fill in the left-pointers. Checks that the tree is well-formed as a
	 * side-effect.
	 */
	findoprnd(item, size, &needcleanup);

	/* Can't have found any QI_VALSTOP nodes */
	Assert(!needcleanup);

	/* Copy operands to output struct */
	for (uint32 i = 0; i < size; i++)
