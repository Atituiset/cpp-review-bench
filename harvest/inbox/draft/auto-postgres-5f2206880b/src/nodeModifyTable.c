// AUTO-DRAFT from postgres/postgres PR #b291954ea5a4855ded56c334859d7de9fea372eb
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	List	   *actionStates;
	TupleTableSlot *newslot = NULL;
	TupleTableSlot *rslot = NULL;
	EState	   *estate = context->estate;
	ExprContext *econtext = mtstate->ps.ps_ExprContext;
	bool		isNull;
/* …（同文件无关代码省略）… */
	 * or NOT MATCHED BY SOURCE tuple and UPDATE/DELETE actions.
	 */
	econtext->ecxt_scantuple = resultRelInfo->ri_oldTupleSlot;
	econtext->ecxt_innertuple = context->planSlot;
	econtext->ecxt_outertuple = NULL;

	/*
/* …（同文件无关代码省略）… */
								{
									actionStates = mergeActions[MERGE_WHEN_NOT_MATCHED_BY_SOURCE];

									/*
									 * If we have both NOT MATCHED BY SOURCE
									 * and NOT MATCHED BY TARGET actions (a
/* …（同文件无关代码省略）… */
												 false,
												 resultRelInfo->ri_oldTupleSlot,
												 newslot,
												 context->planSlot);
					break;

				case CMD_DELETE:
/* …（同文件无关代码省略）… */
												 true,
												 resultRelInfo->ri_oldTupleSlot,
												 NULL,
												 context->planSlot);
					break;

				case CMD_NOTHING:
/* …（同文件无关代码省略）… */
	 * Successfully executed an action or no qualifying action was found.
	 */
out:
	if (ItemPointerIsValid(&lockedtid))
		UnlockTuple(resultRelInfo->ri_RelationDesc, &lockedtid,
					InplaceUpdateTupleLock);
