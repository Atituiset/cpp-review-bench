// AUTO-DRAFT from sqlite/sqlite PR #b1f3151bb75205d4ab34dbd7659bbfc80c79d175
          nRhs++;
        }
      }
      sqlite3ExprListDelete(db, pOrigRhs);
      if( pOrigLhs ){
        sqlite3ExprListDelete(db, pOrigLhs);
