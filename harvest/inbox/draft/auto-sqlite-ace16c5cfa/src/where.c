// AUTO-DRAFT from sqlite/sqlite PR #644fa62518a4f7b15a2d9b3b1c15aa5012c6782b
        }
        sqlite3VdbeAddOp1(v, OP_NullRow, pLevel->iIdxCur);
      }
      if( pLevel->op==OP_Return ){
        sqlite3VdbeAddOp2(v, OP_Gosub, pLevel->p1, pLevel->addrFirst);
      }else{
