// AUTO-DRAFT from postgres/postgres PR #b7eee09eab738e085d6b62d35788f542dd233877
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>


/*
 * sublink_testexpr_is_not_nullable: verify that testexpr of an ANY_SUBLINK
 * guarantees a non-null result, assuming the inner side is also non-null.
 *
 * To ensure the expression never returns NULL, we require both that the outer
 * expressions are provably non-nullable and that the operator itself is safe.
/* …（同文件无关代码省略）… */
 * multi-column ordering checks.  In all cases, we validate the operators and
 * the outer expressions.
 *
 * It is acceptable for this check not to be exhaustive.  We can err on the
 * side of conservatism: if we're not sure, it's okay to return FALSE.
 */
/* …（同文件无关代码省略）… */
{
	Node	   *testexpr = sublink->testexpr;
	List	   *outer_exprs = NIL;

	/* Punt if sublink is not in the expected format */
	if (sublink->subLinkType != ANY_SUBLINK || testexpr == NULL)
/* …（同文件无关代码省略）… */
			return false;

		outer_exprs = lappend(outer_exprs, linitial(opexpr->args));
	}
	else if (is_andclause(testexpr) || is_orclause(testexpr))
	{
/* …（同文件无关代码省略）… */
				return false;

			outer_exprs = lappend(outer_exprs, linitial(opexpr->args));
		}
	}
	else if (IsA(testexpr, RowCompareExpr))
/* …（同文件无关代码省略）… */
		}

		outer_exprs = list_concat(outer_exprs, rcexpr->largs);
	}
	else
	{
		/* Punt if other node types */
		return false;
	}

	/*
	 * Since the query hasn't yet been through expression preprocessing, we
	 * must apply flatten_join_alias_vars to the outer expressions to avoid
