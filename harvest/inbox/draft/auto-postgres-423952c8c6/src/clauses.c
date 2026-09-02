// AUTO-DRAFT from postgres/postgres PR #ad4b80b6f49bcac519175fcb632aa94cdb7f6948
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

 *		*only* nullness of the particular Var, not any other conditions.
 *
 * This is just the single-clause case of find_forced_null_vars(), without
 * any allowance for AND conditions.  It's used by initsplan.c on individual
 * qual clauses.  The reason for not just applying find_forced_null_vars()
 * is that if an AND of an IS NULL clause with something else were to somehow
 * survive AND/OR flattening, initsplan.c might get fooled into discarding
 * the whole clause when only the IS NULL part of it had been proved redundant.
 */
Var *
find_forced_null_var(Node *node)
{
	if (node == NULL)
		return NULL;
	if (IsA(node, NullTest))
	{
		/* check for var IS NULL */
		NullTest   *expr = (NullTest *) node;

		if (expr->nulltesttype == IS_NULL)
		{
			Var		   *var = (Var *) expr->arg;

			/*
			 * A row-format test is accepted only on a whole-row Var, where
			 * its truth requires every column of the relation to be NULL.  On
			 * an ordinary composite-type column it is rejected, because the
			 * test does not force that column null: it is also true when the
			 * column is a non-null row whose fields are all NULL.
			 */
			if (var && IsA(var, Var) &&
				var->varlevelsup == 0 &&
				(!expr->argisrow || var->varattno == 0))
				return var;
		}
	}
	else if (IsA(node, BooleanTest))
	{
		/* var IS UNKNOWN is equivalent to var IS NULL */
		BooleanTest *expr = (BooleanTest *) node;

		if (expr->booltesttype == IS_UNKNOWN)
		{
			Var		   *var = (Var *) expr->arg;

			if (var && IsA(var, Var) &&
				var->varlevelsup == 0)
				return var;
		}
	}
	return NULL;
}
