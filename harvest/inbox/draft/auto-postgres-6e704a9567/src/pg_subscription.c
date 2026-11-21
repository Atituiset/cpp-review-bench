// AUTO-DRAFT from postgres/postgres PR #72146c7db15773ba0660c7c7e0284e3b1116ae2f
RemoveSubscriptionRel(Oid subid, Oid relid)
{
	Relation	rel;
	TableScanDesc scan;  // <<< BUG ANCHOR
	ScanKeyData skey[2];
	HeapTuple	tup;
	int			nkeys = 0;

	rel = table_open(SubscriptionRelRelationId, RowExclusiveLock);

	if (OidIsValid(subid))
	{
		ScanKeyInit(&skey[nkeys++],
					Anum_pg_subscription_rel_srsubid,
					BTEqualStrategyNumber,
					F_OIDEQ,
					ObjectIdGetDatum(subid));
	}

	if (OidIsValid(relid))
	{
		ScanKeyInit(&skey[nkeys++],
					Anum_pg_subscription_rel_srrelid,
					BTEqualStrategyNumber,
					F_OIDEQ,
					ObjectIdGetDatum(relid));
	}

	/* Do the search and delete what we found. */
	scan = table_beginscan_catalog(rel, nkeys, skey);
	while (HeapTupleIsValid(tup = heap_getnext(scan, ForwardScanDirection)))
	{
		Form_pg_subscription_rel subrel;

/* …（同文件无关代码省略）… */

		CatalogTupleDelete(rel, &tup->t_self);
	}
	table_endscan(scan);

	table_close(rel, RowExclusiveLock);
}
