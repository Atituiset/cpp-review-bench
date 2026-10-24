// AUTO-DRAFT from postgres/postgres PR #126976a8ffcfa47f40d3df09243ebf4606001912
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR

static TupleTableSlot *ExecHashJoinOuterGetTuple(PlanState *outerNode,
												 HashJoinState *hjstate,
												 uint32 *hashvalue);
static TupleTableSlot *ExecParallelHashJoinOuterGetTuple(PlanState *outerNode,
														 HashJoinState *hjstate,
														 uint32 *hashvalue);
static TupleTableSlot *ExecHashJoinGetSavedTuple(HashJoinState *hjstate,
												 BufFile *file,
												 uint32 *hashvalue,
/* …（同文件无关代码省略）… */
	HashJoinTable hashtable;
	TupleTableSlot *outerTupleSlot;
	uint32		hashvalue;
	int			batchno;
	ParallelHashJoinState *parallel_state;

/* …（同文件无关代码省略）… */
				if (parallel)
					outerTupleSlot =
						ExecParallelHashJoinOuterGetTuple(outerNode, node,
														  &hashvalue);
				else
					outerTupleSlot =
						ExecHashJoinOuterGetTuple(outerNode, node, &hashvalue);

				if (TupIsNull(outerTupleSlot))
				{
/* …（同文件无关代码省略）… */
				econtext->ecxt_outertuple = outerTupleSlot;
				node->hj_MatchedOuter = false;

				/*
				 * Find the corresponding bucket for this tuple in the main
				 * hash table or skew hash table.
/* …（同文件无关代码省略）… */
 * Returns a null slot if no more outer tuples (within the current batch).
 *
 * On success, the tuple's hash value is stored at *hashvalue --- this is
 * either originally computed, or re-read from the temp file.
 */
static TupleTableSlot *
ExecHashJoinOuterGetTuple(PlanState *outerNode,
						  HashJoinState *hjstate,
						  uint32 *hashvalue)
{
	HashJoinTable hashtable = hjstate->hj_HashTable;
	int			curbatch = hashtable->curbatch;
/* …（同文件无关代码省略）… */

		while (!TupIsNull(slot))
		{
			bool		isnull;

			/*
			 * We have to compute the tuple's hash value.
			 */
/* …（同文件无关代码省略）… */

			*hashvalue = DatumGetUInt32(ExecEvalExprSwitchContext(hjstate->hj_OuterHash,
																  econtext,
																  &isnull));

			if (!isnull)
			{
				/* normal case with a non-null join key */
				/* remember outer relation is not empty for possible rescan */
/* …（同文件无关代码省略）… */
			}
			else if (hjstate->hj_KeepNullTuples)
			{
				/* null join key, but we must save tuple to be emitted later */
				if (hjstate->hj_NullOuterTupleStore == NULL)
					hjstate->hj_NullOuterTupleStore = ExecHashBuildNullTupleStore(hashtable);
				tuplestore_puttupleslot(hjstate->hj_NullOuterTupleStore, slot);
			}

			/*
/* …（同文件无关代码省略）… */
										 hashvalue,
										 hjstate->hj_OuterTupleSlot);
		if (!TupIsNull(slot))
			return slot;
	}

	/* End of this batch */
/* …（同文件无关代码省略）… */
static TupleTableSlot *
ExecParallelHashJoinOuterGetTuple(PlanState *outerNode,
								  HashJoinState *hjstate,
								  uint32 *hashvalue)
{
	HashJoinTable hashtable = hjstate->hj_HashTable;
	int			curbatch = hashtable->curbatch;
/* …（同文件无关代码省略）… */

		while (!TupIsNull(slot))
		{
			bool		isnull;

			ExprContext *econtext = hjstate->js.ps.ps_ExprContext;

			econtext->ecxt_outertuple = slot;
/* …（同文件无关代码省略）… */

			*hashvalue = DatumGetUInt32(ExecEvalExprSwitchContext(hjstate->hj_OuterHash,
																  econtext,
																  &isnull));

			if (!isnull)
			{
				/* normal case with a non-null join key */
				return slot;
			}
			else if (hjstate->hj_KeepNullTuples)
			{
				/* null join key, but we must save tuple to be emitted later */
				if (hjstate->hj_NullOuterTupleStore == NULL)
					hjstate->hj_NullOuterTupleStore = ExecHashBuildNullTupleStore(hashtable);
				tuplestore_puttupleslot(hjstate->hj_NullOuterTupleStore, slot);
			}

			/*
/* …（同文件无关代码省略）… */
			ExecForceStoreMinimalTuple(tuple,
									   hjstate->hj_OuterTupleSlot,
									   false);
			slot = hjstate->hj_OuterTupleSlot;
			return slot;
		}
/* …（同文件无关代码省略）… */
static TupleTableSlot *
ExecHashJoinGetSavedTuple(HashJoinState *hjstate,
						  BufFile *file,
						  uint32 *hashvalue,
						  TupleTableSlot *tupleSlot)
{
	uint32		header[2];
	size_t		nread;
	MinimalTuple tuple;

	/*
	 * We check for interrupts here because this is typically taken as an
	 * alternative code path to an ExecProcNode() call, which would include
	 * such a check.
	 */
	CHECK_FOR_INTERRUPTS();

	/*
	 * Since both the hash value and the MinimalTuple length word are uint32,
	 * we can read them both in one BufFileRead() call without any type
	 * cheating.
	 */
	nread = BufFileReadMaybeEOF(file, header, sizeof(header), true);
	if (nread == 0)				/* end of file */
	{
		ExecClearTuple(tupleSlot);
		return NULL;
	}
	*hashvalue = header[0];
	tuple = (MinimalTuple) palloc(header[1]);
	tuple->t_len = header[1];
	BufFileReadExact(file,
					 (char *) tuple + sizeof(uint32),
					 header[1] - sizeof(uint32));
	ExecForceStoreMinimalTuple(tuple, tupleSlot, true);
	return tupleSlot;
}
