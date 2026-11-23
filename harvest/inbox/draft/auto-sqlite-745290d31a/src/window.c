// AUTO-DRAFT from sqlite/sqlite PR #774768ac159d8bab59579ffcba6e470c96c6e355
  sqlite3 *db = pParse->db;
  sqlite3_value *pVal = 0;
  sqlite3ValueFromExpr(db, pExpr, db->enc, SQLITE_AFF_NUMERIC, &pVal);
  if( pVal && sqlite3_value_int(pVal)>0 ){  // <<< BUG ANCHOR
    ret = 1;
  }
  sqlite3ValueFree(pVal);
