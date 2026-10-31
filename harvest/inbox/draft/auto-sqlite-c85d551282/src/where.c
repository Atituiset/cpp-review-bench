// AUTO-DRAFT from sqlite/sqlite PR #c3351529909a3ad7135b8635fe824977ff5c30bc
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <string.h>
  // <<< BUG ANCHOR
struct CoveringIndexCheck {
  Index *pIdx;       /* The index */
  int iTabCur;       /* Cursor number for the corresponding table */
  u8 bExpr;          /* Uses an indexed expression */
  u8 bUnidx;         /* Uses an unindexed column not within an indexed expr */
};

/*
** Information passed in is pWalk->u.pCovIdxCk.  Call it pCk.
**
/* …（同文件无关代码省略）… */

  pCk = pWalk->u.pCovIdxCk;
  pIdx = pCk->pIdx;
  if( (pExpr->op==TK_COLUMN || pExpr->op==TK_AGG_COLUMN) ){
    /* if( pExpr->iColumn<(BMS-1) && pIdx->bHasExpr==0 ) return WRC_Continue;*/
    if( pExpr->iTable!=pCk->iTabCur ) return WRC_Continue;
/* …（同文件无关代码省略）… */
** a covering index by using the colUsed bitmasks.  We have to do a search
** to see if the index is covering.  This routine does that search.
**
** The return value is one of these:
**
**      0                The index is definitely not a covering index
/* …（同文件无关代码省略）… */
static SQLITE_NOINLINE u32 whereIsCoveringIndex(
  WhereInfo *pWInfo,     /* The WHERE clause context */
  Index *pIdx,           /* Index that is being tested */
  int iTabCur            /* Cursor for the table being indexed */
){
  int i, rc;
  struct CoveringIndexCheck ck;
/* …（同文件无关代码省略）… */
    ** if pIdx is covering.  Assume it is not. */
    return 0;
  }
  if( pIdx->bHasExpr==0 ){
    for(i=0; i<pIdx->nColumn; i++){
      if( pIdx->aiColumn[i]>=BMS-1 ) break;
    }
/* …（同文件无关代码省略）… */
  }
  ck.pIdx = pIdx;
  ck.iTabCur = iTabCur;
  ck.bExpr = 0;
  ck.bUnidx = 0;
  memset(&w, 0, sizeof(w));
/* …（同文件无关代码省略）… */
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
      if( rc ) break;
    }else{
      Bitmask m;
      if( pProbe->isCovering ){
        m = 0;
        pNew->wsFlags = WHERE_IDX_ONLY | WHERE_INDEXED;
/* …（同文件无关代码省略）… */
          wherePartIdxExpr(
              pWInfo->pParse, pProbe, pProbe->pPartIdxWhere, &m, 0, 0
          );
        }
        pNew->wsFlags = WHERE_INDEXED;
        if( m==TOPBIT || (pProbe->bHasExpr && !pProbe->bHasVCol && m!=0) ){
          u32 isCov = whereIsCoveringIndex(pWInfo, pProbe, pSrc->iCursor);
          if( isCov==0 ){
            WHERETRACE(0x200,
               ("-> %s is not a covering index"
