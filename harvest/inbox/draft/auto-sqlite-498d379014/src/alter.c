// AUTO-DRAFT from sqlite/sqlite PR #1ad569d147a736ccecb2b94fde2743a80d9ff34a
  /* Search for a constraint violation. Throw an exception if one is found. */
  sqlite3NestedParse(pParse,
      "SELECT sqlite_fail('constraint failed', %d) "
      "FROM %Q.%Q WHERE (%.*s) IS NOT TRUE",   // <<< BUG ANCHOR
      SQLITE_CONSTRAINT, zDb, pTab->zName, nExpr, zExpr
  );
