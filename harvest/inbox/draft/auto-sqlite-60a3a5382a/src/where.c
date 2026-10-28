// AUTO-DRAFT from sqlite/sqlite PR #7721051f694f039ed364cd04af71024fcbf113f8
          }
        }
      }
      if( pLevel->op==OP_Return ){
        sqlite3VdbeAddOp2(v, OP_Gosub, pLevel->p1, pLevel->addrFirst);
      }else{
