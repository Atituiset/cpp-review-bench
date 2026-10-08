// AUTO-DRAFT from sqlite/sqlite PR #97048e902ce5d46e17901f347f9391308a07f788
      if( ws & WHERE_IN_ABLE ){
        int m;
        for(m=0; m<pLevel->u.in.nIn; m++){
          sqlite3VdbeAddOp1(v, OP_NullRow, pLevel->u.in.aInLoop[m].iCur);  // <<< BUG ANCHOR
        }
      }
      if( pLevel->op==OP_Return ){
