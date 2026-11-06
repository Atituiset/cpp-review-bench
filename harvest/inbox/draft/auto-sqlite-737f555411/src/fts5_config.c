// AUTO-DRAFT from sqlite/sqlite PR #bedc31875f5b9e241d8420c68575c2eb44e6b2ff
  // <<< BUG ANCHOR
  zSql = sqlite3Fts5Mprintf(&rc, zSelect, pConfig->zDb, pConfig->zName);
  if( zSql ){
    rc = sqlite3_prepare_v2(pConfig->db, zSql, -1, &p, 0);
    sqlite3_free(zSql);
  }
