// AUTO-DRAFT from postgres/postgres PR #e5d25959cf8761c7dbdd7cfbb91338e94a40e8e2
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct ChangeContext
{
	/* The relation the changes are applied to. */
	Relation	cc_rel;

	/* Needed to update indexes of cc_rel. */
	ResultRelInfo *cc_rri;
	EState	   *cc_estate;

	/*
	 * The tuple descriptor to deform decoded tuples with; it must have the
	 * attmissingval values from the relation being repacked.
	 */
	TupleDesc	cc_tupdesc;

	/*
	 * Existing tuples to UPDATE and DELETE are located via this index. We
	 * keep the scankey in partially initialized state to avoid repeated work.
	 * sk_argument is completed on the fly.
	 */
	Relation	cc_ident_index;
	ScanKey		cc_ident_key;
	int			cc_ident_key_nentries;

	/* The latest column we need to deform to have the tuple identity */
	AttrNumber	cc_last_key_attno;

	/* Sequential number of the file containing the changes. */
	int			cc_file_seq;
} ChangeContext;
/* …（同文件无关代码省略）… */
static void apply_concurrent_delete(Relation rel, TupleTableSlot *slot);
static void restore_tuple(BufFile *file, Relation relation,
						  TupleTableSlot *slot);
static void adjust_toast_pointers(Relation relation, TupleTableSlot *dest,
								  TupleTableSlot *src);
static bool find_target_tuple(Relation rel, ChangeContext *chgcxt,
							  TupleTableSlot *locator,
							  TupleTableSlot *retrieved);
/* …（同文件无关代码省略）… */
				elog(ERROR, "could not find target tuple");

			/*
			 * If 'tup' contains TOAST pointers, they point to the old
			 * relation's toast. Copy the corresponding TOAST pointers for the
			 * new relation from the existing tuple. (The fact that we
			 * received a TOAST pointer here implies that the attribute hasn't
			 * changed.)
			 */
			adjust_toast_pointers(rel, spilled_tuple, ondisk_tuple);

			apply_concurrent_update(rel, spilled_tuple, ondisk_tuple, chgcxt);

/* …（同文件无关代码省略）… */
static void
apply_concurrent_update(Relation rel, TupleTableSlot *spilled_tuple,
						TupleTableSlot *ondisk_tuple,
						ChangeContext *chgcxt)
{
	LockTupleMode lockmode;
	TM_FailureData tmfd;
	TU_UpdateIndexes update_indexes;
	TM_Result	res;

	/*
	 * Carry out the update, skipping logical decoding for it.
	 */
	res = table_tuple_update(rel, &(ondisk_tuple->tts_tid), spilled_tuple,
							 GetCurrentCommandId(true),
							 TABLE_UPDATE_NO_LOGICAL,
							 InvalidSnapshot,
							 InvalidSnapshot,
							 false,
							 &tmfd, &lockmode, &update_indexes);
	if (res != TM_Ok)
		ereport(ERROR,
				errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
				errmsg("could not apply concurrent %s on relation \"%s\"",
					   "UPDATE", RelationGetRelationName(rel)));

	if (update_indexes != TU_None)
	{
		uint32		flags = EIIT_IS_UPDATE;

		if (update_indexes == TU_Summarizing)
			flags |= EIIT_ONLY_SUMMARIZING;
		ExecInsertIndexTuples(chgcxt->cc_rri,
							  chgcxt->cc_estate,
							  flags,
							  spilled_tuple,
							  NIL, NULL);
	}

	pgstat_progress_incr_param(PROGRESS_REPACK_HEAP_TUPLES_UPDATED, 1);
}
/* …（同文件无关代码省略）… */
static void
apply_concurrent_delete(Relation rel, TupleTableSlot *slot)
{
	TM_Result	res;
	TM_FailureData tmfd;

	/*
	 * Delete tuple from the new heap, skipping logical decoding for it.
	 */
	res = table_tuple_delete(rel, &(slot->tts_tid),
							 GetCurrentCommandId(true),
							 TABLE_DELETE_NO_LOGICAL,
							 InvalidSnapshot, InvalidSnapshot,
							 false,
							 &tmfd);

	if (res != TM_Ok)
		ereport(ERROR,
				errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
				errmsg("could not apply concurrent %s on relation \"%s\"",
					   "DELETE", RelationGetRelationName(rel)));

	pgstat_progress_incr_param(PROGRESS_REPACK_HEAP_TUPLES_DELETED, 1);
}
/* …（同文件无关代码省略）… */
static void
restore_tuple(BufFile *file, Relation relation, TupleTableSlot *slot)
{
	uint32		t_len;
	HeapTuple	tup;
	int			natt_ext;

	/* Read the tuple. */
	BufFileReadExact(file, &t_len, sizeof(t_len));
	tup = (HeapTuple) palloc(HEAPTUPLESIZE + t_len);
	tup->t_data = (HeapTupleHeader) ((char *) tup + HEAPTUPLESIZE);
	BufFileReadExact(file, tup->t_data, t_len);
	tup->t_len = t_len;
	ItemPointerSetInvalid(&tup->t_self);
	tup->t_tableOid = RelationGetRelid(relation);

	/*
	 * Put the tuple we read in a slot. This deforms it, so that we can hack
	 * the external attributes in place.
	 */
	ExecForceStoreHeapTuple(tup, slot, false);

	/*
	 * Next, read any attributes we stored separately into the tts_values
	 * array elements expecting them, if any.  This matches
	 * repack_store_change.
	 */
	BufFileReadExact(file, &natt_ext, sizeof(natt_ext));
	if (natt_ext > 0)
	{
		TupleDesc	desc = slot->tts_tupleDescriptor;

		for (int i = 0; i < desc->natts; i++)
		{
			CompactAttribute *attr = TupleDescCompactAttr(desc, i);
			varlena    *varlen;
			uint64		chunk_header;
			void	   *value;
			Size		varlensz;

			if (attr->attisdropped || attr->attlen != -1)
				continue;
			if (slot_attisnull(slot, i + 1))
				continue;
			varlen = (varlena *) DatumGetPointer(slot->tts_values[i]);
			if (!VARATT_IS_EXTERNAL_INDIRECT(varlen))
				continue;
			slot_getsomeattrs(slot, i + 1);

			BufFileReadExact(file, &chunk_header, VARHDRSZ);
			varlensz = VARSIZE_ANY(&chunk_header);

			value = palloc(varlensz);
			memcpy(value, &chunk_header, VARHDRSZ);
			BufFileReadExact(file, (char *) value + VARHDRSZ, varlensz - VARHDRSZ);

			slot->tts_values[i] = PointerGetDatum(value);
			natt_ext--;
			if (natt_ext < 0)
				elog(ERROR, "insufficient number of attributes stored separately");
		}

		if (natt_ext != 0)
			elog(ERROR,
				 "unexpected number of attributes stored separately (%d remaining)",
				 natt_ext);
	}
}

/*
 * Adjust 'dest' replacing any EXTERNAL_ONDISK toast pointers with the
 * corresponding ones from 'src'.
 */
static void
adjust_toast_pointers(Relation relation, TupleTableSlot *dest, TupleTableSlot *src)
{
	TupleDesc	desc = dest->tts_tupleDescriptor;

/* …（同文件无关代码省略）… */
		varlena    *varlena_dst;

		if (attr->attisdropped)
			continue;
		if (attr->attlen != -1)
			continue;
		if (
