// AUTO-DRAFT from sqlite/sqlite PR #6410c0e4a9bc2a7b24216b3ccc37e45eea257940
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>

  typedef unsigned char u8;
#endif

extern const char *sqlite3ErrName(int);

typedef struct TestSession TestSession;
/* …（同文件无关代码省略）… */
static int sqlite3_test_changeset(int, void *, char **);
static void assert_changeset_is_ok(int n, void *p){
  char *z = 0;
  (void)sqlite3_test_changeset(n, p, &z);
  assert( z==0 );
}
#else
/* …（同文件无关代码省略）… */
static int sqlite3_test_changeset(
  int nChangeset,
  void *pChangeset,
  char **pzErr
){
  sqlite3_changeset_iter *pIter = 0;
  char *zErr = 0;
  int rc = SQLITE_OK;
  int bPatch = (nChangeset>0 && ((char*)pChangeset)[0]=='P');

  rc = sqlite3changeset_start(&pIter, nChangeset, pChangeset);
  if( rc==SQLITE_OK ){
    int rc2;
    while( rc==SQLITE_OK && SQLITE_ROW==sqlite3changeset_next(pIter) ){
      unsigned char *aPk = 0;
      int nCol = 0;
      int op = 0;
      const char *zTab = 0;

      sqlite3changeset_pk(pIter, &aPk, &nCol);
      sqlite3changeset_op(pIter, &zTab, &nCol, &op, 0);

      if( op==SQLITE_UPDATE ){
        int iCol;
        for(iCol=0; iCol<nCol; iCol++){
          sqlite3_value *pNew = 0;
          sqlite3_value *pOld = 0;
          sqlite3changeset_new(pIter, iCol, &pNew);
          sqlite3changeset_old(pIter, iCol, &pOld);

          if( aPk[iCol] ){
            if( pOld==0 ) rc = SQLITE_ERROR;
          }else if( bPatch ){
            if( pOld ) rc = SQLITE_ERROR;
          }else{
            if( (pOld==0)!=(pNew==0) ) rc = SQLITE_ERROR;
          }

          if( rc!=SQLITE_OK ){
            zErr = sqlite3_mprintf(
                "unexpected SQLITE_UPDATE (bPatch=%d pk=%d pOld=%d pNew=%d)",
                bPatch, (int)aPk[iCol], pOld!=0, pNew!=0
            );
            break;
          }
        }
      }
    }
    rc2 = sqlite3changeset_finalize(pIter);
    if( rc==SQLITE_OK ){
      rc = rc2;
    }
  }

  *pzErr = zErr;
  return rc;
}
