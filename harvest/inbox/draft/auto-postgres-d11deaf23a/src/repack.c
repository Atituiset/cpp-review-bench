// AUTO-DRAFT from postgres/postgres PR #060eebf4179b40d8eacd56f2e6670a531f8b0733
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
											   Oid identIdx,
											   TransactionId frozenXid,
											   MultiXactId cutoffMulti);
static List *build_new_indexes(Relation NewHeap, Relation OldHeap, List *OldIndexes);
static void copy_index_constraints(Relation old_index, Oid new_index_id,
								   Oid new_heap_id);
static void copy_attribute_defaults(Oid old_heap_oid, Oid new_heap_oid);
/* …（同文件无关代码省略）… */
	 * are not necessarily dangerous, but can make user confused if the
	 * changes they do get lost due to REPACK.)
	 */
	ind_oids_new = build_new_indexes(NewHeap, OldHeap, ind_oids_old);

	/*
	 * The identity index in the new relation appears in the same relative
/* …（同文件无关代码省略）… */
}

/*
 * Build indexes on NewHeap according to those on OldHeap.
 *
 * OldIndexes is the list of index OIDs on OldHeap. The contained indexes end
 * up locked using ShareUpdateExclusiveLock.
 *
 * A list of OIDs of the corresponding indexes created on NewHeap is
 * returned. The order of items does match, so we can use these arrays to swap
 * index storage.
 */
static List *
build_new_indexes(Relation NewHeap, Relation OldHeap, List *OldIndexes)
{
	List	   *result = NIL;

/* …（同文件无关代码省略）… */
static void
copy_index_constraints(Relation old_index, Oid new_index_id, Oid new_heap_id)
{
	ScanKeyData skey;
	Relation	rel;
	TupleDesc	desc;
	SysScanDesc scan;
	HeapTuple	tup;
	ObjectAddress objrel;

	rel = table_open(ConstraintRelationId, RowExclusiveLock);
	ObjectAddressSet(objrel, RelationRelationId, new_heap_id);

	/*
	 * Retrieve the constraints supported by the old index and create an
	 * identical one that points to the new index.
	 */
	ScanKeyInit(&skey,
				Anum_pg_constraint_conrelid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(old_index->rd_index->indrelid));
	scan = systable_beginscan(rel, ConstraintRelidTypidNameIndexId, true,
							  NULL, 1, &skey);
	desc = RelationGetDescr(rel);
	while (HeapTupleIsValid(tup = systable_getnext(scan)))
	{
		Form_pg_constraint conform = (Form_pg_constraint) GETSTRUCT(tup);
		Oid			oid;
		Datum		values[Natts_pg_constraint] = {0};
		bool		nulls[Natts_pg_constraint] = {0};
		bool		replaces[Natts_pg_constraint] = {0};
		HeapTuple	new_tup;
		ObjectAddress objcon;

		if (conform->conindid != RelationGetRelid(old_index))
			continue;

		oid = GetNewOidWithIndex(rel, ConstraintOidIndexId,
								 Anum_pg_constraint_oid);
		values[Anum_pg_constraint_oid - 1] = ObjectIdGetDatum(oid);
		replaces[Anum_pg_constraint_oid - 1] = true;
		values[Anum_pg_constraint_conrelid - 1] = ObjectIdGetDatum(new_heap_id);
		replaces[Anum_pg_constraint_conrelid - 1] = true;
		values[Anum_pg_constraint_conindid - 1] = ObjectIdGetDatum(new_index_id);
		replaces[Anum_pg_constraint_conindid - 1] = true;

		new_tup = heap_modify_tuple(tup, desc, values, nulls, replaces);

		/* Insert it into the catalog. */
		CatalogTupleInsert(rel, new_tup);

		/* Create a dependency so it's removed when we drop the new heap. */
		ObjectAddressSet(objcon, ConstraintRelationId, oid);
		recordDependencyOn(&objcon, &objrel, DEPENDENCY_AUTO);
	}
	systable_endscan(scan);

	table_close(rel, RowExclusiveLock);

	CommandCounterIncrement();
}
/* …（同文件无关代码省略）… */
static void
copy_attribute_defaults(Oid old_heap_oid, Oid new_heap_oid)
{
	ScanKeyData skey;
	Relation	rel;
	Relation	att_rel;
	SysScanDesc scan;
	HeapTuple	def_tup;
	ObjectAddress objrel;

	rel = table_open(AttrDefaultRelationId, RowExclusiveLock);
	att_rel = table_open(AttributeRelationId, RowExclusiveLock);

	ObjectAddressSet(objrel, RelationRelationId, new_heap_oid);

	ScanKeyInit(&skey,
				Anum_pg_attrdef_adrelid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(old_heap_oid));
	scan = systable_beginscan(rel, AttrDefaultIndexId, true,
							  NULL, 1, &skey);
	while (HeapTupleIsValid(def_tup = systable_getnext(scan)))
	{
		Form_pg_attrdef adform;
		Oid			oid;
		Datum		def_values[Natts_pg_attrdef];
		bool		def_nulls[Natts_pg_attrdef];
		bool		def_replaces[Natts_pg_attrdef] = {0};
		Datum		att_values[Natts_pg_attribute];
		bool		att_nulls[Natts_pg_attribute];
		bool		att_replaces[Natts_pg_attribute] = {0};
		HeapTuple	new_def_tup,
					att_tup,
					new_att_tup;
		ObjectAddress objad;

		adform = (Form_pg_attrdef) GETSTRUCT(def_tup);
		Assert(adform->adrelid == old_heap_oid);

		/*
		 * Insert a new tuple that's identical to the existing one, other than
		 * its OID and the relation it refers to.
		 */
		oid = GetNewOidWithIndex(rel, AttrDefaultOidIndexId,
								 Anum_pg_attrdef_oid);
		def_values[Anum_pg_attrdef_oid - 1] = ObjectIdGetDatum(oid);
		def_nulls[Anum_pg_attrdef_oid - 1] = false;
		def_replaces[Anum_pg_attrdef_oid - 1] = true;
		def_values[Anum_pg_attrdef_adrelid - 1] = ObjectIdGetDatum(new_heap_oid);
		def_nulls[Anum_pg_attrdef_adrelid - 1] = false;
		def_replaces[Anum_pg_attrdef_adrelid - 1] = true;
		new_def_tup = heap_modify_tuple(def_tup, RelationGetDescr(rel),
										def_values, def_nulls, def_replaces);
		CatalogTupleInsert(rel, new_def_tup);

		/* Set atthasdef for this attribute in the transient table */
		att_tup = SearchSysCache2(ATTNUM,
								  ObjectIdGetDatum(new_heap_oid),
								  ObjectIdGetDatum(adform->adnum));
		if (!HeapTupleIsValid(att_tup))
			elog(ERROR, "cache lookup failed for attribute %d of relation %u",
				 adform->adnum, new_heap_oid);
		att_values[Anum_pg_attribute_atthasdef - 1] = BoolGetDatum(true);
		att_nulls[Anum_pg_attribute_atthasdef - 1] = false;
		att_replaces[Anum_pg_attribute_atthasdef - 1] = true;
		new_att_tup = heap_modify_tuple(att_tup, RelationGetDescr(att_rel),
										att_values, att_nulls, att_replaces);
		CatalogTupleUpdate(att_rel, &new_att_tup->t_self, new_att_tup);
		ReleaseSysCache(att_tup);

		/* Add a pg_depend record so it's removed with the transient table */
		ObjectAddressSet(objad, AttrDefaultRelationId, oid);
		recordDependencyOn(&objad, &objrel, DEPENDENCY_AUTO);
	}
	systable_endscan(scan);
