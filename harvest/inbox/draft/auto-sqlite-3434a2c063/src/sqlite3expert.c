// AUTO-DRAFT from sqlite/sqlite PR #814fd8ef4d42d6567f1613a66f556c8194764d06
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef sqlite3_int64 i64;
/* …（同文件无关代码省略）… */
typedef struct IdxConstraint IdxConstraint;
/* …（同文件无关代码省略）… */
#define STRLEN  (int)strlen
/* …（同文件无关代码省略）… */
static void *idxMalloc(int *pRc, i64 nByte){
  void *pRet;
  assert( *pRc==SQLITE_OK );
  assert( nByte>0 );
  pRet = sqlite3_malloc64(nByte);
  if( pRet ){
    memset(pRet, 0, nByte);
  }else{
    *pRc = SQLITE_NOMEM;
  }
  return pRet;
}
/* …（同文件无关代码省略）… */
  int nColl = STRLEN(zColl);

  assert( *pRc==SQLITE_OK );
  pNew = (IdxConstraint*)idxMalloc(pRc, sizeof(IdxConstraint) * nColl + 1);
  if( pNew ){
    pNew->zColl = (char*)&pNew[1];
    memcpy(pNew->zColl, zColl, nColl+1);
