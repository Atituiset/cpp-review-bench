// AUTO-DRAFT from sqlite/sqlite PR #a3426a5bcc467f98490d49c83e8e1cd1255ad9a4
  ** then we have to invoke the (application-supplied) WSD initialization
  ** routine before doing anything else. */
#ifdef SQLITE_OMIT_WSD
  rc = sqlite3_wsd_init(4096, 24);  // <<< BUG ANCHOR
  if( rc!=SQLITE_OK ){
    return rc;
  }
