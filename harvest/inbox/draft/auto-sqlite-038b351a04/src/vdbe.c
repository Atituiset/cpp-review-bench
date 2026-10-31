// AUTO-DRAFT from sqlite/sqlite PR #802f9933c43135a7057a229e08dcf3392aeed93a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>

  break;
}

/* Opcode: NullRow P1 P2 * * *
**
** Move the cursor P1 to a null row if P2 is 0.  Any OP_Column operations
/* …（同文件无关代码省略）… */
*/
case OP_NullRow: {
  VdbeCursor *pC;

  assert( pOp->p1>=0 && pOp->p1<p->nCursor );
  pC = p->apCsr[pOp->p1];
/* …（同文件无关代码省略）… */
  if( pC->eCurType==CURTYPE_BTREE ){
    assert( pC->uc.pCursor!=0 );
    sqlite3BtreeClearCursor(pC->uc.pCursor);
  }
#ifdef SQLITE_DEBUG
  if( pC->seekOp==0 ) pC->seekOp = OP_NullRow;
