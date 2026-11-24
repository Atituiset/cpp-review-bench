// AUTO-DRAFT from postgres/postgres PR #6a0a66b3e7ac394c499708b790effea9a564ef0d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR

#include "access/commit_ts.h"
#include "access/genam.h"
#include "access/tableam.h"
#include "catalog/heap.h"
#include "catalog/pg_am.h"
#include "catalog/pg_namespace.h"
#include "catalog/toasting.h"
#include "executor/executor.h"
#include "pgstat.h"
#include "replication/conflict.h"
#include "replication/worker_internal.h"
#include "storage/lmgr.h"
#include "utils/lsyscache.h"

/*
 * String representations for the supported conflict logging destinations.
/* …（同文件无关代码省略）… */
typedef struct ConflictLogColumnDef
{
	const char *attname;		/* Column name */
	Oid			atttypid;		/* Data type OID */
} ConflictLogColumnDef;
/* …（同文件无关代码省略）… */
 * type OID; the table is created in this column order by
 * create_conflict_log_table().
 *
 * The tuple/key columns (replica_identity, remote_tuple, local_conflicts) are
 * typed json rather than jsonb on purpose: they hold an exact audit snapshot
 * of the applied tuples and replica identity, and json preserves the verbatim
 * representation whereas jsonb would normalize it. Indexing them (jsonb's main
 * advantage) wouldn't help anyway, as the conflict log is looked up by its
 * scalar columns (relid, conflict_type, commit timestamp) while these json
 * columns are per-conflict payload to inspect, not search keys.
 */
static const ConflictLogColumnDef ConflictLogSchema[] = {
	{.attname = "relid", .atttypid = OIDOID},
/* …（同文件无关代码省略）… */
	{.attname = "remote_xid", .atttypid = XIDOID},
	{.attname = "remote_commit_lsn", .atttypid = LSNOID},
	{.attname = "remote_commit_ts", .atttypid = TIMESTAMPTZOID},
	{.attname = "remote_origin", .atttypid = TEXTOID},
	{.attname = "replica_identity_full", .atttypid = BOOLOID},
	{.attname = "replica_identity", .atttypid = JSONOID},
	{.attname = "remote_tuple", .atttypid = JSONOID},
	{.attname = "local_conflicts", .atttypid = JSONARRAYOID}
};

#define NUM_CONFLICT_ATTRS ((AttrNumber) lengthof(ConflictLogSchema))

static const char *const ConflictTypeNames[] = {
	[CT_INSERT_EXISTS] = "insert_exists",
	[CT_UPDATE_ORIGIN_DIFFERS] = "update_origin_differs",
/* …（同文件无关代码省略）… */
	[CT_MULTIPLE_UNIQUE_CONFLICTS] = "multiple_unique_conflicts"
};

static int	errcode_apply_conflict(ConflictType type);
static void errdetail_apply_conflict(EState *estate,
									 ResultRelInfo *relinfo,
/* …（同文件无关代码省略）… */
						   TupleTableSlot *remoteslot, char **remote_desc,
						   TupleTableSlot *searchslot, char **search_desc,
						   Oid indexoid);
static char *build_index_value_desc(EState *estate, Relation localrel,
									TupleTableSlot *slot, Oid indexoid);

/*
 * Builds the TupleDesc for the conflict log table.
/* …（同文件无关代码省略）… */
 * This function is used to report a conflict while applying replication
 * changes.
 *
 * 'searchslot' should contain the tuple used to search the local row to be
 * updated or deleted.
 *
/* …（同文件无关代码省略）… */
 */
void
ReportApplyConflict(EState *estate, ResultRelInfo *relinfo, int elevel,
					ConflictType type, TupleTableSlot *searchslot,
					TupleTableSlot *remoteslot, List *conflicttuples)
{
	Relation	localrel = relinfo->ri_RelationDesc;
	StringInfoData err_detail;

	initStringInfo(&err_detail);

	/* Form errdetail message by combining conflicting tuples information. */
	foreach_ptr(ConflictTupleInfo, conflicttuple, conflicttuples)
		errdetail_apply_conflict(estate, relinfo, type, searchslot,
								 conflicttuple->slot, remoteslot,
								 conflicttuple->indexoid,
								 conflicttuple->xmin,
								 conflicttuple->origin,
								 conflicttuple->ts,
								 &err_detail);

	pgstat_report_subscription_conflict(MySubscription->oid, type);

	ereport(elevel,
			errcode_apply_conflict(type),
/* …（同文件无关代码省略）… */
				   RelationGetRelationName(localrel),
				   ConflictTypeNames[type]),
			errdetail_internal("%s", err_detail.data));
}

/*
/* …（同文件无关代码省略）… */
	relInfo->ri_onConflictArbiterIndexes = uniqueIndexes;
}

/*
 * Add SQLSTATE error code to the current conflict report.
 */
static int
errcode_apply_conflict(ConflictType type)
{
	switch (type)
	{
		case CT_INSERT_EXISTS:
		case CT_UPDATE_EXISTS:
		case CT_MULTIPLE_UNIQUE_CONFLICTS:
			return errcode(ERRCODE_UNIQUE_VIOLATION);
		case CT_UPDATE_ORIGIN_DIFFERS:
		case CT_UPDATE_MISSING:
		case CT_DELETE_ORIGIN_DIFFERS:
		case CT_UPDATE_DELETED:
		case CT_DELETE_MISSING:
			return errcode(ERRCODE_T_R_SERIALIZATION_FAILURE);
	}

	Assert(false);
	return 0;					/* silence compiler warning */
}
/* …（同文件无关代码省略）… */
static void
append_tuple_value_detail(StringInfo buf, List *tuple_values)
{
	bool		first = true;

	Assert(buf != NULL && tuple_values != NIL);

	foreach_ptr(char, tuple_value, tuple_values)
	{
		/*
		 * Skip if the value is NULL. This means the current user does not
		 * have enough permissions to see all columns in the table. See
		 * get_tuple_desc().
		 */
		if (!tuple_value)
			continue;

		/* standard SQL punctuation, not translated */
		if (!first)
			appendStringInfoString(buf, ", ");

		appendStringInfoString(buf, tuple_value);
		first = false;
	}
}
/* …（同文件无关代码省略）… */
static void
errdetail_apply_conflict(EState *estate, ResultRelInfo *relinfo,
						 ConflictType type, TupleTableSlot *searchslot,
						 TupleTableSlot *localslot, TupleTableSlot *remoteslot,
						 Oid indexoid, TransactionId localxmin,
						 ReplOriginId localorigin, TimestampTz localts,
						 StringInfo err_msg)
{
	StringInfoData err_detail;
	StringInfoData tuple_buf;
	char	   *origin_name;
	char	   *key_desc = NULL;
	char	   *local_desc = NULL;
	char	   *remote_desc = NULL;
	char	   *search_desc = NULL;

	/* Get key, replica identity, remote, and local value data */
	get_tuple_desc(estate, relinfo, type, &key_desc,
				   localslot, &local_desc,
				   remoteslot, &remote_desc,
				   searchslot, &search_desc,
				   indexoid);

	initStringInfo(&err_detail);
	initStringInfo(&tuple_buf);

	/* Construct a detailed message describing the type of conflict */
	switch (type)
	{
		case CT_INSERT_EXISTS:
		case CT_UPDATE_EXISTS:
		case CT_MULTIPLE_UNIQUE_CONFLICTS:
			Assert(OidIsValid(indexoid) &&
				   CheckRelationOidLockedByMe(indexoid, RowExclusiveLock, true));

			if (err_msg->len == 0)
			{
				append_tuple_value_detail(&tuple_buf,
										  list_make2(remote_desc, search_desc));

				if (tuple_buf.len)
					appendStringInfo(&err_detail, _("Could not apply remote change: %s.\n"),
									 tuple_buf.data);
				else
					appendStringInfo(&err_detail, _("Could not apply remote change.\n"));


				resetStringInfo(&tuple_buf);
			}

			append_tuple_value_detail(&tuple_buf,
									  list_make2(key_desc, local_desc));

			if (localts)
			{
				if (localorigin == InvalidReplOriginId)
				{
					if (tuple_buf.len)
						appendStringInfo(&err_detail, _("Key already exists in unique index \"%s\", modified locally in transaction %u at %s: %s."),
										 get_rel_name(indexoid),
										 localxmin, timestamptz_to_str(localts),
										 tuple_buf.data);
					else
						appendStringInfo(&err_detail, _("Key already exists in unique index \"%s\", modified locally in transaction %u at %s."),
										 get_rel_name(indexoid),
										 localxmin, timestamptz_to_str(localts));
				}
				else if (replorigin_by_oid(localorigin, true, &origin_name))
				{
					if (tuple_buf.len)
						appendStringInfo(&err_detail, _("Key already exists in unique index \"%s\", modified by origin \"%s\" in transaction %u at %s: %s."),
										 get_rel_name(indexoid), origin_name,
										 localxmin, timestamptz_to_str(localts),
										 tuple_buf.data);
					else
						appendStringInfo(&err_detail, _("Key already exists in unique index \"%s\", modified by origin \"%s\" in transaction %u at %s."),
										 get_rel_name(indexoid), origin_name,
										 localxmin, timestamptz_to_str(localts));
				}

				/*
				 * The origin that modified this row has been removed. This
				 * can happen if the origin was created by a different apply
				 * worker and its associated subscription and origin were
				 * dropped after updating the row, or if the origin was
				 * manually dropped by the user.
				 */
				else
				{
					if (tuple_buf.len)
						appendStringInfo(&err_detail, _("Key already exists in unique index \"%s\", modified by a non-existent origin in transaction %u at %s: %s."),
										 get_rel_name(indexoid),
										 localxmin, timestamptz_to_str(localts),
										 tuple_buf.data);
					else
						appendStringInfo(&err_detail, _("Key already exists in unique index \"%s\", modified by a non-existent origin in transaction %u at %s."),
										 get_rel_name(indexoid),
										 localxmin, timestamptz_to_str(localts));
				}
			}
			else
			{
				if (tuple_buf.len)
					appendStringInfo(&err_detail, _("Key already exists in unique index \"%s\", modified in transaction %u: %s."),
									 get_rel_name(indexoid), localxmin,
									 tuple_buf.data);
				else
					appendStringInfo(&err_detail, _("Key already exists in unique index \"%s\", modified in transaction %u."),
									 get_rel_name(indexoid), localxmin);
			}

			break;

		case CT_UPDATE_ORIGIN_DIFFERS:
			append_tuple_value_detail(&tuple_buf,
									  list_make3(local_desc, remote_desc,
												 search_desc));

			if (localorigin == InvalidReplOriginId)
			{
				if (tuple_buf.len)
					appendStringInfo(&err_detail, _("Updating the row that was modified locally in transaction %u at %s: %s."),
									 localxmin, timestamptz_to_str(localts),
									 tuple_buf.data);
				else
					appendStringInfo(&err_detail, _("Updating the row that was modified locally in transaction %u at %s."),
									 localxmin, timestamptz_to_str(localts));
			}
			else if (replorigin_by_oid(localorigin, true, &origin_name))
			{
				if (tuple_buf.len)
					appendStringInfo(&err_detail, _("Updating the row that was modified by a different origin \"%s\" in transaction %u at %s: %s."),
									 origin_name, localxmin,
									 timestamptz_to_str(localts),
									 tuple_buf.data);
				else
					appendStringInfo(&err_detail, _("Updating the row that was modified by a different origin \"%s\" in transaction %u at %s."),
									 origin_name, localxmin,
									 timestamptz_to_str(localts));
			}

			/* The origin that modified this row has been removed. */
			else
			{
				if (tuple_buf.len)
					appendStringInfo(&err_detail, _("Updating the row that was modified by a non-existent origin in transaction %u at %s: %s."),
									 localxmin, timestamptz_to_str(localts),
									 tuple_buf.data);
				else
					appendStringInfo(&err_detail, _("Updating the row that was modified by a non-existent origin in transaction %u at %s."),
									 localxmin, timestamptz_to_str(localts));
			}

			break;

		case CT_UPDATE_DELETED:
			append_tuple_value_detail(&tuple_buf,
									  list_make2(remote_desc, search_desc));

			if (tuple_buf.len)
				appendStringInfo(&err_detail, _("Could not find the row to be updated: %s.\n"),
								 tuple_buf.data);
			else
				appendStringInfo(&err_detail, _("Could not find the row to be updated.\n"));

			if (localts)
			{
				if (localorigin == InvalidReplOriginId)
					appendStringInfo(&err_detail, _("The row to be updated was deleted locally in transaction %u at %s."),
									 localxmin, timestamptz_to_str(localts));
				else if (replorigin_by_oid(localorigin, true, &origin_name))
					appendStringInfo(&err_detail, _("The row to be updated was deleted by a different origin \"%s\" in transaction %u at %s."),
									 origin_name, localxmin, timestamptz_to_str(localts));

				/* The origin that modified this row has been removed. */
				else
					appendStringInfo(&err_detail, _("The row to be updated was deleted by a non-existent origin in transaction %u at %s."),
									 localxmin, timestamptz_to_str(localts));
			}
			else
				appendStringInfoString(&err_detail, _("The row to be updated was deleted."));

			break;

		case CT_UPDATE_MISSING:
			append_tuple_value_detail(&tuple_buf,
									  list_make2(remote_desc, search_desc));

			if (tuple_buf.len)
				appendStringInfo(&err_detail, _("Could not find the row to be updated: %s."),
								 tuple_buf.data);
			else
				appendStringInfo(&err_detail, _("Could not find the row to be updated."));

			break;

		case CT_DELETE_ORIGIN_DIFFERS:
			append_tuple_value_detail(&tuple_buf,
									  list_make3(local_desc, remote_desc,
												 search_desc));

			if (localorigin == InvalidReplOriginId)
			{
				if (tuple_buf.len)
					appendStringInfo(&err_detail, _("Deleting the row that was modified locally in transaction %u at %s: %s."),
									 localxmin, timestamptz_to_str(localts),
									 tuple_buf.data);
				else
					appendStringInfo(&err_detail, _("Deleting the row that was modified locally in transaction %u at %s."),
									 localxmin, timestamptz_to_str(localts));
			}
			else if (replorigin_by_oid(localorigin, true, &origin_name))
			{
				if (tuple_buf.len)
					appendStringInfo(&err_detail, _("Deleting the row that was modified by a different origin \"%s\" in transaction %u at %s: %s."),
									 origin_name, localxmin,
									 timestamptz_to_str(localts),
									 tuple_buf.data);
				else
					appendStringInfo(&err_detail, _("Deleting the row that was modified by a different origin \"%s\" in transaction %u at %s."),
									 origin_name, localxmin,
									 timestamptz_to_str(localts));
			}

			/* The origin that modified this row has been removed. */
			else
			{
				if (tuple_buf.len)
					appendStringInfo(&err_detail, _("Deleting the row that was modified by a non-existent origin in transaction %u at %s: %s."),
									 localxmin, timestamptz_to_str(localts),
									 tuple_buf.data);
				else
					appendStringInfo(&err_detail, _("Deleting the row that was modified by a non-existent origin in transaction %u at %s."),
									 localxmin, timestamptz_to_str(localts));
			}

			break;

		case CT_DELETE_MISSING:
			append_tuple_value_detail(&tuple_buf,
									  list_make1(search_desc));

			if (tuple_buf.len)
				appendStringInfo(&err_detail, _("Could not find the row to be deleted: %s."),
								 tuple_buf.data);
			else
				appendStringInfo(&err_detail, _("Could not find the row to be deleted."));

			break;
	}

	Assert(err_detail.len > 0);

	/*
	 * Insert a blank line to visually separate the new detail line from the
	 * existing ones.
	 */
	if (err_msg->len > 0)
		appendStringInfoChar(err_msg, '\n');

	appendStringInfoString(err_msg, err_detail.data);
}
/* …（同文件无关代码省略）… */
static void
get_tuple_desc(EState *estate, ResultRelInfo *relinfo, ConflictType type,
			   char **key_desc,
			   TupleTableSlot *localslot, char **local_desc,
			   TupleTableSlot *remoteslot, char **remote_desc,
			   TupleTableSlot *searchslot, char **search_desc,
			   Oid indexoid)
{
	Relation	localrel = relinfo->ri_RelationDesc;
	Oid			relid = RelationGetRelid(localrel);
	TupleDesc	tupdesc = RelationGetDescr(localrel);
	char	   *desc = NULL;

	Assert((localslot && local_desc) || (remoteslot && remote_desc) ||
		   (searchslot && search_desc));

	/*
	 * Report the conflicting key values in the case of a unique constraint
	 * violation.
	 */
	if (type == CT_INSERT_EXISTS || type == CT_UPDATE_EXISTS ||
		type == CT_MULTIPLE_UNIQUE_CONFLICTS)
	{
		Assert(OidIsValid(indexoid) && localslot);

		desc = build_index_value_desc(estate, localrel, localslot,
									  indexoid);

		if (desc)
			*key_desc = psprintf(_("key %s"), desc);
	}

	if (localslot)
	{
		/*
		 * The 'modifiedCols' only applies to the new tuple, hence we pass
		 * NULL for the local row.
		 */
		desc = ExecBuildSlotValueDescription(relid, localslot, tupdesc,
											 NULL, 64);

		if (desc)
			*local_desc = psprintf(_("local row %s"), desc);
	}

	if (remoteslot)
	{
		Bitmapset  *modifiedCols;

		/*
		 * Although logical replication doesn't maintain the bitmap for the
		 * columns being inserted, we still use it to create 'modifiedCols'
		 * for consistency with other calls to ExecBuildSlotValueDescription.
		 *
		 * Note that generated columns are formed locally on the subscriber.
		 */
		modifiedCols = bms_union(ExecGetInsertedCols(relinfo, estate),
								 ExecGetUpdatedCols(relinfo, estate));
		desc = ExecBuildSlotValueDescription(relid, remoteslot,
											 tupdesc, modifiedCols,
											 64);

		if (desc)
			*remote_desc = psprintf(_("remote row %s"), desc);
	}

	if (searchslot)
	{
		/*
		 * Note that while index other than replica identity may be used (see
		 * IsIndexUsableForReplicaIdentityFull for details) to find the tuple
		 * when applying update or delete, such an index scan may not result
		 * in a unique tuple and we still compare the complete tuple in such
		 * cases, thus such indexes are not used here.
		 *
		 * XXX This can disagree with the index the apply worker searched by,
		 * see FindReplTupleInLocalRel(). It may not even be one that
		 * ExecOpenIndices() locked.
		 */
		Oid			replica_index = GetRelationIdentityOrPK(localrel);

		Assert(type != CT_INSERT_EXISTS);

		/*
		 * If the table has a valid replica identity index, build the index
		 * key value string. Otherwise, construct the full tuple value for
		 * REPLICA IDENTITY FULL cases.
		 */
		if (OidIsValid(replica_index))
			desc = build_index_value_desc(estate, localrel, searchslot, replica_index);
		else
			desc = ExecBuildSlotValueDescription(relid, searchslot, tupdesc, NULL, 64);

		if (desc)
		{
			if (OidIsValid(replica_index))
				*search_desc = psprintf(_("replica identity %s"), desc);
			else
				*search_desc = psprintf(_("replica identity full %s"), desc);
		}
	}
}

/*
 * Helper functions to construct a string describing the contents of an index
 * entry. See BuildIndexValueDescription for details.
/* …（同文件无关代码省略）… */
static char *
build_index_value_desc(EState *estate, Relation localrel, TupleTableSlot *slot,
					   Oid indexoid)
{
	char	   *index_value;
	Relation	indexDesc;
	Datum		values[INDEX_MAX_KEYS];
	bool		isnull[INDEX_MAX_KEYS];
	TupleTableSlot *tableslot = slot;

	if (!tableslot)
		return NULL;

	Assert(CheckRelationOidLockedByMe(indexoid, RowExclusiveLock, true));

	indexDesc = index_open(indexoid, NoLock);

	/*
	 * If the slot is a virtual slot, copy it into a heap tuple slot as
	 * FormIndexDatum only works with heap tuple slots.
	 */
	if (TTS_IS_VIRTUAL(slot))
	{
		tableslot = table_slot_create(localrel, &estate->es_tupleTable);
		tableslot = ExecCopySlot(tableslot, slot);
	}

	/*
	 * Initialize ecxt_scantuple for potential use in FormIndexDatum when
	 * index expressions are present.
	 */
	GetPerTupleExprContext(estate)->ecxt_scantuple = tableslot;

	/*
	 * The values/nulls arrays passed to BuildIndexValueDescription should be
	 * the results of FormIndexDatum, which are the "raw" input to the index
	 * AM.
	 */
	FormIndexDatum(BuildIndexInfo(indexDesc), tableslot, estate, values, isnull);

	index_value = BuildIndexValueDescription(indexDesc, values, isnull);

	index_close(indexDesc, NoLock);

	return index_value;
}
