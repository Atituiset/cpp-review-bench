// AUTO-DRAFT from postgres/postgres PR #19db7e5b36e33c80a032e0e811176ad3b115b0ca
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	List	   *actionStates;
	TupleTableSlot *newslot = NULL;
	TupleTableSlot *rslot = NULL;
	TupleTableSlot *planSlot = context->planSlot;
	EState	   *estate = context->estate;
	ExprContext *econtext = mtstate->ps.ps_ExprContext;
	bool		isNull;
/* …（同文件无关代码省略）… */
	 * or NOT MATCHED BY SOURCE tuple and UPDATE/DELETE actions.
	 */
	econtext->ecxt_scantuple = resultRelInfo->ri_oldTupleSlot;
	econtext->ecxt_innertuple = planSlot;
	econtext->ecxt_outertuple = NULL;

	/*
/* …（同文件无关代码省略）… */
									 * contain the NOT MATCHED BY TARGET
									 * tuple, with non-NULL source values.
									 * Instead, build a new all-NULL plan slot
									 * for use locally, leaving
									 * context->planSlot unchanged for any
									 * subsequent NOT MATCHED BY TARGET
									 * actions.  We don't bother setting
									 * "junk" attributes, since they are no
									 * longer needed.
									 */
									planSlot =
										MakeSingleTupleTableSlot(context->planSlot->tts_tupleDescriptor,
																 &TTSOpsVirtual);
									ExecStoreAllNullTuple(planSlot);
									econtext->ecxt_innertuple = planSlot;

									/*
									 * If we have both NOT MATCHED BY SOURCE
/* …（同文件无关代码省略）… */
												 false,
												 resultRelInfo->ri_oldTupleSlot,
												 newslot,
												 planSlot);
					break;

				case CMD_DELETE:
/* …（同文件无关代码省略）… */
												 true,
												 resultRelInfo->ri_oldTupleSlot,
												 NULL,
												 planSlot);
					break;

				case CMD_NOTHING:
/* …（同文件无关代码省略）… */
	 * Successfully executed an action or no qualifying action was found.
	 */
out:
	if (planSlot != context->planSlot)
	{
		econtext->ecxt_innertuple = NULL;
		if (resultRelInfo->ri_projectReturning != NULL)
			resultRelInfo->ri_projectReturning->pi_exprContext->ecxt_outertuple = NULL;
		ExecDropSingleTupleTableSlot(planSlot);
	}

	if (ItemPointerIsValid(&lockedtid))
