// AUTO-DRAFT from sqlite/sqlite PR #bedc31875f5b9e241d8420c68575c2eb44e6b2ff
  if( p->rc==SQLITE_OK ){
    if( zSql ){
      int rc = sqlite3_prepare_v3(p->pConfig->db, zSql, -1,
          SQLITE_PREPARE_PERSISTENT|SQLITE_PREPARE_NO_VTAB,  // <<< BUG ANCHOR
          ppStmt, 0);
      /* If this prepare() call fails with SQLITE_ERROR, then one of the
      ** %_idx or %_data tables has been removed or modified. Call this
