// AUTO-DRAFT from sqlite/sqlite PR #b7395681d389862fcd4a3f3777042e177b1b6e55
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
  assert( pOp->p1>=0 && pOp->p1<p->nCursor );
  pC = p->apCsr[pOp->p1];
  if( pC && pC->nullRow ){
    sqlite3VdbeMemSetNull(aMem + pOp->p3);
    goto jump_to_p2;
  }
  break;
