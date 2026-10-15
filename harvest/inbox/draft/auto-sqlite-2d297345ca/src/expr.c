// AUTO-DRAFT from sqlite/sqlite PR #9d1806e3c099345f9f5400f8c5bb496f9a6ddb73
        testcase( aiMap[i]!=i );
        sqlite3VdbeAddOp3(v, OP_Copy, rLhsOrig+i, rLhs+aiMap[i], 0);
      }
      sqlite3ReleaseTempReg(pParse, rLhsOrig);  // <<< BUG ANCHOR
    }
  }

/* …（同文件无关代码省略）… */
*/
void sqlite3ReleaseTempReg(Parse *pParse, int iReg){
  if( iReg ){
    sqlite3VdbeReleaseRegisters(pParse, iReg, 1, 0, 0);
    if( pParse->nTempReg<ArraySize(pParse->aTempReg) ){
      pParse->aTempReg[pParse->nTempReg++] = iReg;
