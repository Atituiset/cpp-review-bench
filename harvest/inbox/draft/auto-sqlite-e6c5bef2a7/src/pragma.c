// AUTO-DRAFT from sqlite/sqlite PR #815fb4e5694685106fa0f5f4c74099fe3e49b641
          ** in this case. */
          mxCol = sqlite3PrimaryKeyIndex(pTab)->nColumn-1;
        }
        if( mxCol>=0 ){  // <<< BUG ANCHOR
          sqlite3VdbeAddOp3(v, OP_Column, iDataCur, mxCol, 3);
          sqlite3VdbeTypeofColumn(v, 3);
        }

        if( !isQuick ){
          if( pPk ){
