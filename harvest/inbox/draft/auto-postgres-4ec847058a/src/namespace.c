// AUTO-DRAFT from postgres/postgres PR #0b776de09ed3e237181def135577321bdd31b548
static void
RemoveTempRelations(Oid tempNamespaceId)
{
	ObjectAddress object;
  // <<< BUG ANCHOR
	/*
	 * We want to get rid of everything in the target namespace, but not the
	 * namespace itself (deleting it only to recreate it later would be a
	 * waste of cycles).  Hence, specify SKIP_ORIGINAL.  It's also an INTERNAL
	 * deletion, and we want to not drop any extensions that might happen to
	 * own temp objects.
	 */
	object.classId = NamespaceRelationId;
	object.objectId = tempNamespaceId;
	object.objectSubId = 0;

	performDeletion(&object, DROP_CASCADE,
					PERFORM_DELETION_INTERNAL |
					PERFORM_DELETION_QUIETLY |
					PERFORM_DELETION_SKIP_ORIGINAL |
					PERFORM_DELETION_SKIP_EXTENSIONS);
}
/* …（同文件无关代码省略）… */
		/* Need to ensure we have a usable transaction. */
		AbortOutOfAnyTransaction();
		StartTransactionCommand();
		PushActiveSnapshot(GetTransactionSnapshot());

		RemoveTempRelations(myTempNamespace);
