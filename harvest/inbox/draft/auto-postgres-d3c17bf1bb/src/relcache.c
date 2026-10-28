// AUTO-DRAFT from postgres/postgres PR #27417270ab4e929aa3731e55945872206c425f75
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static HeapTuple
ScanPgRelation(Oid targetRelId, bool indexOK, bool force_non_historic)
{
	HeapTuple	pg_class_tuple;
	Relation	pg_class_desc;
	SysScanDesc pg_class_scan;
	ScanKeyData key[1];
	Snapshot	snapshot = NULL;

	/*
	 * If something goes wrong during backend startup, we might find ourselves
	 * trying to read pg_class before we've selected a database.  That ain't
	 * gonna work, so bail out with a useful error message.  If this happens,
	 * it probably means a relcache entry that needs to be nailed isn't.
	 */
	if (!OidIsValid(MyDatabaseId))
		elog(FATAL, "cannot read pg_class without having selected a database");

	/*
	 * form a scan key
	 */
	ScanKeyInit(&key[0],
				Anum_pg_class_oid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(targetRelId));

	/*
	 * Open pg_class and fetch a tuple.  Force heap scan if we haven't yet
	 * built the critical relcache entries (this includes initdb and startup
	 * without a pg_internal.init file).  The caller can also force a heap
	 * scan by setting indexOK == false.
	 */
	pg_class_desc = table_open(RelationRelationId, AccessShareLock);

	/*
	 * The caller might need a tuple that's newer than what's visible to the
	 * historic snapshot; currently the only case requiring to do so is
	 * looking up the relfilenumber of non mapped system relations during
	 * decoding.
	 */
	if (force_non_historic)
		snapshot = RegisterSnapshot(GetNonHistoricCatalogSnapshot(RelationRelationId));

	pg_class_scan = systable_beginscan(pg_class_desc, ClassOidIndexId,
									   indexOK && criticalRelcachesBuilt,
									   snapshot,
									   1, key);

	pg_class_tuple = systable_getnext(pg_class_scan);

	/*
	 * Must copy tuple before releasing buffer.
	 */
	if (HeapTupleIsValid(pg_class_tuple))
		pg_class_tuple = heap_copytuple(pg_class_tuple);

	/* all done */
	systable_endscan(pg_class_scan);

	if (snapshot)
		UnregisterSnapshot(snapshot);

	table_close(pg_class_desc, AccessShareLock);

	return pg_class_tuple;
}
/* …（同文件无关代码省略）… */
static void
RelationInitPhysicalAddr(Relation relation)
{
	RelFileNumber oldnumber = relation->rd_locator.relNumber;

	/* these relations kinds never have storage */
	if (!RELKIND_HAS_STORAGE(relation->rd_rel->relkind))
		return;

	if (relation->rd_rel->reltablespace)
		relation->rd_locator.spcOid = relation->rd_rel->reltablespace;
	else
		relation->rd_locator.spcOid = MyDatabaseTableSpace;
	if (relation->rd_locator.spcOid == GLOBALTABLESPACE_OID)
		relation->rd_locator.dbOid = InvalidOid;
	else
		relation->rd_locator.dbOid = MyDatabaseId;

	if (relation->rd_rel->relfilenode)
	{
		/*
		 * Even if we are using a decoding snapshot that doesn't represent the
		 * current state of the catalog we need to make sure the filenode
		 * points to the current file since the older file will be gone (or
		 * truncated). The new file will still contain older rows so lookups
		 * in them will work correctly. This wouldn't work correctly if
		 * rewrites were allowed to change the schema in an incompatible way,
		 * but those are prevented both on catalog tables and on user tables
		 * declared as additional catalog tables.
		 */
		if (HistoricSnapshotActive()
			&& RelationIsAccessibleInLogicalDecoding(relation)
			&& IsTransactionState())
		{
			HeapTuple	phys_tuple;
			Form_pg_class physrel;

			phys_tuple = ScanPgRelation(RelationGetRelid(relation),
										RelationGetRelid(relation) != ClassOidIndexId,
										true);
			if (!HeapTupleIsValid(phys_tuple))
				elog(ERROR, "could not find pg_class entry for %u",
					 RelationGetRelid(relation));
			physrel = (Form_pg_class) GETSTRUCT(phys_tuple);

			relation->rd_rel->reltablespace = physrel->reltablespace;
			relation->rd_rel->relfilenode = physrel->relfilenode;
			heap_freetuple(phys_tuple);
		}

		relation->rd_locator.relNumber = relation->rd_rel->relfilenode;
	}
	else
	{
		/* Consult the relation mapper */
		relation->rd_locator.relNumber =
			RelationMapOidToFilenumber(relation->rd_id,
									   relation->rd_rel->relisshared);
		if (!RelFileNumberIsValid(relation->rd_locator.relNumber))
			elog(ERROR, "could not find relation mapping for relation \"%s\", OID %u",
				 RelationGetRelationName(relation), relation->rd_id);
	}

	/*
	 * For RelationNeedsWAL() to answer correctly on parallel workers, restore
	 * rd_firstRelfilelocatorSubid.  No subtransactions start or end while in
	 * parallel mode, so the specific SubTransactionId does not matter.
	 */
	if (IsParallelWorker() && oldnumber != relation->rd_locator.relNumber)
	{
		if (RelFileLocatorSkippingWAL(relation->rd_locator))
			relation->rd_firstRelfilelocatorSubid = TopSubTransactionId;
		else
			relation->rd_firstRelfilelocatorSubid = InvalidSubTransactionId;
	}
}
/* …（同文件无关代码省略）… */
	RelationInitPhysicalAddr(relation);

	/*
	 * For a non-system index, there are fields of the pg_index row that are
	 * allowed to change, so re-read that row and update the relcache entry.
	 * Most of the info derived from pg_index (such as support function lookup
	 * info) cannot change, and indeed the whole point of this routine is to
	 * update the relcache entry without clobbering that data; so wholesale
	 * replacement is not appropriate.
	 */
	if (!IsSystemRelation(relation))
	{
		HeapTuple	tuple;
		Form_pg_index index;
