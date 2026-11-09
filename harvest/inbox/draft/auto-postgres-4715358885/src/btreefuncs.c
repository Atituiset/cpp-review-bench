// AUTO-DRAFT from postgres/postgres PR #553262aff40d34711e04e989e91d1da430589822
				 errmsg("\"%s\" is not a %s index",
						RelationGetRelationName(rel), "btree")));
  // <<< BUG ANCHOR
	/*
	 * Reject attempts to read non-local temporary relations; we would be
	 * likely to get wrong data since we have no visibility into the owning
	 * session's local buffers.
	 */
	if (RELATION_IS_OTHER_TEMP(rel))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("cannot access temporary tables of other sessions")));

	if (blkno == 0)
		ereport(ERROR,
/* …（同文件无关代码省略）… */
				 errmsg("\"%s\" is not a %s index",
						RelationGetRelationName(rel), "btree")));

	/*
	 * Reject attempts to read non-local temporary relations; we would be
	 * likely to get wrong data since we have no visibility into the owning
	 * session's local buffers.
	 */
	if (RELATION_IS_OTHER_TEMP(rel))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("cannot access temporary tables of other sessions")));

	buffer = ReadBuffer(rel, 0);
	LockBuffer(buffer, BUFFER_LOCK_SHARE);
