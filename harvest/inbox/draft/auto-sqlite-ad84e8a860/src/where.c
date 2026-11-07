// AUTO-DRAFT from sqlite/sqlite PR #9d4b8be4b3bd033b19a3f2ab7fcb27a009c67688
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
        idxCols |= cMask;
        pIdx->aiColumn[n] = pTerm->u.x.leftColumn;
        pColl = sqlite3ExprCompareCollSeq(pParse, pX);
        assert( pColl!=0 || pParse->nErr>0 ); /* TH3 collate01.800 */
        if( !sqlite3IsBinary(pColl) ){
          /* Disallow the use of a Bloom filter if any non-BINARY collating
          ** sequence is involved.  tag-202607231411 */
