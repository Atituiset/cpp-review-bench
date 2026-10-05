// AUTO-DRAFT from sqlite/sqlite PR #52a8a406eb9cda436b10842734c63050da6d8f13
static int fts3CursorSeekStmt(Fts3Cursor *pCsr){
  int rc = SQLITE_OK;
  if( pCsr->pStmt==0 ){
    Fts3Table *p = (Fts3Table *)pCsr->base.pVtab;
    char *zSql;
    if( p->pSeekStmt ){
      pCsr->pStmt = p->pSeekStmt;
      p->pSeekStmt = 0;
    }else{
      zSql = sqlite3_mprintf("SELECT %s WHERE rowid = ?", p->zReadExprlist);
      if( !zSql ) return SQLITE_NOMEM;
      p->bLock++;
      rc = sqlite3Fts3PrepareStmt(p, zSql, 1, 1, &pCsr->pStmt);
      p->bLock--;
      sqlite3_free(zSql);
    }
    if( rc==SQLITE_OK ) pCsr->bSeekStmt = 1;
  }
  return rc;
}
/* …（同文件无关代码省略）… */
static int fts3CursorSeek(sqlite3_context *pContext, Fts3Cursor *pCsr){
  int rc = SQLITE_OK;
  if( pCsr->isRequireSeek ){
    rc = fts3CursorSeekStmt(pCsr);
    if( rc==SQLITE_OK ){
      Fts3Table *pTab = (Fts3Table*)pCsr->base.pVtab;
      pTab->bLock++;
      sqlite3_bind_int64(pCsr->pStmt, 1, pCsr->iPrevId);
      pCsr->isRequireSeek = 0;
      if( SQLITE_ROW==sqlite3_step(pCsr->pStmt) ){
        pTab->bLock--;
        return SQLITE_OK;
      }else{
        pTab->bLock--;
        rc = sqlite3_reset(pCsr->pStmt);
        if( rc==SQLITE_OK && ((Fts3Table *)pCsr->base.pVtab)->zContentTbl==0 ){
          /* If no row was found and no error has occurred, then the %_content
          ** table is missing a row that is present in the full-text index.
          ** The data structures are corrupt.  */
          rc = FTS_CORRUPT_VTAB;
          pCsr->isEof = 1;
        }
      }
    }
  }
  // <<< BUG ANCHOR
  if( rc!=SQLITE_OK && pContext ){
    sqlite3_result_error_code(pContext, rc);
  }
  return rc;
}
/* …（同文件无关代码省略）… */
  const char *zStart = "<b>";
  const char *zEnd = "</b>";
  const char *zEllipsis = "<b>...</b>";
  int iCol = -1;
  int nToken = 15;                /* Default number of tokens in snippet */

  /* There must be at least one argument passed to this function (otherwise
  ** the non-overloaded version would have been called instead of this one).
/* …（同文件无关代码省略）… */
  pTab = (Fts3Table *)pCsr->base.pVtab;

  switch( nVal ){
    case 6: nToken = sqlite3_value_int(apVal[5]);
            /* no break */ deliberate_fall_through
    case 5: iCol = sqlite3_value_int(apVal[4]);
            /* no break */ deliberate_fall_through
    case 4: zEllipsis = (const char*)sqlite3_value_text(apVal[3]);
            /* no break */ deliberate_fall_through
/* …（同文件无关代码省略）… */
  }else if( nToken==0 || iCol>=pTab->nColumn ){
    sqlite3_result_text(pContext, "", -1, SQLITE_STATIC);
  }else if( SQLITE_OK==fts3CursorSeek(pContext, pCsr) ){
    sqlite3Fts3Snippet(pContext, pCsr, zStart, zEnd, zEllipsis, iCol, nToken);
  }
}
