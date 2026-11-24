// AUTO-DRAFT from postgres/postgres PR #9ebc9a458c38465cd9609814cfc9ca3a2f46fef5
  // <<< BUG ANCHOR
	if (collOid == DEFAULT_COLLATION_OID)
		ereport(ERROR,
				(errmsg("cannot refresh version of default collation"),
		/* translator: %s is an SQL command */
				 errhint("Use %s instead.",
						 "ALTER DATABASE ... REFRESH COLLATION VERSION")));
