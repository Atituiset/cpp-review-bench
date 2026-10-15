// AUTO-DRAFT from sqlite/sqlite PR #fb0878ac38d23ebf699109e2c472e0469dd67be3
  regBase = sqlite3GetTempRange(pParse, nCol);
  if( pPrior && (regBase!=regPrior || pPrior->pPartIdxWhere) ) pPrior = 0;
  for(j=0; j<nCol; j++){
    if( pPrior
     && pPrior->aiColumn[j]==pIdx->aiColumn[j]
     && pPrior->aiColumn[j]!=XN_EXPR
/* …（同文件无关代码省略）… */
      continue;
    }
    sqlite3ExprCodeLoadIndexColumn(pParse, pIdx, iDataCur, j, regBase+j);
    if( pIdx->aiColumn[j]>=0 ){  // <<< BUG ANCHOR
      /* If the column affinity is REAL but the number is an integer, then it
      ** might be stored in the table as an integer (using a compact
      ** representation) then converted to REAL by an OP_RealAffinity opcode.
