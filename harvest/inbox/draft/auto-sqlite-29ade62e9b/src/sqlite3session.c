// AUTO-DRAFT from sqlite/sqlite PR #d64f3d492cd3938ac9e7e5f2eefa2ff6ba05c174
typedef struct SessionBuffer SessionBuffer;
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
  const char *zStr,               /* String to quote, escape and append */
  int *pRc                        /* IN/OUT: Error code */
){
  int nStr = sqlite3Strlen30(zStr)*2 + 2 + 2;
  if( 0==sessionBufferGrow(p, nStr, pRc) ){
    char *zOut = (char *)&p->aBuf[p->nBuf];
    const char *zIn = zStr;
