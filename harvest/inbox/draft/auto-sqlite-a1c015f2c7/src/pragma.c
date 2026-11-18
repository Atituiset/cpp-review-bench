// AUTO-DRAFT from sqlite/sqlite PR #6abdfa68f496262d4c11e98f056e35284b57b5c3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>

static int tableSkipIntegrityCheck(const Table *pTab, const Table *pObjTab){
  if( pObjTab ){
    return pTab!=pObjTab;
  }else{
    return (pTab->tabFlags & TF_Imposter)!=0;
  }
}
/* …（同文件无关代码省略）… */

      /* Make sure all the indices are constructed correctly.
      */
      for(x=sqliteHashFirst(pTbls); x; x=sqliteHashNext(x)){
        Table *pTab = sqliteHashData(x);
        Index *pIdx, *pPk;
/* …（同文件无关代码省略）… */
        int bStrict;            /* True for a STRICT table */
        int r2;                 /* Previous key for WITHOUT ROWID tables */
        int mxCol;              /* Maximum non-virtual column number */

        if( tableSkipIntegrityCheck(pTab,pObjTab) ) continue;
        if( !IsOrdinaryTable(pTab) ) continue;
        if( isQuick || HasRowid(pTab) ){
          pPk = 0;
/* …（同文件无关代码省略）… */
        */
        sqlite3VdbeAddOp2(v, OP_Integer, 0, 7);
        assert( sqlite3NoTempsInRange(pParse,1,7) );
        sqlite3VdbeAddOp2(v, OP_Rewind, iDataCur, 0); VdbeCoverage(v);
        loopTop = sqlite3VdbeAddOp2(v, OP_AddImm, 7, 1);

/* …（同文件无关代码省略）… */
        }
        if( !isQuick ){ /* Omit the remaining tests for quick_check */
          /* Validate index entries for the current row */
          for(j=0, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, j++){
            int jmp2, jmp3, jmp4, jmp5, label6;
            int kk;
/* …（同文件无关代码省略）… */
            r1 = sqlite3GenerateIndexKey(pParse, pIdx, iDataCur, 0, 0, &jmp3,
                                         pPrior, r1);
            pPrior = pIdx;
            /* Verify that an index entry exists for the current table row */
            sqlite3VdbeAddOp4Int(v, OP_Found, iIdxCur+j, ckUniq, r1,
                                        pIdx->nColumn); VdbeCoverage(v);
/* …（同文件无关代码省略）… */
        }
        sqlite3VdbeAddOp2(v, OP_Next, iDataCur, loopTop); VdbeCoverage(v);
        sqlite3VdbeJumpHere(v, loopTop-1);
        if( pPk ){
          assert( !isQuick );
          sqlite3ReleaseTempRange(pParse, r2, pPk->nKeyCol);
