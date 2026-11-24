// AUTO-DRAFT from postgres/postgres PR #6a0a66b3e7ac394c499708b790effea9a564ef0d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
static void
update_most_recent_deletion_info(TupleTableSlot *scanslot,
								 TransactionId oldestxmin,
								 TransactionId *delete_xid,
								 TimestampTz *delete_time,
								 ReplOriginId *delete_origin)
{
/* …（同文件无关代码省略）… */
	if (TransactionIdGetCommitTsData(xmax, &localts, &localorigin) &&
		TimestampDifferenceExceeds(*delete_time, localts, 0))
	{
		*delete_xid = xmax;
		*delete_time = localts;
		*delete_origin = localorigin;
	}
/* …（同文件无关代码省略）… */
RelationFindDeletedTupleInfoSeq(Relation rel, Oid identidxoid,
								TupleTableSlot *searchslot,
								TransactionId oldestxmin,
								TransactionId *delete_xid,
								ReplOriginId *delete_origin,
								TimestampTz *delete_time)
{
/* …（同文件无关代码省略）… */

	Assert(equalTupleDescs(desc, searchslot->tts_tupleDescriptor));

	*delete_xid = InvalidTransactionId;
	*delete_origin = InvalidReplOriginId;
	*delete_time = 0;

/* …（同文件无关代码省略）… */
									bool skipduplicates,
									TupleTableSlot *searchslot,
									TransactionId oldestxmin,
									TransactionId *delete_xid,
									ReplOriginId *delete_origin,
									TimestampTz *delete_time)
{
/* …（同文件无关代码省略）… */
	Assert(equalTupleDescs(desc, searchslot->tts_tupleDescriptor));
	Assert(OidIsValid(idxoid));

	*delete_xid = InvalidTransactionId;
	*delete_time = 0;
	*delete_origin = InvalidReplOriginId;

/* …（同文件无关代码省略）… */
							  &conflictslot))
		{
			ConflictTupleInfo *conflicttuple = palloc0_object(ConflictTupleInfo);

			conflicttuple->slot = conflictslot;
			conflicttuple->indexoid = uniqueidx;

			GetTupleTransactionInfo(conflictslot, &conflicttuple->xmin,
									&conflicttuple->origin, &conflicttuple->ts);

			conflicttuples = lappend(conflicttuples, conflicttuple);
		}
	}
/* …（同文件无关代码省略）… */
	if (conflicttuples)
		ReportApplyConflict(estate, resultRelInfo, ERROR,
							list_length(conflicttuples) > 1 ? CT_MULTIPLE_UNIQUE_CONFLICTS : type,
							searchslot, remoteslot, conflicttuples);
}

/*
