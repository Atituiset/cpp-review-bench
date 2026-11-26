// AUTO-DRAFT from sqlite/sqlite PR #ad110ce72022f3da86715efab3ba09ddbc941512
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
static void resolveP2Values(Vdbe *p, int *pMaxVtabArgs){
  int nMaxVtabArgs = *pMaxVtabArgs;
  Op *pOp;
  Parse *pParse = p->pParse;
  int *aLabel = pParse->aLabel;

  assert( pParse->db->mallocFailed==0 ); /* tag-20230419-1 */
  p->readOnly = 1;
  p->bIsReader = 0;
  pOp = &p->aOp[p->nOp-1];
  assert( p->aOp[0].opcode==OP_Init );
  while( 1 /* Loop terminates when it reaches the OP_Init opcode */ ){
    /* Only JUMP opcodes and the short list of special opcodes in the switch
    ** below need to be considered.  The mkopcodeh.tcl generator script groups
    ** all these opcodes together near the front of the opcode list.  Skip
    ** any opcode that does not need processing by virtual of the fact that
    ** it is larger than SQLITE_MX_JUMP_OPCODE, as a performance optimization.
    */
    if( pOp->opcode<=SQLITE_MX_JUMP_OPCODE ){
      /* NOTE: Be sure to update mkopcodeh.tcl when adding or removing
      ** cases from this switch! */
      switch( pOp->opcode ){
        case OP_Transaction: {
          if( pOp->p2!=0 ) p->readOnly = 0;
          /* no break */ deliberate_fall_through
        }
        case OP_AutoCommit:
        case OP_Savepoint: {
          p->bIsReader = 1;
          break;
        }
#ifndef SQLITE_OMIT_WAL
        case OP_Checkpoint:
#endif
        case OP_Vacuum:
        case OP_JournalMode: {
          p->readOnly = 0;
          p->bIsReader = 1;
          break;
        }
        case OP_Init: {
          assert( pOp->p2>=0 );
          goto resolve_p2_values_loop_exit;
        }
#ifndef SQLITE_OMIT_VIRTUALTABLE
        case OP_VUpdate: {
          if( pOp->p2>nMaxVtabArgs ) nMaxVtabArgs = pOp->p2;
          break;
        }
        case OP_VFilter: {
          int n;
          /* The instruction immediately prior to VFilter will be an
          ** OP_Integer that sets the "argc" value for the VFilter.  See
          ** the code where OP_VFilter is generated at tag-20250207a. */
          assert( (pOp - p->aOp) >= 3 );
          assert( pOp[-1].opcode==OP_Integer );
          assert( pOp[-1].p2==pOp->p3+1 );
          n = pOp[-1].p1;
          if( n>nMaxVtabArgs ) nMaxVtabArgs = n;
          /* Fall through into the default case */
          /* no break */ deliberate_fall_through
        }
#endif
        default: {
          if( pOp->p2<0 ){
            /* The mkopcodeh.tcl script has so arranged things that the only
            ** non-jump opcodes less than SQLITE_MX_JUMP_CODE are guaranteed to
            ** have non-negative values for P2. */
            assert( (sqlite3OpcodeProperty[pOp->opcode] & OPFLG_JUMP)!=0 );
            assert( ADDR(pOp->p2)<-pParse->nLabel );
            assert( aLabel!=0 );  /* True because of tag-20230419-1 */
            pOp->p2 = aLabel[ADDR(pOp->p2)];
          }

          /* OPFLG_JUMP opcodes never have P2==0, though OPFLG_JUMP0 opcodes
          ** might */
          assert( pOp->p2>0 
                  || (sqlite3OpcodeProperty[pOp->opcode] & OPFLG_JUMP0)!=0 );

          /* Jumps never go off the end of the bytecode array */
          assert( pOp->p2<p->nOp
                  || (sqlite3OpcodeProperty[pOp->opcode] & OPFLG_JUMP)==0 );
          break;
        }
      }
      /* The mkopcodeh.tcl script has so arranged things that the only
      ** non-jump opcodes less than SQLITE_MX_JUMP_CODE are guaranteed to
      ** have non-negative values for P2. */
      assert( (sqlite3OpcodeProperty[pOp->opcode]&OPFLG_JUMP)==0 || pOp->p2>=0);
    }
    assert( pOp>p->aOp );   
    pOp--;
  }
resolve_p2_values_loop_exit:
  if( aLabel ){
    sqlite3DbNNFreeNN(p->db, pParse->aLabel);
    pParse->aLabel = 0;
  }
  pParse->nLabel = 0;
  *pMaxVtabArgs = nMaxVtabArgs;
  assert( p->bIsReader!=0 || DbMaskAllZero(p->btreeMask) );
}
/* …（同文件无关代码省略）… */
  assert( EIGHT_BYTE_ALIGNMENT(&x.pSpace[x.nFree]) );

  resolveP2Values(p, &nArg);
  p->usesStmtJournal = (u8)(pParse->isMultiWrite && pParse->mayAbort);
  if( pParse->explain ){
    if( nMem<10 ) nMem = 10;
    p->explain = pParse->explain;
