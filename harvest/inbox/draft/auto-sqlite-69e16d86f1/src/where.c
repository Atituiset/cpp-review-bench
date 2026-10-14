// AUTO-DRAFT from sqlite/sqlite PR #9919c36b55bf5d37ad8eb67270c001b1d00db0e9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <string.h>
  // <<< BUG ANCHOR
static void whereIndexedExprCleanup(sqlite3 *db, void *pObject){
  IndexedExpr **pp = (IndexedExpr**)pObject;
  while( *pp!=0 ){
    IndexedExpr *p = *pp;
    *pp = p->pIENext;
    sqlite3ExprDelete(db, p->pExpr);
    sqlite3DbFreeNN(db, p);
  }
}
/* …（同文件无关代码省略）… */
static void wherePartIdxExpr(
  Parse *pParse,                  /* Parse context */
  Index *pIdx,                    /* Partial index being processed */
  Expr *pPart,                    /* WHERE clause being processed */
  Bitmask *pMask,                 /* Mask to clear bits in */
  int iIdxCur,                    /* Cursor number for index */
  SrcItem *pItem                  /* The FROM clause entry for the table */
){
  assert( pItem==0 || (pItem->fg.jointype & JT_RIGHT)==0 );
  assert( (pItem==0 || pMask==0) && (pMask!=0 || pItem!=0) );

  if( pPart->op==TK_AND ){
    wherePartIdxExpr(pParse, pIdx, pPart->pRight, pMask, iIdxCur, pItem);
    pPart = pPart->pLeft;
  }

  if( (pPart->op==TK_EQ || pPart->op==TK_IS) ){
    Expr *pLeft = pPart->pLeft;
    Expr *pRight = pPart->pRight;
    Table *pLeftTab;            /* Table of the left operand */
    Column *pLeftCol;           /* Column of the left operand */
    u8 affRight, affLeft;

    if( pLeft->op!=TK_COLUMN ) return;
    if( !sqlite3ExprIsConstant(0, pRight) ) return;
    if( !sqlite3IsBinary(sqlite3ExprCompareCollSeq(pParse, pPart)) ) return;
    if( pLeft->iColumn<0 ) return;
    pLeftTab = pIdx->pTable;
    pLeftCol = &pLeftTab->aCol[pLeft->iColumn];
    affLeft = pLeftCol->affinity;
    assert( SQLITE_AFF_BLOB<SQLITE_AFF_TEXT );
    if( affLeft<SQLITE_AFF_TEXT ) return;
    affRight = sqlite3ExprAffinity(pRight);
    if( affRight!=0 && affRight!=affLeft ) return;
    if( pItem ){
      sqlite3 *db = pParse->db;
      IndexedExpr *p = (IndexedExpr*)sqlite3DbMallocRaw(db, sizeof(*p));
      if( p ){
        int bNullRow = (pItem->fg.jointype&(JT_LEFT|JT_LTORJ))!=0;
        pRight = sqlite3ExprDup(db, pRight, 0);
        if( (pLeftTab->tabFlags & TF_Strict)!=0 ){
          Token t;
          t.z = sqlite3ColumnType(pLeftCol, 0);
          assert( t.z!=0 );  /* Because affLeft>=SQLITE_AFF_TEXT */
          t.n = (int)strlen(t.z);
          p->pExpr = sqlite3ExprAlloc(db, TK_CAST, &t, 1);
          sqlite3ExprAttachSubtrees(db, p->pExpr, pRight, 0);
        }else{
          p->pExpr = pRight;
        }
        p->iDataCur = pItem->iCursor;
        p->iIdxCur = iIdxCur;
        p->iIdxCol = pLeft->iColumn;
        p->bMaybeNullRow = bNullRow;
        p->pIENext = pParse->pIdxPartExpr;
        p->aff = affLeft;
        pParse->pIdxPartExpr = p;
        if( p->pIENext==0 ){
          void *pArg = (void*)&pParse->pIdxPartExpr;
          sqlite3ParserAddCleanup(pParse, whereIndexedExprCleanup, pArg);
        }
      }
    }else if( pLeft->iColumn<(BMS-1) ){
      *pMask &= ~((Bitmask)1 << pLeft->iColumn);
    }
  }
}
/* …（同文件无关代码省略）… */
        pNew->wsFlags = WHERE_IDX_ONLY | WHERE_INDEXED;
      }else{
        m = pSrc->colUsed & pProbe->colNotIdxed;
        if( pProbe->pPartIdxWhere ){
          wherePartIdxExpr(
              pWInfo->pParse, pProbe, pProbe->pPartIdxWhere, &m, 0, 0
          );
