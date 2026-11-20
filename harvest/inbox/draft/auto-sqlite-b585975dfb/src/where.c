// AUTO-DRAFT from sqlite/sqlite PR #aa4813822fc50e71d9af8afb7d8dbfda0f710419
      ** successful row, it should break out of itself. */
      sqlite3VdbeAddOp2(v, OP_Goto, 0, pLevel->addrBrk);
      VdbeComment((v, "EXISTS break %d", i));
    }else if( pLevel->addrDeleteCont ){  // <<< BUG ANCHOR
      /* An inner loop matched no rows on this auto-index driven loop and
      ** there are outer loops.  So delete the auto-index entry to avoid
      ** future unnecessary work. */
