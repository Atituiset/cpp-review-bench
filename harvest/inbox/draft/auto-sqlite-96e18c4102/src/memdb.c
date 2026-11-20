// AUTO-DRAFT from sqlite/sqlite PR #ef1ce9fa9f103f43a49432f6b47360b686a9aee8
  }
  if( szDb<0 ) return SQLITE_MISUSE_BKPT;
  if( szBuf<0 ) return SQLITE_MISUSE_BKPT;
#endif

  sqlite3_mutex_enter(db->mutex);
