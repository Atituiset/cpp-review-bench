// AUTO-DRAFT from sqlite/sqlite PR #d8ab154a39beefc971f88c217283e6f04d71d954
    rc = sqlite3OsAccess(pVfs, zSuper, SQLITE_ACCESS_EXISTS, &res);
  }
  if( rc!=SQLITE_OK || !res ){
    goto end_playback;
  }
  pPager->journalOff = 0;
