// AUTO-DRAFT from sqlite/sqlite PR #3044c8b3b2be8f2fff54bb8357aa5377c01570c6
      if( pOp->opcode==OP_OpenRead ) break;
      if( pOp->opcode==OP_OpenWrite && (pOp->p5 & OPFLAG_P2ISREG)==0 ) break;
      if( pOp->opcode==OP_ReopenIdx ) break;     
    }else
#endif
    {
