// AUTO-DRAFT from sqlite/sqlite PR #3b4484bed2fc51b91dfd4bbe301ba576625e9e34
#ifdef APPENDVFS_TEST
  if( rc==SQLITE_OK ){
    rc = sqlite3_auto_extension((void(*)(void))apndvfsRegister);
  }
#endif
  if( rc==SQLITE_OK ) rc = SQLITE_OK_LOAD_PERMANENTLY;
