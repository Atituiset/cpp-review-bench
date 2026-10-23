// AUTO-DRAFT from sqlite/sqlite PR #bc766d966a3690f848e8c9ace9f01380f98ab9cd
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct SessionBuffer SessionBuffer;
/* …（同文件无关代码省略）… */
static int sessionVarintGet(const u8 *aBuf, int *piVal){
  int ret = getVarint32(aBuf, *piVal);
  *piVal = (*piVal & 0x7FFFFFFF);
  return ret;
}
/* …（同文件无关代码省略）… */
static int sessionSerialLen(const u8 *a){
  int e;
  int n;
  assert( a!=0 );
  e = *a;
  if( e==SQLITE_INTEGER || e==SQLITE_FLOAT ) return 9;
  if( e==SQLITE_TEXT || e==SQLITE_BLOB ){
    return sessionVarintGet(&a[1], &n) + 1 + n;
  }
  return 1;
}
/* …（同文件无关代码省略）… */
static int sessionBufferGrow(SessionBuffer *p, i64 nByte, int *pRc){
#define SESSION_MAX_BUFFER_SZ (0x7FFFFF00 - 1) 
  i64 nReq = p->nBuf + nByte;
  if( *pRc==SQLITE_OK && nReq>p->nAlloc ){
    u8 *aNew;
    i64 nNew = p->nAlloc ? p->nAlloc : 128;

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
static void sessionAppendByte(SessionBuffer *p, u8 v, int *pRc){
  if( 0==sessionBufferGrow(p, 1, pRc) ){
    p->aBuf[p->nBuf++] = v;
  }
}
/* …（同文件无关代码省略）… */
      a2 = aChange;
      for(i=0; i<pIter->nCol; i++){
        int n1 = sessionSerialLen(a1);
        int n2 = sessionSerialLen(a2);
        if( pIter->abPK[i] || a2[0]!=0xFF ){
          memcpy(pOut, a1, n1);
          pOut += n1;
        }else{
/* …（同文件无关代码省略）… */
      if( pIter->bPatchset ){
        rc = SQLITE_ERROR;
      }

      /* Append a table header to the output for this new table */
      sessionAppendByte(&sOut, pIter->bPatchset ? 'P' : 'T', &rc);
