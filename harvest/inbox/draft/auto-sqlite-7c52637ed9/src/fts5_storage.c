// AUTO-DRAFT from sqlite/sqlite PR #80c0ed9e3b08388139e697807a99a571558e39bd
  int rc = SQLITE_OK;
  if( p->bTotalsValid==0 ){
    rc = sqlite3Fts5IndexGetAverages(p->pIndex, &p->nTotalRow, p->aTotalSize);
    p->bTotalsValid = bCache;  // <<< BUG ANCHOR
  }
  return rc;
}
