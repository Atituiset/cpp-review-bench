// AUTO-DRAFT from sqlite/sqlite PR #0290d4b7778eb75714d5cb97eefd14b5fd1f4096
               sqlite3TableColumnToStorage(pFKey->pFrom, aiCol[i])+1+regData,
               regTemp+i);
      }
    // <<< BUG ANCHOR
      /* If the parent table is the same as the child table, and we are about
      ** to increment the constraint-counter (i.e. this is an INSERT operation),
      ** then check if the row being inserted matches itself. If so, do not
/* …（同文件无关代码省略）… */
      if( pTab==pFKey->pFrom && nIncr==1 ){
        int iJump = sqlite3VdbeCurrentAddr(v) + nCol + 1;
        for(i=0; i<nCol; i++){
          int iChild = sqlite3TableColumnToStorage(pFKey->pFrom,aiCol[i])
                              +1+regData;
          int iParent = 1+regData;
          iParent += sqlite3TableColumnToStorage(pIdx->pTable,
                                                 pIdx->aiColumn[i]);
/* …（同文件无关代码省略）… */
            /* The parent key is a composite key that includes the IPK column */
            iParent = regData;
          }
          sqlite3VdbeAddOp3(v, OP_Ne, iChild, iJump, iParent); VdbeCoverage(v);
          sqlite3VdbeChangeP5(v, SQLITE_JUMPIFNULL);
        }
        sqlite3VdbeGoto(v, iOk);
      }

      sqlite3VdbeAddOp4(v, OP_Affinity, regTemp, nCol, 0,
                        sqlite3IndexAffinityStr(pParse->db,pIdx), nCol);
      sqlite3VdbeAddOp4Int(v, OP_Found, iCur, iOk, regTemp, nCol);
      VdbeCoverage(v);
      sqlite3ReleaseTempRange(pParse, regTemp, nCol);
