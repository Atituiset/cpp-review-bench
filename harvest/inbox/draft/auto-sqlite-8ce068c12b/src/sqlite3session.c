// AUTO-DRAFT from sqlite/sqlite PR #291abc0c02154e8633668feaf7c3665d33c66fc7
typedef struct SessionTable SessionTable;
/* …（同文件无关代码省略）… */
typedef struct SessionBuffer SessionBuffer;
/* …（同文件无关代码省略）… */
static int sessionVarintLen(int iVal){
  return sqlite3VarintLen(iVal);
}
/* …（同文件无关代码省略）… */
static int sessionBufferGrow(SessionBuffer *p, i64 nByte, int *pRc){
#define SESSION_MAX_BUFFER_SZ (0x7FFFFF00 - 1) 
  i64 nReq = p->nBuf + nByte;
  if( *pRc==SQLITE_OK && nReq>p->nAlloc ){
    u8 *aNew;
    i64 nNew = p->nAlloc ? p->nAlloc : 128;
  // <<< BUG ANCHOR
    do {
      nNew = nNew*2;
    }while( nNew<nReq );

    /* The value of SESSION_MAX_BUFFER_SZ is copied from the implementation
    ** of sqlite3_realloc64(). Allocations greater than this size in bytes
    ** always fail. It is used here to ensure that this routine can always
    ** allocate up to this limit - instead of up to the largest power of
    ** two smaller than the limit.  */
    if( nNew>SESSION_MAX_BUFFER_SZ ){
      nNew = SESSION_MAX_BUFFER_SZ;
      if( nNew<nReq ){
        *pRc = SQLITE_NOMEM;
        return 1;
      }
    }

    aNew = (u8 *)sqlite3_realloc64(p->aBuf, nNew);
    if( 0==aNew ){
      *pRc = SQLITE_NOMEM;
    }else{
      p->aBuf = aNew;
      p->nAlloc = nNew;
    }
  }
  return (*pRc!=SQLITE_OK);
}
/* …（同文件无关代码省略）… */
typedef struct ChangeData ChangeData;
/* …（同文件无关代码省略）… */
struct sqlite3_changegroup {
  int rc;                         /* Error code */
  int bPatch;                     /* True to accumulate patchsets */
  SessionTable *pList;            /* List of tables in current patch */
  SessionBuffer rec;

  sqlite3 *db;                    /* Configured by changegroup_schema() */
  char *zDb;                      /* Configured by changegroup_schema() */
  ChangeData cd;                  /* Used by changegroup_change_xxx() APIs. */
};
/* …（同文件无关代码省略）… */
static int checkChangeParams(
  sqlite3_changegroup *pGrp, 
  int bNew,
  int iCol,
  sqlite3_int64 nReq,
  SessionBuffer **ppBuf
){
  int rc = SQLITE_OK;
  if( pGrp->cd.pTab==0 ){
    rc = SQLITE_MISUSE;
  }else if( iCol<0 || iCol>=pGrp->cd.pTab->nCol ){
    rc = SQLITE_RANGE;
  }else if( 
      (bNew && pGrp->cd.eOp==SQLITE_DELETE)
   || (!bNew && pGrp->cd.eOp==SQLITE_INSERT) 
  ){
    rc = SQLITE_ERROR;
  }else{
    SessionBuffer *pBuf = &pGrp->cd.aBuf[iCol]; 
    if( pGrp->cd.eOp==SQLITE_UPDATE && bNew ){
      pBuf += pGrp->cd.pTab->nCol;
    }
    pBuf->nBuf = 0;
    if( sessionBufferGrow(pBuf, nReq, &rc)==0 ){
      pBuf->nBuf = (int)nReq;
    }
    *ppBuf = pBuf;
  }
  return rc;
}
/* …（同文件无关代码省略）… */
  const void *pVal, 
  int nVal
){
  sqlite3_int64 nByte = 1 + sessionVarintLen(nVal) + (i64)nVal;
  int rc = SQLITE_OK;
  SessionBuffer *pBuf = 0;

  if( SQLITE_OK!=(rc = checkChangeParams(pGrp, bNew, iCol, nByte, &pBuf)) ){
    return rc;
  }
