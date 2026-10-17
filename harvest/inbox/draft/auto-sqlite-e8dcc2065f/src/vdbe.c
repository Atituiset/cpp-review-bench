// AUTO-DRAFT from sqlite/sqlite PR #7fd436eaf965da688c22babb5b5042149ff12947
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
    assert( t>=12 );
    sqlite3RCStrRef(pBuf);
    if( t&1 ){
      rc = sqlite3VdbeMemSetStr(pDest, pBuf, len, encoding,
                                sqlite3RCStrUnref);
      pDest->flags |= MEM_Term;
    }else{
      rc = sqlite3VdbeMemSetStr(pDest, pBuf, len, 0,
                                sqlite3RCStrUnref);
      pDest->enc = encoding;
    }
  }else{
    rc = sqlite3VdbeMemFromBtree(pC->uc.pCursor, iOffset, len, pDest);
    if( rc ) return rc;
