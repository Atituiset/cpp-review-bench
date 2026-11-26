// AUTO-DRAFT from sqlite/sqlite PR #b3b913b8db89055855103c6f0dd671b778c68dbb
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct DistinctCtx DistinctCtx;
/* …（同文件无关代码省略）… */
typedef struct SortCtx SortCtx;
/* …（同文件无关代码省略）… */
#define SORTFLAG_UseSorter  0x01   /* Use SorterOpen instead of OpenEphemeral */
/* …（同文件无关代码省略）… */
static void clearSelect(sqlite3 *db, Select *p, int bFree){
  assert( db!=0 );
  while( p ){
    Select *pPrior = p->pPrior;
    sqlite3ExprListDelete(db, p->pEList);
    sqlite3SrcListDelete(db, p->pSrc);
    sqlite3ExprDelete(db, p->pWhere);
    sqlite3ExprListDelete(db, p->pGroupBy);
    sqlite3ExprDelete(db, p->pHaving);
    sqlite3ExprListDelete(db, p->pOrderBy);
    sqlite3ExprDelete(db, p->pLimit);
    if( OK_IF_ALWAYS_TRUE(p->pWith) ) sqlite3WithDelete(db, p->pWith);
#ifndef SQLITE_OMIT_WINDOWFUNC
    if( OK_IF_ALWAYS_TRUE(p->pWinDefn) ){
      sqlite3WindowListDelete(db, p->pWinDefn);
    }
    while( p->pWin ){
      assert( p->pWin->ppThis==&p->pWin );
      sqlite3WindowUnlinkFromSelect(p->pWin);
    }
#endif
    if( bFree ) sqlite3DbNNFreeNN(db, p);
    p = pPrior;
    bFree = 1;
  }
}
/* …（同文件无关代码省略）… */
void sqlite3SelectDestInit(SelectDest *pDest, int eDest, int iParm){
  pDest->eDest = (u8)eDest;
  pDest->iSDParm = iParm;
  pDest->iSDParm2 = 0;
  pDest->zAffSdst = 0;
  pDest->iSdst = 0;
  pDest->nSdst = 0;
}
/* …（同文件无关代码省略）… */
void sqlite3SelectDelete(sqlite3 *db, Select *p){
  if( OK_IF_ALWAYS_TRUE(p) ) clearSelect(db, p, 1);
}
void sqlite3SelectDeleteGeneric(sqlite3 *db, void *p){
  if( ALWAYS(p) ) clearSelect(db, (Select*)p, 1);
}
/* …（同文件无关代码省略）… */
void sqlite3SetJoinExpr(Expr *p, int iTable, u32 joinFlag){
  assert( joinFlag==EP_OuterON || joinFlag==EP_InnerON );
  while( p ){
    ExprSetProperty(p, joinFlag);
    assert( !ExprHasProperty(p, EP_TokenOnly|EP_Reduced) );
    ExprSetVVAProperty(p, EP_NoReduce);
    p->w.iJoin = iTable;
    if( ExprUseXList(p) ){
      if( p->x.pList ){
        int i;
        for(i=0; i<p->x.pList->nExpr; i++){
          sqlite3SetJoinExpr(p->x.pList->a[i].pExpr, iTable, joinFlag);
        }
      }
    }
    sqlite3SetJoinExpr(p->pLeft, iTable, joinFlag);
    p = p->pRight;
  }
}
/* …（同文件无关代码省略）… */
static void unsetJoinExpr(Expr *p, int iTable, int nullable){
  while( p ){
    if( iTable<0 || (ExprHasProperty(p, EP_OuterON) && p->w.iJoin==iTable) ){
      ExprClearProperty(p, EP_OuterON|EP_InnerON);
      if( iTable>=0 ) ExprSetProperty(p, EP_InnerON);
    }
    if( p->op==TK_COLUMN && p->iTable==iTable && !nullable ){
      ExprClearProperty(p, EP_CanBeNull);
    }
    if( p->op==TK_FUNCTION ){
      assert( ExprUseXList(p) );
      assert( p->pLeft==0 );
      if( p->x.pList ){
        int i;
        for(i=0; i<p->x.pList->nExpr; i++){
          unsetJoinExpr(p->x.pList->a[i].pExpr, iTable, nullable);
        }
      }
    }
    unsetJoinExpr(p->pLeft, iTable, nullable);
    p = p->pRight;
  }
}
/* …（同文件无关代码省略）… */
static int clearOuterOnExprCb(Walker *pWalker, Expr *p){
  if( ExprHasProperty(p, EP_OuterON) && p->w.iJoin==pWalker->u.iCur ){
    ExprClearProperty(p, EP_OuterON|EP_InnerON);
  }
  return WRC_Continue;
}
/* …（同文件无关代码省略）… */
static void clearOuterOnExpr(Expr *p, int iTable){
  Walker w;
  memset(&w, 0, sizeof(w));
  w.u.iCur = iTable;
  w.xExprCallback = clearOuterOnExprCb;
  w.xSelectCallback = 0;
  sqlite3WalkExpr(&w,p);
}
/* …（同文件无关代码省略）… */
typedef struct RowLoadInfo RowLoadInfo;
/* …（同文件无关代码省略）… */
static void innerLoopLoadRow(
  Parse *pParse,             /* Statement under construction */
  Select *pSelect,           /* The query being coded */
  RowLoadInfo *pInfo         /* Info needed to complete the row load */
){
  sqlite3ExprCodeExprList(pParse, pSelect->pEList, pInfo->regResult,
                          0, pInfo->ecelFlags);
#ifdef SQLITE_ENABLE_SORTER_REFERENCES
  if( pInfo->pExtra ){
    sqlite3ExprCodeExprList(pParse, pInfo->pExtra, pInfo->regExtraResult, 0, 0);
    sqlite3ExprListDelete(pParse->db, pInfo->pExtra);
  }
#endif
}
/* …（同文件无关代码省略）… */
static int makeSorterRecord(
  Parse *pParse,
  SortCtx *pSort,
  Select *pSelect,
  int regBase,
  int nBase
){
  int nOBSat = pSort->nOBSat;
  Vdbe *v = pParse->pVdbe;
  int regOut = ++pParse->nMem;
  if( pSort->pDeferredRowLoad ){
    innerLoopLoadRow(pParse, pSelect, pSort->pDeferredRowLoad);
  }
  sqlite3VdbeAddOp3(v, OP_MakeRecord, regBase+nOBSat, nBase-nOBSat, regOut);
  return regOut;
}
/* …（同文件无关代码省略）… */
static void pushOntoSorter(
  Parse *pParse,         /* Parser context */
  SortCtx *pSort,        /* Information about the ORDER BY clause */
  Select *pSelect,       /* The whole SELECT statement */
  int regData,           /* First register holding data to be sorted */
  int regOrigData,       /* First register holding data before packing */
  int nData,             /* Number of elements in the regData data array */
  int nPrefixReg         /* No. of reg prior to regData available for use */
){
  Vdbe *v = pParse->pVdbe;                         /* Stmt under construction */
  int bSeq = ((pSort->sortFlags & SORTFLAG_UseSorter)==0);
  int nExpr = pSort->pOrderBy->nExpr;              /* No. of ORDER BY terms */
  int nBase = nExpr + bSeq + nData;                /* Fields in sorter record */
  int regBase;                                     /* Regs for sorter record */
  int regRecord = 0;                               /* Assembled sorter record */
  int nOBSat = pSort->nOBSat;                      /* ORDER BY terms to skip */
  int op;                            /* Opcode to add sorter record to sorter */
  int iLimit;                        /* LIMIT counter */
  int iSkip = 0;                     /* End of the sorter insert loop */

  assert( bSeq==0 || bSeq==1 );

  /* Three cases:
  **   (1) The data to be sorted has already been packed into a Record
  **       by a prior OP_MakeRecord.  In this case nData==1 and regData
  **       will be completely unrelated to reg
