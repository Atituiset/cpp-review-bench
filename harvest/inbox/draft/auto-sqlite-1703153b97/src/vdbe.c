// AUTO-DRAFT from sqlite/sqlite PR #fc96a3c4530bbc66274133d3727a556821dddce6
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
        p->apCsr[pOp->p1] = 0;  /* Not required; helps with static analysis */
      }else{
        assert( sqlite3BtreeClosesWithCursor(pCx->ub.pBtx, pCx->uc.pCursor) );
      }
    }
  }
/* …（同文件无关代码省略）… */
** If the cursor P1 was opened using the OPFLAG_SEEKEQ flag, then this
** opcode will either land on a record that exactly matches the key, or
** else it will cause a jump to P2.  When the cursor is OPFLAG_SEEKEQ,
** this opcode must be followed by an IdxLE opcode with the same arguments.
** The IdxGT opcode will be skipped if this opcode succeeds, but the
** IdxGT opcode will be used on subsequent loop iterations.  The
** OPFLAG_SEEKEQ flags is a hint to the btree layer to say that this
** is an equality search.
**
** This opcode leaves the cursor configured to move in forward order,
** from the beginning toward the end.  In other words, the cursor is
** configured to use Next, not Prev.
/* …（同文件无关代码省略）… */
** If the cursor P1 was opened using the OPFLAG_SEEKEQ flag, then this
** opcode will either land on a record that exactly matches the key, or
** else it will cause a jump to P2.  When the cursor is OPFLAG_SEEKEQ,
** this opcode must be followed by an IdxLE opcode with the same arguments.
** The IdxGE opcode will be skipped if this opcode succeeds, but the
** IdxGE opcode will be used on subsequent loop iterations.  The
** OPFLAG_SEEKEQ flags is a hint to the btree layer to say that this
** is an equality search.
**
** See also: Found, NotFound, SeekGt, SeekGe, SeekLt
*/
case OP_SeekLT:         /* jump0, in3, group, ncycle */
/* …（同文件无关代码省略）… */
      assert( pOp->opcode==OP_SeekGE || pOp[1].opcode==OP_IdxLT );
      assert( pOp->opcode==OP_SeekLE || pOp[1].opcode==OP_IdxGT );
      assert( pOp[1].p1==pOp[0].p1 );
      assert( pOp[1].p2==pOp[0].p2 );
      assert( pOp[1].p3==pOp[0].p3 );
      assert( pOp[1].p4.i==pOp[0].p4.i );
    }
