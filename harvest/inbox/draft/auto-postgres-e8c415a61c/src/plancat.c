// AUTO-DRAFT from postgres/postgres PR #62e46c00c657fc2e092ceec0cd79d16035798afe
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	foreach_ptr(InferenceElem, elem, onconflict->arbiterElems)
	{
		Var		   *var;
		int			attno;

		/* we cannot also have a constraint name, per grammar */
		Assert(!OidIsValid(onconflict->constraint));

		if (!IsA(elem->expr, Var))
		{
			/* If not a plain Var, just shove it in inferElems for now */
			inferElems = lappend(inferElems, elem->expr);
			continue;
		}

		var = (Var *) elem->expr;
		attno = var->varattno;

		if (attno == 0)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("whole row unique index inference specifications are not supported")));

		inferAttrs = bms_add_member(inferAttrs,
									attno - FirstLowInvalidHeapAttributeNumber);
	}
/* …（同文件无关代码省略）… */
		Form_pg_index idxForm;
		Bitmapset  *indexedAttrs;
		List	   *idxExprs;
		List	   *predExprs;
		AttrNumber	natt;
		bool		match;
/* …（同文件无关代码省略）… */
		if (idxForm->indisexclusion)
			continue;

		/* Build BMS representation of plain (non expression) index attrs */
		indexedAttrs = NULL;
		for (natt = 0; natt < idxForm->indnkeyatts; natt++)
		{
			int			attno = idxRel->rd_index->indkey.values[natt];

			if (attno != 0)
				indexedAttrs = bms_add_member(indexedAttrs,
											  attno - FirstLowInvalidHeapAttributeNumber);
		}

		/* Non-expression attributes (if any) must match */
		if (!bms_equal(indexedAttrs, inferAttrs))
			continue;

		/* Expression attributes (if any) must match */
		idxExprs = RelationGetIndexExpressions(idxRel);
		if (idxExprs)
		{
			if (varno != 1)
				ChangeVarNodes((Node *) idxExprs, 1, varno, 0);

			idxExprs = (List *) eval_const_expressions(root, (Node *) idxExprs);
		}

		/* Check the arbiterElems against this index. */
		match = true;
		foreach_ptr(InferenceElem, elem, onconflict->arbiterElems)
/* …（同文件无关代码省略）… */
		 * indexes redundantly repeat the same attribute, or if attributes
		 * redundantly appear multiple times within an inference clause.
		 */
		if (list_difference(idxExprs, inferElems) != NIL)
			continue;

		predExprs = RelationGetIndexPredicate(idxRel);
/* …（同文件无关代码省略）… */
		}

		/* If one matching index att found, good enough -- return true */
		if (IsA(elem->expr, Var))
		{
			if (((Var *) elem->expr)->varattno == attno)
				return true;
		}
		else if (attno == 0)
		{
			Node	   *nattExpr = list_nth(idxExprs, (natt - 1) - nplain);
