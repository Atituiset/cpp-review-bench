// AUTO-DRAFT from sqlite/sqlite PR #ac429ff80483f25c0f7d1ea02f3e21040ec388e3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
static int alsoAnInt(Mem *pRec, double rValue, i64 *piValue){
  i64 iValue;
  iValue = sqlite3RealToI64(rValue);
  if( sqlite3RealSameAsInt(rValue,iValue) ){
    *piValue = iValue;
    return 1;
  }
  return 0==sqlite3Atoi64(pRec->z, piValue, pRec->n, pRec->enc);
}
/* …（同文件无关代码省略）… */
static void applyNumericAffinity(Mem *pRec, int bTryForInt){
  double rValue;
  int rc;
  assert( (pRec->flags & (MEM_Str|MEM_Int|MEM_Real|MEM_IntReal))==MEM_Str );
  rc = sqlite3MemRealValueRC(pRec, &rValue);
  if( rc<=0 ) return;
  if( (rc&2)==0 && alsoAnInt(pRec, rValue, &pRec->u.i) ){
    pRec->flags |= MEM_Int;
  }else{
    pRec->u.r = rValue;
    pRec->flags |= MEM_Real;
    if( bTryForInt ) sqlite3VdbeIntegerAffinity(pRec);
  }
  /* TEXT->NUMERIC is many->one.  Hence, it is important to invalidate the
  ** string representation after computing a numeric equivalent, because the
  ** string representation might not be the canonical representation for the
  ** numeric value.  Ticket [343634942dd54ab57b7024] 2018-01-31. */
  pRec->flags &= ~MEM_Str;
}
/* …（同文件无关代码省略）… */
      if( (pRec->flags & (MEM_Real|MEM_IntReal))==0 ){
        if( pRec->flags & MEM_Str ) applyNumericAffinity(pRec,1);
      }else if( affinity<=SQLITE_AFF_REAL ){
        sqlite3VdbeIntegerAffinity(pRec);
      }
    }
  }else if( affinity==SQLITE_AFF_TEXT ){
