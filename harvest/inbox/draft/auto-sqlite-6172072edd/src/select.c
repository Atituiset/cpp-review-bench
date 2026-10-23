// AUTO-DRAFT from sqlite/sqlite PR #31ebfdf4a1483218a7e17499957eca4caabce915
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>

static SrcItem *isSelfJoinView(
  SrcList *pTabList,           /* Search for self-joins in this FROM clause */
  SrcItem *pThis,              /* Search for prior reference to this subquery */
  int iFirst, int iEnd        /* Range of FROM-clause entries to search. */
){
  SrcItem *pItem;
  Select *pSel;
  assert( pThis->fg.isSubquery );
  pSel = pThis->u4.pSubq->pSelect;
  assert( pSel!=0 );
  if( pSel->selFlags & SF_PushDown ) return 0;
  while( iFirst<iEnd ){
    Select *pS1;
    pItem = &pTabList->a[iFirst++];
    if( !pItem->fg.isSubquery ) continue;
    if( pItem->fg.viaCoroutine ) continue;
    if( pItem->zName==0 ) continue;
    assert( pItem->pSTab!=0 );
    assert( pThis->pSTab!=0 );
    if( pItem->pSTab->pSchema!=pThis->pSTab->pSchema ) continue;
    if( sqlite3_stricmp(pItem->zName, pThis->zName)!=0 ) continue;
    pS1 = pItem->u4.pSubq->pSelect;
    if( pItem->pSTab->pSchema==0 && pSel->selId!=pS1->selId ){
      /* The query flattener left two different CTE tables with identical
      ** names in the same FROM clause. */
      continue;
    }
    if( pS1->selFlags & SF_PushDown ){
      /* The view was modified by some other optimization such as
      ** pushDownWhereTerms() */
      continue;
    }
    return pItem;
  }
  return 0;
}
/* …（同文件无关代码省略）… */
      ** then make the pItem->iCursor be a copy of the ephemeral table that
      ** holds the result of the materialization. tag-select-0484 */
      CteUse *pCteUse = pItem->u2.pCteUse;
      sqlite3VdbeAddOp2(v, OP_Gosub, pCteUse->regRtn, pCteUse->addrM9e);
      if( pItem->iCursor!=pCteUse->iCur ){
        sqlite3VdbeAddOp2(v, OP_OpenDup, pItem->iCursor, pCteUse->iCur);
        VdbeComment((v, "%!S", pItem));
      }
      pSub->nSelectRow = pCteUse->nRowEst;
    }else if( (pPrior = isSelfJoinView(pTabList, pItem, 0, i))!=0 ){
      /* This view has already been materialized by a prior entry in
