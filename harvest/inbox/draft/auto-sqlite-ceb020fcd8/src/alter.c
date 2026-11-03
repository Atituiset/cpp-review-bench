// AUTO-DRAFT from sqlite/sqlite PR #7b667bbf9fadc419b56b953dbe066afe534e2646
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static void renameTestSchema(
  Parse *pParse,                  /* Parse context */
  const char *zDb,                /* Name of db to verify schema of */
  int bTemp,                      /* True if this is the temp db */
  const char *zWhen,              /* "when" part of error message */
  int bNoDQS                      /* Do not allow DQS in the schema */
){
  pParse->colNamesSet = 1;
  sqlite3NestedParse(pParse,
      "SELECT 1 "
      "FROM \"%w\"." LEGACY_SCHEMA_TABLE " "
      "WHERE name NOT LIKE 'sqliteX_%%' ESCAPE 'X'"
      " AND sql NOT LIKE 'create virtual%%'"
      " AND sqlite_rename_test(%Q, sql, type, name, %d, %Q, %d)=NULL ",
      zDb,
      zDb, bTemp, zWhen, bNoDQS
  );

  if( bTemp==0 ){
    sqlite3NestedParse(pParse,
        "SELECT 1 "
        "FROM temp." LEGACY_SCHEMA_TABLE " "
        "WHERE name NOT LIKE 'sqliteX_%%' ESCAPE 'X'"
        " AND sql NOT LIKE 'create virtual%%'"
        " AND sqlite_rename_test(%Q, sql, type, name, 1, %Q, %d)=NULL ",
        zDb, zWhen, bNoDQS
    );
  }
}
/* …（同文件无关代码省略）… */
  renameTestSchema(pParse, zDb, iDb==1, "after drop column", 1);

  /* Edit rows of table on disk */
  if( pParse->nErr==0 && (pTab->aCol[iCol].colFlags & COLFLAG_VIRTUAL)==0 ){
    int i;
    int addr;
    int reg;
    int regRec;
    Index *pPk = 0;
    int nField = 0;               /* Number of non-virtual columns after drop */
    int iCur;
    Vdbe *v = sqlite3GetVdbe(pParse);
    iCur = pParse->nTab++;
    sqlite3OpenTable(pParse, iCur, iDb, pTab, OP_OpenWrite);
    addr = sqlite3VdbeAddOp1(v, OP_Rewind, iCur); VdbeCoverage(v);
    reg = ++pParse->nMem;
    if( HasRowid(pTab) ){
      sqlite3VdbeAddOp2(v, OP_Rowid, iCur, reg);
      pParse->nMem += pTab->nCol;
    }else{
      pPk = sqlite3PrimaryKeyIndex(pTab);
      pParse->nMem += pPk->nColumn;
      for(i=0; i<pPk->nKeyCol; i++){
        sqlite3VdbeAddOp3(v, OP_Column, iCur, i, reg+i+1);
      }
      nField = pPk->nKeyCol;
    }
    regRec = ++pParse->nMem;
    for(i=0; i<pTab->nCol; i++){
      if( i!=iCol && (pTab->aCol[i].colFlags & COLFLAG_VIRTUAL)==0 ){
        int regOut;
        if( pPk ){
          int iPos = sqlite3TableColumnToIndex(pPk, i);
          int iColPos = sqlite3TableColumnToIndex(pPk, iCol);
          if( iPos<pPk->nKeyCol ) continue;
          regOut = reg+1+iPos-(iPos>iColPos);
        }else{
          regOut = reg+1+nField;
        }
        if( i==pTab->iPKey ){
          sqlite3VdbeAddOp2(v, OP_Null, 0, regOut);
        }else{
          sqlite3ExprCodeGetColumnOfTable(v, pTab, iCur, i, regOut);
          assert( pParse->nErr
               || sqlite3VdbeGetOp(v,sqlite3VdbeCurrentAddr(v)-1)->opcode
                    != OP_RealAffinity
               || pTab->aCol[i].affinity==SQLITE_AFF_REAL );
          sqlite3VdbeDeletePriorOpcode(v, OP_RealAffinity);
        }
        nField++;
      }
    }
    if( nField==0 ){
      /* dbsqlfuzz 5f09e7bcc78b4954d06bf9f2400d7715f48d1fef */
      pParse->nMem++;
      sqlite3VdbeAddOp2(v, OP_Null, 0, reg+1);
      nField = 1;
    }
    sqlite3VdbeAddOp3(v, OP_MakeRecord, reg+1, nField, regRec);
    if( pPk ){
      sqlite3VdbeAddOp4Int(v, OP_IdxInsert, iCur, regRec, reg+1, pPk->nKeyCol);
    }else{
      sqlite3VdbeAddOp3(v, OP_Insert, iCur, regRec, reg);
    }
    sqlite3VdbeChangeP5(v, OPFLAG_SAVEPOSITION);

    sqlite3VdbeAddOp2(v, OP_Next, iCur, addr+1); VdbeCoverage(v);
    sqlite3VdbeJumpHere(v, addr);
  }

exit_drop_column:
