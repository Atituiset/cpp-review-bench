// AUTO-DRAFT from sqlite/sqlite PR #260ea5cf6a4024fe0a0d8959078abe3d08a6ed70
  pTab = pIdx->pTable;
  for(i=0; i<pIdx->nColumn; i++){
    Expr *pExpr;
    int j = pIdx->aiColumn[i];
    if( j==XN_EXPR ){
      pExpr = pIdx->aColExpr->a[i].pExpr;
/* …（同文件无关代码省略）… */
      ** numeric affinities, so check for that mismatch here. */
      if( (pTab->aCol[j].affinity==SQLITE_AFF_REAL)
           !=(sqlite3ExprAffinity(pExpr)==SQLITE_AFF_REAL) ) continue;
    }else{
      continue;
    }
