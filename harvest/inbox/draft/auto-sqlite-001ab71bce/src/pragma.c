// AUTO-DRAFT from sqlite/sqlite PR #2439a84bbd1304c4b93184da04370f5ce32801c0
  if( i<pTab->iHidden ){
    sqlite3_result_value(ctx, sqlite3_column_value(pCsr->pPragma, i));
  }else{
    sqlite3_result_text(ctx, pCsr->azArg[i-pTab->iHidden],-1,SQLITE_TRANSIENT);  // <<< BUG ANCHOR
  }
  return SQLITE_OK;
}
