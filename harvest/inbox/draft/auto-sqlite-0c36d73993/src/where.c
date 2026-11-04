// AUTO-DRAFT from sqlite/sqlite PR #d9af1e23f1c480b3a38812f1009d222ed1f24609
  Bitmask idxCols;            /* Bitmap of columns used for indexing */
  Bitmask pkCols;             /* PK cols whose key collation keeps them unique */
  Bitmask extraCols;          /* Bitmap of additional columns */
  u8 sentWarning = 0;         /* True if a warning has been issued */
  u8 useBloomFilter = 1;      /* True to also add a Bloom filter */
  Expr *pPartial = 0;         /* Partial Index Expression */
/* …（同文件无关代码省略）… */
  }else{
    extraCols = pSrc->colUsed & (~idxCols | MASKBIT(BMS-1));
  }
  if( !HasRowid(pTable) ){
    /* For WITHOUT ROWID tables, ensure that all PRIMARY KEY columns are
    ** either in the pkCols mask or in the extraCols mask. A column that
/* …（同文件无关代码省略）… */
    for(i=0; i<pTable->nCol; i++){
      if( (pTable->aCol[i].colFlags & COLFLAG_PRIMKEY)==0 ) continue;
      if( i>=BMS-1 ){
        extraCols |= MASKBIT(BMS-1);
        break;
      }
      if( pkCols & MASKBIT(i) ) continue;
/* …（同文件无关代码省略）… */
  for(i=0; i<mxBitCol; i++){
    if( extraCols & MASKBIT(i) ) nKeyCol++;
  }
  if( pSrc->colUsed & MASKBIT(BMS-1) ){  // <<< BUG ANCHOR
    nKeyCol += pTable->nCol - BMS + 1;
  }

/* …（同文件无关代码省略）… */
      n++;
    }
  }
  if( pSrc->colUsed & MASKBIT(BMS-1) ){
    for(i=BMS-1; i<pTable->nCol; i++){
      pIdx->aiColumn[n] = i;
      pIdx->azColl[n] = sqlite3StrBINARY;
