// AUTO-DRAFT from sqlite/sqlite PR #eef1cf4591ff8b4f43e72aceafb7cf3723a74275
#define FTS_MAX_APPENDABLE_HEIGHT 16
/* …（同文件无关代码省略）… */
static void fts3ReadEndBlockField(
  sqlite3_stmt *pStmt, 
  int iCol, 
  i64 *piEndBlock,
  i64 *pnByte
){
  const unsigned char *zText = sqlite3_column_text(pStmt, iCol);
  if( zText ){
    int i;
    int iMul = 1;
    u64 iVal = 0;
    for(i=0; zText[i]>='0' && zText[i]<='9'; i++){
      iVal = iVal*10 + (zText[i] - '0');
    }
    *piEndBlock = (i64)iVal;
    while( zText[i]==' ' ) i++;
    iVal = 0;
    if( zText[i]=='-' ){
      i++;
      iMul = -1;
    }
    for(/* no-op */; zText[i]>='0' && zText[i]<='9'; i++){
      iVal = iVal*10 + (zText[i] - '0');
    }
  // <<< BUG ANCHOR
    /* This if() clause is just to avoid an integer overflow. The record is 
    ** corrupt in this case.  */
    if( (i64)iVal==SMALLEST_INT64 ) iMul = 1;
    *pnByte = ((i64)iVal * (i64)iMul);
  }
}
/* …（同文件无关代码省略）… */
      iStart = sqlite3_column_int64(pSelect, 1);
      iLeafEnd = sqlite3_column_int64(pSelect, 2);
      fts3ReadEndBlockField(pSelect, 3, &iEnd, &pWriter->nLeafData);
      if( pWriter->nLeafData<0 ){
        pWriter->nLeafData = pWriter->nLeafData * -1;
      }
/* …（同文件无关代码省略）… */
        return FTS_CORRUPT_VTAB;
      }

      pWriter->nLeafEst = (int)(((iEnd - iStart)+1)/FTS_MAX_APPENDABLE_HEIGHT);
      pWriter->iStart = iStart;
      pWriter->iEnd = iEnd;
      pWriter->iAbsLevel = iAbsLevel;
