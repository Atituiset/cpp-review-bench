// AUTO-DRAFT from sqlite/sqlite PR #a5ae4bd098c4d727c6b237fd0994e949c7aa1d15
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <string.h>
  // <<< BUG ANCHOR
      p->selFlags |= SF_OnToWhere;
    }

    if( pRight->fg.isTabFunc && joinType==EP_OuterON && pRight->u1.pFuncArg ){
      p->selFlags |= SF_OnToWhere;
    }
  }
/* …（同文件无关代码省略）… */
typedef struct CheckOnCtx CheckOnCtx;
struct CheckOnCtx {
  SrcList *pSrc;       /* SrcList for this context */
  int iJoin;           /* Cursors must be left of this one, if not zero */
  int bFuncArg;        /* True for table-function arg */
  CheckOnCtx *pParent; /* Parent context */
};
/* …（同文件无关代码省略）… */
    ** set it to the cursor number of the RHS of the join to which this
    ** ON expression was attached and then iterate through the entire 
    ** expression.  */
    assert( pCtx->iJoin==0 || pCtx->iJoin==pExpr->w.iJoin );
    if( pCtx->iJoin==0 ){
      pCtx->iJoin = pExpr->w.iJoin;
      sqlite3WalkExprNN(pWalker, pExpr);
      pCtx->iJoin = 0;
      return WRC_Prune;
    }
  }
/* …（同文件无关代码省略）… */
      for(ii=0; ii<nSrc && pSrc->a[ii].iCursor!=iTab; ii++){}
      if( ii<nSrc ){
        /* pSrc is the FROM clause that contains iTab */
        if( pCtx->iJoin ){
          for(ii--; ii>=0 && pSrc->a[ii].iCursor!=pCtx->iJoin; ii--){}
          if( ii>=0 ){
            /* Table iJoin appears to the left of table iTab in the SrcList.
/* …（同文件无关代码省略）… */
    memset(&sCtx, 0, sizeof(sCtx));
    sCtx.pSrc = pSelect->pSrc;
    sCtx.pParent = pCtx;
    pWalker->u.pCheckOnCtx = &sCtx;
    sqlite3WalkSelect(pWalker, pSelect);
    pWalker->u.pCheckOnCtx = pCtx;
/* …（同文件无关代码省略）… */
  w.u.pCheckOnCtx = &sCtx;
  memset(&sCtx, 0, sizeof(sCtx));
  sCtx.pSrc = pSelect->pSrc;
  sqlite3WalkExpr(&w, pSelect->pWhere);
  pSelect->selFlags &= ~SF_OnToWhere;

/* …（同文件无关代码省略）… */
  for(ii=0; ii<pSelect->pSrc->nSrc; ii++){
    SrcItem *pItem = &pSelect->pSrc->a[ii];
    if( pItem->fg.isTabFunc
     && (pItem->fg.jointype & JT_OUTER)
    ){
      sCtx.iJoin = pItem->iCursor;
      sqlite3WalkExprList(&w, pItem->u1.pFuncArg);
