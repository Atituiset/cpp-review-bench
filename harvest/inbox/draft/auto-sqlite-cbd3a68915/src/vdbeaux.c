// AUTO-DRAFT from sqlite/sqlite PR #8532bf12f45ba35d156fcbfc1afa0d808435e63b
     || opcode==OP_Function || opcode==OP_PureFunc
     || ((opcode==OP_Halt || opcode==OP_HaltIfNull)
      && ((pOp->p1)!=SQLITE_OK && pOp->p2==OE_Abort))
    ){
      hasAbort = 1;
      break;
