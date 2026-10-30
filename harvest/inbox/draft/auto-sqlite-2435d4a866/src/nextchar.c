// AUTO-DRAFT from sqlite/sqlite PR #67914d1478f9f71a1b731b7de76a92fe2cd71129
typedef struct nextCharContext nextCharContext;
/* …（同文件无关代码省略）… */
static void nextCharAppend(nextCharContext *p, unsigned c){
  int i;
  for(i=0; i<p->nUsed; i++){
    if( p->aResult[i]==c ) return;
  }
  if( p->nUsed+1 > p->nAlloc ){
    unsigned int *aNew;
    int n = p->nAlloc*2 + 30;
    aNew = sqlite3_realloc64(p->aResult, n*sizeof(unsigned int));
    if( aNew==0 ){
      p->mallocFailed = 1;
      return;
    }else{
      p->aResult = aNew;
      p->nAlloc = n;
    }
  }
  p->aResult[p->nUsed++] = c;
}
/* …（同文件无关代码省略）… */
static int writeUtf8(unsigned char *z, unsigned c){
  if( c<0x00080 ){
    z[0] = (unsigned char)(c&0xff);
    return 1;
  }
  if( c<0x00800 ){
    z[0] = 0xC0 + (unsigned char)((c>>6)&0x1F);
    z[1] = 0x80 + (unsigned char)(c & 0x3F);
    return 2;
  }
  if( c<0x10000 ){
    z[0] = 0xE0 + (unsigned char)((c>>12)&0x0F);
    z[1] = 0x80 + (unsigned char)((c>>6) & 0x3F);
    z[2] = 0x80 + (unsigned char)(c & 0x3F);
    return 3;
  }
  z[0] = 0xF0 + (unsigned char)((c>>18) & 0x07);
  z[1] = 0x80 + (unsigned char)((c>>12) & 0x3F);
  z[2] = 0x80 + (unsigned char)((c>>6) & 0x3F);
  z[3] = 0x80 + (unsigned char)(c & 0x3F);
  return 4;
}
/* …（同文件无关代码省略）… */
static int readUtf8(const unsigned char *z, unsigned *pOut){
  static const unsigned char validBits[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x00, 0x00,
  };
  unsigned c = z[0];
  if( c<0xc0 ){
    *pOut = c;
    return 1;
  }else{
    int n = 1;
    c = validBits[c-0xc0];
    while( (z[n] & 0xc0)==0x80 ){
      c = (c<<6) + (0x3f & z[n++]);
    }
    if( c<0x80 || (c&0xFFFFF800)==0xD800 || (c&0xFFFFFFFE)==0xFFFE ){
      c = 0xFFFD;
    }
    *pOut = c;
    return n;
  }
}
/* …（同文件无关代码省略）… */
/*
** The nextCharContext structure has been set up.  Add all "next" characters
** to the result set.
*/
static void findNextChars(nextCharContext *p){
  unsigned cPrev = 0;
  unsigned char zPrev[8];
  int n, rc;
    // <<< BUG ANCHOR
  for(;;){
    sqlite3_bind_text(p->pStmt, 1, (char*)p->zPrefix, p->nPrefix,
                      SQLITE_STATIC);
    n = writeUtf8(zPrev, cPrev+1);
    sqlite3_bind_text(p->pStmt, 2, (char*)zPrev, n, SQLITE_STATIC);
    rc = sqlite3_step(p->pStmt);
    if( rc==SQLITE_DONE ){
      sqlite3_reset(p->pStmt);
      return;
    }else if( rc!=SQLITE_ROW ){
      p->otherError = rc;
      return;
    }else{
      const unsigned char *zOut = sqlite3_column_text(p->pStmt, 0);
      unsigned cNext;
      if( sqlite3_column_bytes(p->pStmt,0)<=p->nPrefix ){
        sqlite3_reset(p->pStmt);
        continue;
      }
      n = readUtf8(zOut+p->nPrefix, &cNext);
      sqlite3_reset(p->pStmt);
      nextCharAppend(p, cNext);
      cPrev = cNext;
      if( p->mallocFailed ) return;
    }
  }
}


/* …（同文件无关代码省略）… */
  }
  zSql = sqlite3_mprintf(
    "SELECT %s FROM %s"
    " WHERE %s>=(?1 || ?2) %s"
    "   AND %s<=(?1 || char(1114111)) %s" /* 1114111 == 0x10ffff */
    "   %s"
    " ORDER BY 1 %s ASC LIMIT 1",
    zField, zTable, zField, zColl, zField, zColl, zWhereClause, zColl
  );
  if( zWhereClause[0] ) sqlite3_free(zWhereClause);
  if( zColl[0] ) sqlite3_free(zColl);
