// AUTO-DRAFT from postgres/postgres PR #b8bf28800559a9ad71360fb2384d4b9981d24fa8
	if (!object_ownercheck(RelationRelationId, relid, GetUserId()))
		aclcheck_error(ACLCHECK_NOT_OWNER, get_relkind_objtype(get_rel_relkind(relid)), rv->relname);

	/*
	 * Conflict log tables are used internally for logical replication
	 * conflict logging and should not be modified directly, as it could
	 * disrupt conflict logging.
	 */
	if (IsConflictLogTableClass(classform))
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
