// AUTO-DRAFT from postgres/postgres PR #425daf545d9146e008220ae0b21415982220cd3f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
static HeapTuple
ExecCallTriggerFunc(TriggerData *trigdata,
					int tgindx,
					FmgrInfo *finfo,
					TriggerInstrumentation *instr,
					MemoryContext per_tuple_context)
{
	LOCAL_FCINFO(fcinfo, 0);
	PgStat_FunctionCallUsage fcusage;
	Datum		result;
	MemoryContext oldContext;

	/*
	 * Protect against code paths that may fail to initialize transition table
	 * info.
	 */
	Assert(((TRIGGER_FIRED_BY_INSERT(trigdata->tg_event) ||
			 TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event) ||
			 TRIGGER_FIRED_BY_DELETE(trigdata->tg_event)) &&
			TRIGGER_FIRED_AFTER(trigdata->tg_event) &&
			!(trigdata->tg_event & AFTER_TRIGGER_DEFERRABLE) &&
			!(trigdata->tg_event & AFTER_TRIGGER_INITDEFERRED)) ||
		   (trigdata->tg_oldtable == NULL && trigdata->tg_newtable == NULL));

	finfo += tgindx;

	/*
	 * We cache fmgr lookup info, to avoid making the lookup again on each
	 * call.
	 */
	if (finfo->fn_oid == InvalidOid)
		fmgr_info(trigdata->tg_trigger->tgfoid, finfo);

	Assert(finfo->fn_oid == trigdata->tg_trigger->tgfoid);

	/*
	 * If doing EXPLAIN ANALYZE, start charging time to this trigger.
	 */
	if (instr)
		InstrStartTrigger(instr + tgindx);

	/*
	 * Do the function evaluation in the per-tuple memory context, so that
	 * leaked memory will be reclaimed once per tuple. Note in particular that
	 * any new tuple created by the trigger function will live till the end of
	 * the tuple cycle.
	 */
	oldContext = MemoryContextSwitchTo(per_tuple_context);

	/*
	 * Call the function, passing no arguments but setting a context.
	 */
	InitFunctionCallInfoData(*fcinfo, finfo, 0,
							 InvalidOid, (Node *) trigdata, NULL);

	pgstat_init_function_usage(fcinfo, &fcusage);

	MyTriggerDepth++;
	PG_TRY();
	{
		result = FunctionCallInvoke(fcinfo);
	}
	PG_FINALLY();
	{
		MyTriggerDepth--;
	}
	PG_END_TRY();

	pgstat_end_function_usage(&fcusage, true);

	MemoryContextSwitchTo(oldContext);

	/*
	 * Trigger protocol allows function to return a null pointer, but NOT to
	 * set the isnull result flag.
	 */
	if (fcinfo->isnull)
		ereport(ERROR,
				(errcode(ERRCODE_E_R_I_E_TRIGGER_PROTOCOL_VIOLATED),
				 errmsg("trigger function %u returned null value",
						fcinfo->flinfo->fn_oid)));

	/*
	 * If doing EXPLAIN ANALYZE, stop charging time to this trigger, and count
	 * the firing of the trigger.
	 */
	if (instr)
		InstrStopTrigger(instr + tgindx, 1);

	return (HeapTuple) DatumGetPointer(result);
}
/* …（同文件无关代码省略）… */
typedef struct SetConstraintTriggerData
{
	Oid			sct_tgoid;
	bool		sct_tgisdeferred;
} SetConstraintTriggerData;
/* …（同文件无关代码省略）… */
typedef struct SetConstraintStateData
{
	bool		all_isset;
	bool		all_isdeferred;
	int			numstates;		/* number of trigstates[] entries in use */
	int			numalloc;		/* allocated size of trigstates[] */
	SetConstraintTriggerData trigstates[FLEXIBLE_ARRAY_MEMBER];
} SetConstraintStateData;
/* …（同文件无关代码省略）… */
typedef SetConstraintStateData *SetConstraintState;
/* …（同文件无关代码省略）… */
typedef uint32 TriggerFlags;
/* …（同文件无关代码省略）… */
#define AFTER_TRIGGER_OFFSET			0x07FFFFFF	/* must be low-order bits */
#define AFTER_TRIGGER_DONE				0x80000000
#define AFTER_TRIGGER_IN_PROGRESS		0x40000000
/* …（同文件无关代码省略）… */
#define AFTER_TRIGGER_FDW_REUSE			0x00000000
#define AFTER_TRIGGER_FDW_FETCH			0x20000000
#define AFTER_TRIGGER_1CTID				0x10000000
#define AFTER_TRIGGER_2CTID				0x30000000
#define AFTER_TRIGGER_CP_UPDATE			0x08000000
#define AFTER_TRIGGER_TUP_BITS			0x38000000
typedef struct AfterTriggerSharedData *AfterTriggerShared;
/* …（同文件无关代码省略）… */
typedef struct AfterTriggerSharedData
{
	TriggerEvent ats_event;		/* event type indicator, see trigger.h */
	Oid			ats_tgoid;		/* the trigger's ID */
	Oid			ats_relid;		/* the relation it's on */
	Oid			ats_rolid;		/* role to execute the trigger */
	CommandId	ats_firing_id;	/* ID for firing cycle */
	struct AfterTriggersTableData *ats_table;	/* transition table access */
	Bitmapset  *ats_modifiedcols;	/* modified columns */
} AfterTriggerSharedData;
/* …（同文件无关代码省略）… */
typedef struct AfterTriggerEventData *AfterTriggerEvent;
/* …（同文件无关代码省略）… */
typedef struct AfterTriggerEventData
{
	TriggerFlags ate_flags;		/* status bits and offset to shared data */
	ItemPointerData ate_ctid1;	/* inserted, deleted, or old updated tuple */
	ItemPointerData ate_ctid2;	/* new updated tuple */

	/*
	 * During a cross-partition update of a partitioned table, we also store
	 * the OIDs of source and destination partitions that are needed to fetch
	 * the old (ctid1) and the new tuple (ctid2) from, respectively.
	 */
	Oid			ate_src_part;
	Oid			ate_dst_part;
} AfterTriggerEventData;
/* …（同文件无关代码省略）… */
typedef struct AfterTriggerEventDataNoOids
{
	TriggerFlags ate_flags;
	ItemPointerData ate_ctid1;
	ItemPointerData ate_ctid2;
}			AfterTriggerEventDataNoOids;
/* …（同文件无关代码省略）… */
typedef struct AfterTriggerEventDataOneCtid
{
	TriggerFlags ate_flags;		/* status bits and offset to shared data */
	ItemPointerData ate_ctid1;	/* inserted, deleted, or old updated tuple */
}			AfterTriggerEventDataOneCtid;
/* …（同文件无关代码省略）… */
typedef struct AfterTriggerEventDataZeroCtids
{
	TriggerFlags ate_flags;		/* status bits and offset to shared data */
}			AfterTriggerEventDataZeroCtids;
/* …（同文件无关代码省略）… */
#define SizeofTriggerEvent(evt) \
	(((evt)->ate_flags & AFTER_TRIGGER_TUP_BITS) == AFTER_TRIGGER_CP_UPDATE ? \
	 sizeof(AfterTriggerEventData) : \
	 (((evt)->ate_flags & AFTER_TRIGGER_TUP_BITS) == AFTER_TRIGGER_2CTID ? \
	  sizeof(AfterTriggerEventDataNoOids) : \
	  (((evt)->ate_flags & AFTER_TRIGGER_TUP_BITS) == AFTER_TRIGGER_1CTID ? \
	   sizeof(AfterTriggerEventDataOneCtid) : \
	   sizeof(AfterTriggerEventDataZeroCtids))))
/* …（同文件无关代码省略）… */
#define GetTriggerSharedData(evt) \
	((AfterTriggerShared) ((char *) (evt) + ((evt)->ate_flags & AFTER_TRIGGER_OFFSET)))
/* …（同文件无关代码省略）… */
typedef struct AfterTriggerEventChunk
{
	struct AfterTriggerEventChunk *next;	/* list link */
	char	   *freeptr;		/* start of free space in chunk */
	char	   *endfree;		/* end of free space in chunk */
	char	   *endptr;			/* end of chunk */
	/* event data follows here */
} AfterTriggerEventChunk;
/* …（同文件无关代码省略）… */
#define CHUNK_DATA_START(cptr) ((char *) (cptr) + MAXALIGN(sizeof(AfterTriggerEventChunk)))
/* …（同文件无关代码省略）… */
typedef struct AfterTriggerEventList
{
	AfterTriggerEventChunk *head;
	AfterTriggerEventChunk *tail;
	char	   *tailfree;		/* freeptr of tail chunk */
} AfterTriggerEventList;
/* …（同文件无关代码省略）… */
typedef struct AfterTriggersTransData AfterTriggersTransData;
typedef struct AfterTriggersTableData AfterTriggersTableData;
/* …（同文件无关代码省略）… */
typedef struct AfterTriggersData
{
	CommandId	firing_counter; /* next firing ID to assign */
	SetConstraintState state;	/* the active S C state */
	AfterTriggerEventList events;	/* deferred-event list */
	MemoryContext event_cxt;	/* memory context for events, if any */

	/* per-query-level data: */
	AfterTriggersQueryData *query_stack;	/* array of structs shown below */
	int			query_depth;	/* current index in above array */
	int			maxquerydepth;	/* allocated len of above array */

	/* per-subtransaction-level data: */
	AfterTriggersTransData *trans_stack;	/* array of structs shown below */
	int			maxtransdepth;	/* allocated len of above array */

	List	   *batch_callbacks;	/* List of AfterTriggerCallbackItem; for
									 * deferred constraints */
	bool		firing_batch_callbacks; /* true when in
										 * FireAfterTriggerBatchCallbacks() */

	/*
	 * Incremented around the trigger-firing loops in AfterTriggerEndQuery,
	 * AfterTriggerFireDeferred, and AfterTriggerSetState.  Used by
	 * AfterTriggerIsActive() to signal that after-trigger firing is active.
	 */
	int			firing_depth;
} AfterTriggersData;

struct AfterTriggersQueryData
{
	AfterTriggerEventList events;	/* events pending from this query */
	Tuplestorestate *fdw_tuplestore;	/* foreign tuples for said events */
	List	   *tables;			/* list of AfterTriggersTableData, see below */
	List	   *batch_callbacks;	/* List of AfterTriggerCallbackItem */
};

struct AfterTriggersTransData
/* …（同文件无关代码省略）… */
	SetConstraintState state;	/* saved S C state, or NULL if not yet saved */
	AfterTriggerEventList events;	/* saved list pointer */
	int			query_depth;	/* saved query_depth */
	int			firing_depth;	/* saved firing_depth */
	bool		firing_batch_callbacks; /* saved firing_batch_callbacks */
	CommandId	firing_counter; /* saved firing_counter */
};

/* …（同文件无关代码省略）… */
	TupleTableSlot *storeslot;	/* for converting to tuplestore's format */
};

/* Entry in afterTriggers.batch_callbacks */
typedef struct AfterTriggerCallbackItem
{
	AfterTriggerBatchCallback callback;
	void	   *arg;
} AfterTriggerCallbackItem;

static AfterTriggersData afterTriggers;

static void AfterTriggerExecute(EState *estate,
/* …（同文件无关代码省略）… */
													Oid tgoid, bool tgisdeferred);
static void cancel_prior_stmt_triggers(Oid relid, CmdType cmdType, int tgevent);

static void FireAfterTriggerBatchCallbacks(List *callbacks);

/*
 * Get the FDW tuplestore for the current trigger query level, creating it
/* …（同文件无关代码省略）… */
static Tuplestorestate *
GetCurrentFDWTuplestore(void)
{
	Tuplestorestate *ret;

	ret = afterTriggers.query_stack[afterTriggers.query_depth].fdw_tuplestore;
	if (ret == NULL)
	{
		MemoryContext oldcxt;
		ResourceOwner saveResourceOwner;

		/*
		 * Make the tuplestore valid until end of subtransaction.  We really
		 * only need it until AfterTriggerEndQuery().
		 */
		oldcxt = MemoryContextSwitchTo(CurTransactionContext);
		saveResourceOwner = CurrentResourceOwner;
		CurrentResourceOwner = CurTransactionResourceOwner;

		ret = tuplestore_begin_heap(false, false, work_mem);

		CurrentResourceOwner = saveResourceOwner;
		MemoryContextSwitchTo(oldcxt);

		afterTriggers.query_stack[afterTriggers.query_depth].fdw_tuplestore = ret;
	}

	return ret;
}
/* …（同文件无关代码省略）… */
static bool
afterTriggerCheckState(AfterTriggerShared evtshared)
{
	Oid			tgoid = evtshared->ats_tgoid;
	SetConstraintState state = afterTriggers.state;
	int			i;

	/*
	 * For not-deferrable triggers (i.e. normal AFTER ROW triggers and
	 * constraints declared NOT DEFERRABLE), the state is always false.
	 */
	if ((evtshared->ats_event & AFTER_TRIGGER_DEFERRABLE) == 0)
		return false;

	/*
	 * If constraint state exists, SET CONSTRAINTS might have been executed
	 * either for this trigger or for all triggers.
	 */
	if (state != NULL)
	{
		/* Check for SET CONSTRAINTS for this specific trigger. */
		for (i = 0; i < state->numstates; i++)
		{
			if (state->trigstates[i].sct_tgoid == tgoid)
				return state->trigstates[i].sct_tgisdeferred;
		}

		/* Check for SET CONSTRAINTS ALL. */
		if (state->all_isset)
			return state->all_isdeferred;
	}

	/*
	 * Otherwise return the default state for the trigger.
	 */
	return ((evtshared->ats_event & AFTER_TRIGGER_INITDEFERRED) != 0);
}
/* …（同文件无关代码省略）… */
static Bitmapset *
afterTriggerCopyBitmap(Bitmapset *src)
{
	Bitmapset  *dst;
	MemoryContext oldcxt;

	if (src == NULL)
		return NULL;

	oldcxt = MemoryContextSwitchTo(afterTriggers.event_cxt);

	dst = bms_copy(src);

	MemoryContextSwitchTo(oldcxt);

	return dst;
}
/* …（同文件无关代码省略）… */
static void
afterTriggerAddEvent(AfterTriggerEventList *events,
					 AfterTriggerEvent event, AfterTriggerShared evtshared)
{
	Size		eventsize = SizeofTriggerEvent(event);
	Size		needed = eventsize + sizeof(AfterTriggerSharedData);
	AfterTriggerEventChunk *chunk;
	AfterTriggerShared newshared;
	AfterTriggerEvent newevent;

	/*
	 * If empty list or not enough room in the tail chunk, make a new chunk.
	 * We assume here that a new shared record will always be needed.
	 */
	chunk = events->tail;
	if (chunk == NULL ||
		chunk->endfree - chunk->freeptr < needed)
	{
		Size		chunksize;

		/* Create event context if we didn't already */
		if (afterTriggers.event_cxt == NULL)
			afterTriggers.event_cxt =
				AllocSetContextCreate(TopTransactionContext,
									  "AfterTriggerEvents",
									  ALLOCSET_DEFAULT_SIZES);

		/*
		 * Chunk size starts at 1KB and is allowed to increase up to 1MB.
		 * These numbers are fairly arbitrary, though there is a hard limit at
		 * AFTER_TRIGGER_OFFSET; else we couldn't link event records to their
		 * shared records using the available space in ate_flags.  Another
		 * constraint is that if the chunk size gets too huge, the search loop
		 * below would get slow given a (not too common) usage pattern with
		 * many distinct event types in a chunk.  Therefore, we double the
		 * preceding chunk size only if there weren't too many shared records
		 * in the preceding chunk; otherwise we halve it.  This gives us some
		 * ability to adapt to the actual usage pattern of the current query
		 * while still having large chunk sizes in typical usage.  All chunk
		 * sizes used should be MAXALIGN multiples, to ensure that the shared
		 * records will be aligned safely.
		 */
#define MIN_CHUNK_SIZE 1024
#define MAX_CHUNK_SIZE (1024*1024)

#if MAX_CHUNK_SIZE > (AFTER_TRIGGER_OFFSET+1)
#error MAX_CHUNK_SIZE must not exceed AFTER_TRIGGER_OFFSET
#endif

		if (chunk == NULL)
			chunksize = MIN_CHUNK_SIZE;
		else
		{
			/* preceding chunk size... */
			chunksize = chunk->endptr - (char *) chunk;
			/* check number of shared records in preceding chunk */
			if ((chunk->endptr - chunk->endfree) <=
				(100 * sizeof(AfterTriggerSharedData)))
				chunksize *= 2; /* okay, double it */
			else
				chunksize /= 2; /* too many shared records */
			chunksize = Min(chunksize, MAX_CHUNK_SIZE);
		}
		chunk = MemoryContextAlloc(afterTriggers.event_cxt, chunksize);
		chunk->next = NULL;
		chunk->freeptr = CHUNK_DATA_START(chunk);
		chunk->endptr = chunk->endfree = (char *) chunk + chunksize;
		Assert(chunk->endfree - chunk->freeptr >= needed);

		if (events->tail == NULL)
		{
			Assert(events->head == NULL);
			events->head = chunk;
		}
		else
			events->tail->next = chunk;
		events->tail = chunk;
		/* events->tailfree is now out of sync, but we'll fix it below */
	}

	/*
	 * Try to locate a matching shared-data record already in the chunk. If
	 * none, make a new one. The search begins with the most recently added
	 * record, since newer ones are most likely to match.
	 */
	for (newshared = (AfterTriggerShared) chunk->endfree;
		 (char *) newshared < chunk->endptr;
		 newshared++)
	{
		/* compare fields roughly by probability of them being different */
		if (newshared->ats_tgoid == evtshared->ats_tgoid &&
			newshared->ats_event == evtshared->ats_event &&
			newshared->ats_firing_id == 0 &&
			newshared->ats_table == evtshared->ats_table &&
			newshared->ats_relid == evtshared->ats_relid &&
			newshared->ats_rolid == evtshared->ats_rolid &&
			bms_equal(newshared->ats_modifiedcols,
					  evtshared->ats_modifiedcols))
			break;
	}
	if ((char *) newshared >= chunk->endptr)
	{
		newshared = ((AfterTriggerShared) chunk->endfree) - 1;
		*newshared = *evtshared;
		/* now we must make a suitably-long-lived copy of the bitmap */
		newshared->ats_modifiedcols = afterTriggerCopyBitmap(evtshared->ats_modifiedcols);
		newshared->ats_firing_id = 0;	/* just to be sure */
		chunk->endfree = (char *) newshared;
	}

	/* Insert the data */
	newevent = (AfterTriggerEvent) chunk->freeptr;
	memcpy(newevent, event, eventsize);
	/* ... and link the new event to its shared record */
	newevent->ate_flags &= ~AFTER_TRIGGER_OFFSET;
	newevent->ate_flags |= (char *) newshared - (char *) newevent;

	chunk->freeptr += eventsize;
	events->tailfree = chunk->freeptr;
}
/* …（同文件无关代码省略）… */
static void
afterTriggerFreeEventList(AfterTriggerEventList *events)
{
	AfterTriggerEventChunk *chunk;

	while ((chunk = events->head) != NULL)
	{
		events->head = chunk->next;
		pfree(chunk);
	}
	events->tail = NULL;
	events->tailfree = NULL;
}
/* …（同文件无关代码省略）… */
static void
AfterTriggerExecute(EState *estate,
					AfterTriggerEvent event,
					ResultRelInfo *relInfo,
					ResultRelInfo *src_relInfo,
					ResultRelInfo *dst_relInfo,
					TriggerDesc *trigdesc,
					FmgrInfo *finfo, TriggerInstrumentation *instr,
					MemoryContext per_tuple_context,
					TupleTableSlot *trig_tuple_slot1,
					TupleTableSlot *trig_tuple_slot2)
{
	Relation	rel = relInfo->ri_RelationDesc;
	Relation	src_rel = src_relInfo->ri_RelationDesc;
	Relation	dst_rel = dst_relInfo->ri_RelationDesc;
	AfterTriggerShared evtshared = GetTriggerSharedData(event);
	Oid			tgoid = evtshared->ats_tgoid;
	TriggerData LocTriggerData = {0};
	Oid			save_rolid;
	int			save_sec_context;
	HeapTuple	rettuple;
	int			tgindx;
	bool		should_free_trig = false;
	bool		should_free_new = false;

	/*
	 * Locate trigger in trigdesc.  It might not be present, and in fact the
	 * trigdesc could be NULL, if the trigger was dropped since the event was
	 * queued.  In that case, silently do nothing.
	 */
	if (trigdesc == NULL)
		return;
	for (tgindx = 0; tgindx < trigdesc->numtriggers; tgindx++)
	{
		if (trigdesc->triggers[tgindx].tgoid == tgoid)
		{
			LocTriggerData.tg_trigger = &(trigdesc->triggers[tgindx]);
			break;
		}
	}
	if (LocTriggerData.tg_trigger == NULL)
		return;

	/*
	 * If doing EXPLAIN ANALYZE, start charging time to this trigger. We want
	 * to include time spent re-fetching tuples in the trigger cost.
	 */
	if (instr)
		InstrStartTrigger(instr + tgindx);

	/*
	 * Fetch the required tuple(s).
	 */
	switch (event->ate_flags & AFTER_TRIGGER_TUP_BITS)
	{
		case AFTER_TRIGGER_FDW_FETCH:
			{
				Tuplestorestate *fdw_tuplestore = GetCurrentFDWTuplestore();

				if (!tuplestore_gettupleslot(fdw_tuplestore, true, false,
											 trig_tuple_slot1))
					elog(ERROR, "failed to fetch tuple1 for AFTER trigger");

				if ((evtshared->ats_event & TRIGGER_EVENT_OPMASK) ==
					TRIGGER_EVENT_UPDATE &&
					!tuplestore_gettupleslot(fdw_tuplestore, true, false,
											 trig_tuple_slot2))
					elog(ERROR, "failed to fetch tuple2 for AFTER trigger");
			}
			pg_fallthrough;
		case AFTER_TRIGGER_FDW_REUSE:

			/*
			 * Store tuple in the slot so that tg_trigtuple does not reference
			 * tuplestore memory.  (It is formally possible for the trigger
			 * function to queue trigger events that add to the same
			 * tuplestore, which can push other tuples out of memory.)  The
			 * distinction is academic, because we start with a minimal tuple
			 * that is stored as a heap tuple, constructed in different memory
			 * context, in the slot anyway.
			 */
			LocTriggerData.tg_trigslot = trig_tuple_slot1;
			LocTriggerData.tg_trigtuple =
				ExecFetchSlotHeapTuple(trig_tuple_slot1, true, &should_free_trig);

			if ((evtshared->ats_event & TRIGGER_EVENT_OPMASK) ==
				TRIGGER_EVENT_UPDATE)
			{
				LocTriggerData.tg_newslot = trig_tuple_slot2;
				LocTriggerData.tg_newtuple =
					ExecFetchSlotHeapTuple(trig_tuple_slot2, true, &should_free_new);
			}
			else
			{
				LocTriggerData.tg_newtuple = NULL;
			}
			break;

		default:
			if (ItemPointerIsValid(&(event->ate_ctid1)))
			{
				TupleTableSlot *src_slot = ExecGetTriggerOldSlot(estate,
																 src_relInfo);

				if (!table_tuple_fetch_row_version(src_rel,
												   &(event->ate_ctid1),
												   SnapshotAny,
												   src_slot))
					elog(ERROR, "failed to fetch tuple1 for AFTER trigger");

				/*
				 * Store the tuple fetched from the source partition into the
				 * target (root partitioned) table slot, converting if needed.
				 */
				if (src_relInfo != relInfo)
				{
					TupleConversionMap *map = ExecGetChildToRootMap(src_relInfo);

					LocTriggerData.tg_trigslot = ExecGetTriggerOldSlot(estate, relInfo);
					if (map)
					{
						execute_attr_map_slot(map->attrMap,
											  src_slot,
											  LocTriggerData.tg_trigslot);
					}
					else
						ExecCopySlot(LocTriggerData.tg_trigslot, src_slot);
				}
				else
					LocTriggerData.tg_trigslot = src_slot;
				LocTriggerData.tg_trigtuple =
					ExecFetchSlotHeapTuple(LocTriggerData.tg_trigslot, false, &should_free_trig);
			}
			else
			{
				LocTriggerData.tg_trigtuple = NULL;
			}

			/* don't touch ctid2 if not there */
			if (((event->ate_flags & AFTER_TRIGGER_TUP_BITS) == AFTER_TRIGGER_2CTID ||
				 (event->ate_flags & AFTER_TRIGGER_CP_UPDATE)) &&
				ItemPointerIsValid(&(event->ate_ctid2)))
			{
				TupleTableSlot *dst_slot = ExecGetTriggerNewSlot(estate,
																 dst_relInfo);

				if (!table_tuple_fetch_row_version(dst_rel,
												   &(event->ate_ctid2),
												   SnapshotAny,
												   dst_slot))
					elog(ERROR, "failed to fetch tuple2 for AFTER trigger");

				/*
				 * Store the tuple fetched from the destination partition into
				 * the target (root partitioned) table slot, converting if
				 * needed.
				 */
				if (dst_relInfo != relInfo)
				{
					TupleConversionMap *map = ExecGetChildToRootMap(dst_relInfo);

					LocTriggerData.tg_newslot = ExecGetTriggerNewSlot(estate, relInfo);
					if (map)
					{
						execute_attr_map_slot(map->attrMap,
											  dst_slot,
											  LocTriggerData.tg_newslot);
					}
					else
						ExecCopySlot(LocTriggerData.tg_newslot, dst_slot);
				}
				else
					LocTriggerData.tg_newslot = dst_slot;
				LocTriggerData.tg_newtuple =
					ExecFetchSlotHeapTuple(LocTriggerData.tg_newslot, false, &should_free_new);
			}
			else
			{
				LocTriggerData.tg_newtuple = NULL;
			}
	}

	/*
	 * Set up the tuplestore information to let the trigger have access to
	 * transition tables.  When we first make a transition table available to
	 * a trigger, mark it "closed" so that it cannot change anymore.  If any
	 * additional events of the same type get queued in the current trigger
	 * query level, they'll go into new transition tables.
	 */
	LocTriggerData.tg_oldtable = LocTriggerData.tg_newtable = NULL;
	if (evtshared->ats_table)
	{
		if (LocTriggerData.tg_trigger->tgoldtable)
		{
			LocTriggerData.tg_oldtable = evtshared->ats_table->old_tuplestore;
			evtshared->ats_table->closed = true;
		}

		if (LocTriggerData.tg_trigger->tgnewtable)
		{
			LocTriggerData.tg_newtable = evtshared->ats_table->new_tuplestore;
			evtshared->ats_table->closed = true;
		}
	}

	/*
	 * Setup the remaining trigger information
	 */
	LocTriggerData.type = T_TriggerData;
	LocTriggerData.tg_event =
		evtshared->ats_event & (TRIGGER_EVENT_OPMASK | TRIGGER_EVENT_ROW);
	LocTriggerData.tg_relation = rel;
	if (TRIGGER_FOR_UPDATE(LocTriggerData.tg_trigger->tgtype))
		LocTriggerData.tg_updatedcols = evtshared->ats_modifiedcols;

	MemoryContextReset(per_tuple_context);

	/*
	 * If necessary, become the role that was active when the trigger got
	 * queued.  Note that the role might have been dropped since the trigger
	 * was queued, but if that is a problem, we will get an error later.
	 * Checking here would still leave a race condition.
	 */
	GetUserIdAndSecContext(&save_rolid, &save_sec_context);
	if (save_rolid != evtshared->ats_rolid)
		SetUserIdAndSecContext(evtshared->ats_rolid,
							   save_sec_context | SECURITY_LOCAL_USERID_CHANGE);

	/*
	 * Call the trigger and throw away any possibly returned updated tuple.
	 * (Don't let ExecCallTriggerFunc measure EXPLAIN time.)
	 */
	rettuple = ExecCallTriggerFunc(&LocTriggerData,
								   tgindx,
								   finfo,
								   NULL,
								   per_tuple_context);
	if (rettuple != NULL &&
		rettuple != LocTriggerData.tg_trigtuple &&
		rettuple != LocTriggerData.tg_newtuple)
		heap_freetuple(rettuple);

	/* Restore the current role if necessary */
	if (save_rolid != evtshared->ats_rolid)
		SetUserIdAndSecContext(save_rolid, save_sec_context);

	/*
	 * Release resources
	 */
	if (should_free_trig)
		heap_freetuple(LocTriggerData.tg_trigtuple);
	if (should_free_new)
		heap_freetuple(LocTriggerData.tg_newtuple);

	/* don't clear slots' contents if foreign table */
	if (trig_tuple_slot1 == NULL)
	{
		if (LocTriggerData.tg_trigslot)
			ExecClearTuple(LocTriggerData.tg_trigslot);
		if (LocTriggerData.tg_newslot)
			ExecClearTuple(LocTriggerData.tg_newslot);
	}

	/*
	 * If doing EXPLAIN ANALYZE, stop charging time to this trigger, and count
	 * the firing of the trigger.
	 */
	if (instr)
		InstrStopTrigger(instr + tgindx, 1);
}
/* …（同文件无关代码省略）… */
static bool
afterTriggerMarkEvents(AfterTriggerEventList *events,
					   AfterTriggerEventList *move_list,
					   bool immediate_only)
{
	bool		found = false;
	bool		deferred_found = false;
	AfterTriggerEvent event;
	AfterTriggerEventChunk *chunk;

	for_each_event_chunk(event, chunk, *events)
	{
		AfterTriggerShared evtshared = GetTriggerSharedData(event);
		bool		defer_it = false;

		if (!(event->ate_flags &
			  (AFTER_TRIGGER_DONE | AFTER_TRIGGER_IN_PROGRESS)))
		{
			/*
			 * This trigger hasn't been called or scheduled yet. Check if we
			 * should call it now.
			 */
			if (immediate_only && afterTriggerCheckState(evtshared))
			{
				defer_it = true;
			}
			else
			{
				/*
				 * Mark it as to be fired in this firing cycle.
				 */
				evtshared->ats_firing_id = afterTriggers.firing_counter;
				event->ate_flags |= AFTER_TRIGGER_IN_PROGRESS;
				found = true;
			}
		}

		/*
		 * If it's deferred, move it to move_list, if requested.
		 */
		if (defer_it && move_list != NULL)
		{
			deferred_found = true;
			/* add it to move_list */
			afterTriggerAddEvent(move_list, event, evtshared);
			/* mark original copy "done" so we don't do it again */
			event->ate_flags |= AFTER_TRIGGER_DONE;
		}
	}

	/*
	 * We could allow deferred triggers if, before the end of the
	 * security-restricted operation, we were to verify that a SET CONSTRAINTS
	 * ... IMMEDIATE has fired all such triggers.  For now, don't bother.
	 */
	if (deferred_found && InSecurityRestrictedOperation())
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("cannot fire deferred trigger within security-restricted operation")));

	return found;
}
/* …（同文件无关代码省略）… */
static bool
afterTriggerInvokeEvents(AfterTriggerEventList *events,
						 CommandId firing_id,
						 EState *estate,
						 bool delete_ok)
{
	bool		all_fired = true;
	AfterTriggerEventChunk *chunk;
	MemoryContext per_tuple_context;
	bool		local_estate = false;
	ResultRelInfo *rInfo = NULL;
	Relation	rel = NULL;
	TriggerDesc *trigdesc = NULL;
	FmgrInfo   *finfo = NULL;
	TriggerInstrumentation *instr = NULL;
	TupleTableSlot *slot1 = NULL,
			   *slot2 = NULL;

	/* Make a local EState if need be */
	if (estate == NULL)
	{
		estate = CreateExecutorState();
		local_estate = true;
	}

	/* Make a per-tuple memory context for trigger function calls */
	per_tuple_context =
		AllocSetContextCreate(CurrentMemoryContext,
							  "AfterTriggerTupleContext",
							  ALLOCSET_DEFAULT_SIZES);

	for_each_chunk(chunk, *events)
	{
		AfterTriggerEvent event;
		bool		all_fired_in_chunk = true;

		for_each_event(event, chunk)
		{
			AfterTriggerShared evtshared = GetTriggerSharedData(event);

			/*
			 * Is it one for me to fire?
			 */
			if ((event->ate_flags & AFTER_TRIGGER_IN_PROGRESS) &&
				evtshared->ats_firing_id == firing_id)
			{
				ResultRelInfo *src_rInfo,
						   *dst_rInfo;

				/*
				 * So let's fire it... but first, find the correct relation if
				 * this is not the same relation as before.
				 */
				if (rel == NULL || RelationGetRelid(rel) != evtshared->ats_relid)
				{
					rInfo = ExecGetTriggerResultRel(estate, evtshared->ats_relid,
													NULL);
					rel = rInfo->ri_RelationDesc;
					/* Catch calls with insufficient relcache refcounting */
					Assert(!RelationHasReferenceCountZero(rel));
					trigdesc = rInfo->ri_TrigDesc;
					/* caution: trigdesc could be NULL here */
					finfo = rInfo->ri_TrigFunctions;
					instr = rInfo->ri_TrigInstrument;
					if (slot1 != NULL)
					{
						ExecDropSingleTupleTableSlot(slot1);
						ExecDropSingleTupleTableSlot(slot2);
						slot1 = slot2 = NULL;
					}
					if (rel->rd_rel->relkind == RELKIND_FOREIGN_TABLE)
					{
						slot1 = MakeSingleTupleTableSlot(rel->rd_att,
														 &TTSOpsMinimalTuple);
						slot2 = MakeSingleTupleTableSlot(rel->rd_att,
														 &TTSOpsMinimalTuple);
					}
				}

				/*
				 * Look up source and destination partition result rels of a
				 * cross-partition update event.
				 */
				if ((event->ate_flags & AFTER_TRIGGER_TUP_BITS) ==
					AFTER_TRIGGER_CP_UPDATE)
				{
					Assert(OidIsValid(event->ate_src_part) &&
						   OidIsValid(event->ate_dst_part));
					src_rInfo = ExecGetTriggerResultRel(estate,
														event->ate_src_part,
														rInfo);
					dst_rInfo = ExecGetTriggerResultRel(estate,
														event->ate_dst_part,
														rInfo);
				}
				else
					src_rInfo = dst_rInfo = rInfo;

				/*
				 * Fire it.  Note that the AFTER_TRIGGER_IN_PROGRESS flag is
				 * still set, so recursive examinations of the event list
				 * won't try to re-fire it.
				 */
				AfterTriggerExecute(estate, event, rInfo,
									src_rInfo, dst_rInfo,
									trigdesc, finfo, instr,
									per_tuple_context, slot1, slot2);

				/*
				 * Mark the event as done.
				 */
				event->ate_flags &= ~AFTER_TRIGGER_IN_PROGRESS;
				event->ate_flags |= AFTER_TRIGGER_DONE;
			}
			else if (!(event->ate_flags & AFTER_TRIGGER_DONE))
			{
				/* something remains to be done */
				all_fired = all_fired_in_chunk = false;
			}
		}

		/* Clear the chunk if delete_ok and nothing left of interest */
		if (delete_ok && all_fired_in_chunk)
		{
			chunk->freeptr = CHUNK_DATA_START(chunk);
			chunk->endfree = chunk->endptr;

			/*
			 * If it's last chunk, must sync event list's tailfree too.  Note
			 * that delete_ok must NOT be passed as true if there could be
			 * additional AfterTriggerEventList values pointing at this event
			 * list, since we'd fail to fix their copies of tailfree.
			 */
			if (chunk == events->tail)
				events->tailfree = chunk->freeptr;
		}
	}
	if (slot1 != NULL)
	{
		ExecDropSingleTupleTableSlot(slot1);
		ExecDropSingleTupleTableSlot(slot2);
	}

	/* Release working resources */
	MemoryContextDelete(per_tuple_context);

	if (local_estate)
	{
		ExecCloseResultRelations(estate);
		ExecResetTupleTable(estate->es_tupleTable, false);
		FreeExecutorState(estate);
	}

	return all_fired;
}
/* …（同文件无关代码省略）… */
static AfterTriggersTableData *
GetAfterTriggersTableData(Oid relid, CmdType cmdType)
{
	AfterTriggersTableData *table;
	AfterTriggersQueryData *qs;
	MemoryContext oldcxt;
	ListCell   *lc;

	/* At this level, cmdType should not be, eg, CMD_MERGE */
	Assert(cmdType == CMD_INSERT ||
		   cmdType == CMD_UPDATE ||
		   cmdType == CMD_DELETE);

	/* Caller should have ensured query_depth is OK. */
	Assert(afterTriggers.query_depth >= 0 &&
		   afterTriggers.query_depth < afterTriggers.maxquerydepth);
	qs = &afterTriggers.query_stack[afterTriggers.query_depth];

	foreach(lc, qs->tables)
	{
		table = (AfterTriggersTableData *) lfirst(lc);
		if (table->relid == relid && table->cmdType == cmdType &&
			!table->closed)
			return table;
	}

	oldcxt = MemoryContextSwitchTo(CurTransactionContext);

	table = palloc0_object(AfterTriggersTableData);
	table->relid = relid;
	table->cmdType = cmdType;
	qs->tables = lappend(qs->tables, table);

	MemoryContextSwitchTo(oldcxt);

	return table;
}
/* …（同文件无关代码省略）… */
	 */
	afterTriggers.firing_counter = (CommandId) 1;	/* mustn't be 0 */
	afterTriggers.query_depth = -1;
	afterTriggers.firing_depth = 0;
	afterTriggers.batch_callbacks = NIL;
	afterTriggers.firing_batch_callbacks = false;

	/*
	 * Verify that there is no leftover state remaining.  If these assertions
/* …（同文件无关代码省略）… */
	 */
	qs = &afterTriggers.query_stack[afterTriggers.query_depth];

	afterTriggers.firing_depth++;
	for (;;)
	{
		if (afterTriggerMarkEvents(&qs->events, &afterTriggers.events, true))
/* …（同文件无关代码省略）… */
			break;
	}

	/*
	 * Fire batch callbacks before releasing query-level storage and before
	 * decrementing query_depth.  Callbacks may do real work (index probes,
	 * error reporting).
	 *
	 * Recompute qs first: the loop above refreshes it after each
	 * afterTriggerInvokeEvents() call (see comment there), but the "all
	 * fired" break exits without doing so, leaving qs potentially stale here.
	 */
	qs = &afterTriggers.query_stack[afterTriggers.query_depth];
	FireAfterTriggerBatchCallbacks(qs->batch_callbacks);

	/* Release query-level-local storage, including tuplestores if any */
	AfterTriggerFreeQuery(&afterTriggers.query_stack[afterTriggers.query_depth]);

	afterTriggers.query_depth--;
	afterTriggers.firing_depth--;
}


/* …（同文件无关代码省略）… */
static void
AfterTriggerFreeQuery(AfterTriggersQueryData *qs)
{
	Tuplestorestate *ts;
	List	   *tables;
	ListCell   *lc;

	/* Drop the trigger events */
	afterTriggerFreeEventList(&qs->events);

	/* Drop FDW tuplestore if any */
	ts = qs->fdw_tuplestore;
	qs->fdw_tuplestore = NULL;
	if (ts)
		tuplestore_end(ts);

	/* Release per-table subsidiary storage */
	tables = qs->tables;
	foreach(lc, tables)
	{
		AfterTriggersTableData *table = (AfterTriggersTableData *) lfirst(lc);

		ts = table->old_tuplestore;
		table->old_tuplestore = NULL;
		if (ts)
			tuplestore_end(ts);
		ts = table->new_tuplestore;
		table->new_tuplestore = NULL;
		if (ts)
			tuplestore_end(ts);
		if (table->storeslot)
		{
			TupleTableSlot *slot = table->storeslot;

			table->storeslot = NULL;
			ExecDropSingleTupleTableSlot(slot);
		}
	}

	/*
	 * Now free the AfterTriggersTableData structs and list cells.  Reset list
	 * pointer first; if list_free_deep somehow gets an error, better to leak
	 * that storage than have an infinite loop.
	 */
	qs->tables = NIL;
	list_free_deep(tables);

	list_free_deep(qs->batch_callbacks);
	qs->batch_callbacks = NIL;
}


/* …（同文件无关代码省略）… */
	 * Run all the remaining triggers.  Loop until they are all gone, in case
	 * some trigger queues more for us to do.
	 */
	afterTriggers.firing_depth++;
	while (afterTriggerMarkEvents(events, NULL, false))
	{
		CommandId	firing_id = afterTriggers.firing_counter++;

		(void) afterTriggerInvokeEvents(events, firing_id, NULL, true);

		/*
		 * Flush any fast-path FK-check batches accumulated by the triggers
		 * just fired.  A batch callback runs user-supplied cast or equality
		 * functions, whose DML can queue further deferred trigger events.
		 * Flush inside the loop so afterTriggerMarkEvents() sees any such
		 * events on the next iteration and fires them; flushing after the
		 * loop would leave them unfired, silently skipping e.g. a deferred FK
		 * check and letting a violating row commit.  (The former "all fired"
		 * break is therefore gone: the loop now terminates only when
		 * afterTriggerMarkEvents() finds nothing left, including events
		 * queued by the flush.)
		 */
		FireAfterTriggerBatchCallbacks(afterTriggers.batch_callbacks);
	}

	afterTriggers.firing_depth--;

	/*
	 * We don't bother freeing the event list or batch_callbacks, since they
	 * will go away anyway (and more efficiently than via pfree) in
	 * AfterTriggerEndXact.
	 */

	if (snap_pushed)
/* …（同文件无关代码省略）… */

	/* No more afterTriggers manipulation until next transaction starts. */
	afterTriggers.query_depth = -1;

	afterTriggers.firing_depth = 0;

	list_free_deep(afterTriggers.batch_callbacks);
	afterTriggers.batch_callbacks = NIL;
	afterTriggers.firing_batch_callbacks = false;
}

/*
/* …（同文件无关代码省略）… */
	afterTriggers.trans_stack[my_level].state = NULL;
	afterTriggers.trans_stack[my_level].events = afterTriggers.events;
	afterTriggers.trans_stack[my_level].query_depth = afterTriggers.query_depth;
	afterTriggers.trans_stack[my_level].firing_depth = afterTriggers.firing_depth;
	afterTriggers.trans_stack[my_level].firing_batch_callbacks =
		afterTriggers.firing_batch_callbacks;
	afterTriggers.trans_stack[my_level].firing_counter = afterTriggers.firing_counter;
}

/* …（同文件无关代码省略）… */
			}
		}
	}

	/*
	 * Restore firing_depth and firing_batch_callbacks to their values at
	 * subtransaction start.  The matching decrement of firing_depth in
	 * AfterTriggerEndQuery()/AfterTriggerFireDeferred(), and the clearing of
	 * firing_batch_callbacks in FireAfterTriggerBatchCallbacks(), run after
	 * their loops and are not protected by PG_FINALLY.  A trigger or batch
	 * callback error caught by this subtransaction can therefore leave either
	 * one set; restoring the saved values unwinds only this subtransaction's
	 * firing.
	 *
	 * Restoring (rather than zeroing/clearing) matters because a
	 * subtransaction can begin and end while an outer query's triggers are
	 * firing -- for instance a batch callback whose user-supplied cast or
	 * equality function runs DML in a BEGIN ... EXCEPTION block.  There
	 * firing_depth is positive and firing_batch_callbacks is true; forcing
	 * them to 0/false would corrupt the outer firing
	 * (FireAfterTriggerBatchCallbacks() asserts firing_depth > 0, and
	 * clearing the guard would defeat its re-entrancy check).
	 */
	afterTriggers.firing_depth = afterTriggers.trans_stack[my_level].firing_depth;
	afterTriggers.firing_batch_callbacks =
		afterTriggers.trans_stack[my_level].firing_batch_callbacks;
}

/*
/* …（同文件无关代码省略）… */
		qs->events.tailfree = NULL;
		qs->fdw_tuplestore = NULL;
		qs->tables = NIL;
		qs->batch_callbacks = NIL;

		++init_depth;
	}
/* …（同文件无关代码省略）… */
		AfterTriggerEventList *events = &afterTriggers.events;
		bool		snapshot_set = false;

		afterTriggers.firing_depth++;
		while (afterTriggerMarkEvents(events, NULL, true))
		{
			CommandId	firing_id = afterTriggers.firing_counter++;
/* …（同文件无关代码省略）… */
				break;			/* all fired */
		}

		/*
		 * Flush any fast-path batches accumulated by the triggers just fired.
		 */
		FireAfterTriggerBatchCallbacks(afterTriggers.batch_callbacks);
		afterTriggers.firing_depth--;
		list_free_deep(afterTriggers.batch_callbacks);
		afterTriggers.batch_callbacks = NIL;

		if (snapshot_set)
			PopActiveSnapshot();
	}
/* …（同文件无关代码省略）… */
static void
cancel_prior_stmt_triggers(Oid relid, CmdType cmdType, int tgevent)
{
	AfterTriggersTableData *table;
	AfterTriggersQueryData *qs = &afterTriggers.query_stack[afterTriggers.query_depth];

	/*
	 * We keep this state in the AfterTriggersTableData that also holds
	 * transition tables for the relation + operation.  In this way, if we are
	 * forced to make a new set of transition tables because more tuples get
	 * entered after we've already fired triggers, we will allow a new set of
	 * statement triggers to get queued without canceling the old ones.
	 */
	table = GetAfterTriggersTableData(relid, cmdType);

	if (table->after_trig_done)
	{
		/*
		 * We want to start scanning from the tail location that existed just
		 * before we inserted any statement triggers.  But the events list
		 * might've been entirely empty then, in which case scan from the
		 * current head.
		 */
		AfterTriggerEvent event;
		AfterTriggerEventChunk *chunk;

		if (table->after_trig_events.tail)
		{
			chunk = table->after_trig_events.tail;
			event = (AfterTriggerEvent) table->after_trig_events.tailfree;
		}
		else
		{
			chunk = qs->events.head;
			event = NULL;
		}

		for_each_chunk_from(chunk)
		{
			if (event == NULL)
				event = (AfterTriggerEvent) CHUNK_DATA_START(chunk);
			for_each_event_from(event, chunk)
			{
				AfterTriggerShared evtshared = GetTriggerSharedData(event);

				/*
				 * Exit loop when we reach events that aren't AS triggers for
				 * the target relation.
				 */
				if (evtshared->ats_relid != relid)
					goto done;
				if ((evtshared->ats_event & TRIGGER_EVENT_OPMASK) != tgevent)
					goto done;
				if (!TRIGGER_FIRED_FOR_STATEMENT(evtshared->ats_event))
					goto done;
				if (!TRIGGER_FIRED_AFTER(evtshared->ats_event))
					goto done;
				/* OK, mark it DONE */
				event->ate_flags &= ~AFTER_TRIGGER_IN_PROGRESS;
				event->ate_flags |= AFTER_TRIGGER_DONE;
			}
			/* signal we must reinitialize event ptr for next chunk */
			event = NULL;
		}
	}
done:

	/* In any case, save current insertion point for next time */
	table->after_trig_done = true;
	table->after_trig_events = qs->events;
}
/* …（同文件无关代码省略）… */

	return tuple;
}

/*
 * RegisterAfterTriggerBatchCallback
 *		Register a function to be called when the current trigger-firing
 *		batch completes.
 *
 * Must be called from within a trigger function's execution context
 * (i.e., while afterTriggers state is active).
 *
 * The callback list is cleared after invocation, so the caller must
 * re-register for each new batch if needed.
 */
void
RegisterAfterTriggerBatchCallback(AfterTriggerBatchCallback callback,
								  void *arg)
{
	AfterTriggerCallbackItem *item;
	MemoryContext oldcxt;

	/*
	 * Allocate in TopTransactionContext so the item survives for the duration
	 * of the batch, which may span multiple trigger invocations.
	 *
	 * Must be called while afterTriggers is active; callbacks registered
	 * outside a trigger-firing context would never fire.
	 */
	Assert(afterTriggers.firing_depth > 0);
	Assert(!afterTriggers.firing_batch_callbacks);
	oldcxt = MemoryContextSwitchTo(TopTransactionContext);
	item = palloc_object(AfterTriggerCallbackItem);
	item->callback = callback;
	item->arg = arg;
	if (afterTriggers.query_depth >= 0)
	{
		AfterTriggersQueryData *qs =
			&afterTriggers.query_stack[afterTriggers.query_depth];

		qs->batch_callbacks = lappend(qs->batch_callbacks, item);
	}
	else
		afterTriggers.batch_callbacks =
			lappend(afterTriggers.batch_callbacks, item);
	MemoryContextSwitchTo(oldcxt);
}

/*
 * FireAfterTriggerBatchCallbacks
 *		Invoke all callbacks in the given list.
 *
 * Memory cleanup of the list and its items is handled by the caller
 * (AfterTriggerFreeQuery for query-level callbacks, AfterTriggerEndXact
 * for top-level deferred callbacks).
 */
static void
FireAfterTriggerBatchCallbacks(List *callbacks)
{
	ListCell   *lc;

	Assert(afterTriggers.firing_depth > 0);
	afterTriggers.firing_batch_callbacks = true;
	foreach(lc, callbacks)
	{
		AfterTriggerCallbackItem *item = lfirst(lc);

		item->callback(item->arg);
	}
	afterTriggers.firing_batch_callbacks = false;
}

/*
 * AfterTriggerIsActive
 *		Returns true if we're inside the after-trigger framework where
 *		registered batch callbacks will actually be invoked.
 *
 * This is false during validateForeignKeyConstraint(), which calls
 * RI trigger functions directly outside the after-trigger framework.
 */
bool
AfterTriggerIsActive(void)
{
	return afterTriggers.firing_depth > 0;
}

/*
 * AfterTriggerCurrentQueryDepth
 *		Return the current after-trigger query nesting depth.
 *
 * Lets a batch-callback registrant (e.g. the RI fast path) associate cached
 * state with the firing cycle that created it, so a nested cycle's callback
 * acts only on its own entries.  Returns -1 outside any query level.
 */
int
AfterTriggerCurrentQueryDepth(void)
{
	return afterTriggers.query_depth;
}
