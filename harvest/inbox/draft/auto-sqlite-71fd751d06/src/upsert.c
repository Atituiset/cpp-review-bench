// AUTO-DRAFT from sqlite/sqlite PR #c30e731ac37f5a063736565c4829ad6d05b093f7
    /* Check for matches against other indexes */
    for(pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext){
      int ii, jj, nn;
      if( !IsUniqueIndex(pIdx) ) continue;
      if( pTarget->nExpr!=pIdx->nKeyCol ) continue;
      if( pIdx->pPartIdxWhere ){
/* …（同文件无关代码省略）… */
          sCol[1].iColumn = pIdx->aiColumn[ii];
          pExpr = &sCol[0];
        }
        for(jj=0; jj<nn; jj++){
          if( sqlite3ExprCompare(0,pTarget->a[jj].pExpr,pExpr,iCursor)<2 ){
            break;  /* Column ii of the index matches column jj of target */
          }
        }
        if( jj>=nn ){  // <<< BUG ANCHOR
          /* The target contains no match for column jj of the index */
          break;
        }
      }
      if( ii<nn ){
        /* Column ii of the index did not match any term of the conflict target.
        ** Continue the search with the next index. */
        continue;
      }
      pUpsert->pUpsertIdx = pIdx;
      if( sqlite3UpsertOfIndex(pAll,pIdx)!=pUpsert ){
        /* Really this should be an error.  The isDup ON CONFLICT clause will
/* …（同文件无关代码省略）… */
Upsert *sqlite3UpsertOfIndex(Upsert *pUpsert, Index *pIdx){
  while(
      pUpsert
   && pUpsert->pUpsertTarget!=0
   && pUpsert->pUpsertIdx!=pIdx
  ){
     pUpsert = pUpsert->pNextUpsert;
  }
  return pUpsert;
}
