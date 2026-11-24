// AUTO-DRAFT from sqlite/sqlite PR #67a23f5ce761fcce65029c4d7d555614932e54b8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
static int integrityCheckResultRow(Vdbe *v, int iCur, int iCol){
  int addr;
  sqlite3VdbeAddOp2(v, OP_ResultRow, 3, 1);
  addr = sqlite3VdbeAddOp3(v, OP_IfPos, 1, sqlite3VdbeCurrentAddr(v)+2, 1);
  VdbeCoverage(v);
  sqlite3VdbeAddOp0(v, OP_Halt);
  if( iCol>=0 ){
    sqlite3VdbeAddOp3(v, OP_Column, iCur, iCol, 3);
    sqlite3VdbeTypeofColumn(v, 3);
  }
  return addr;
}
/* …（同文件无关代码省略）… */
          if( pIdx->pPartIdxWhere ) nPartIdx++;
        }
        if( !IsOrdinaryTable(pTab) ) continue;
        if( isQuick || HasRowid(pTab) ){
          pPk = 0;
          r2 = 0;
        }else{
/* …（同文件无关代码省略）… */
        sqlite3VdbeAddOp3(v, OP_Column, iDataCur, mxCol, 3);
        sqlite3VdbeTypeofColumn(v, 3);

        if( !isQuick ){
          if( pPk ){
            /* Verify WITHOUT ROWID keys are in ascending order */
            int a1;
            char *zErr;
            a1 = sqlite3VdbeAddOp4Int(v, OP_IdxGT, iDataCur, 0,r2,pPk->nKeyCol);
            VdbeCoverage(v);
            sqlite3VdbeAddOp1(v, OP_IsNull, r2); VdbeCoverage(v);
            zErr = sqlite3MPrintf(db,
                   "row not in PRIMARY KEY order for %s",
                    pTab->zName);
            sqlite3VdbeAddOp4(v, OP_String8, 0, 3, 0, zErr, P4_DYNAMIC);
            integrityCheckResultRow(v, iDataCur, mxCol);
            sqlite3VdbeJumpHere(v, a1);
            sqlite3VdbeJumpHere(v, a1+1);
            for(j=0; j<pPk->nKeyCol; j++){
              sqlite3ExprCodeLoadIndexColumn(pParse, pPk, iDataCur, j, r2+j);
            }
          }
        }
        /* Verify datatypes for all columns:
/* …（同文件无关代码省略）… */
          sqlite3ReleaseTempRange(pParse, regPartCnt, nPartIdx);
        }
        if( pPk ){
          assert( !isQuick );
          sqlite3ReleaseTempRange(pParse, r2, pPk->nKeyCol);
        }
      }
