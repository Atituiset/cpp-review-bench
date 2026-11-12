// AUTO-DRAFT from postgres/postgres PR #c1694b7b962c8b8031bc115bfca2f21e7a156654
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
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
