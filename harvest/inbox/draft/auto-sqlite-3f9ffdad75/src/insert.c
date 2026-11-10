// AUTO-DRAFT from sqlite/sqlite PR #a8bdc0ba6bf7108f5ca872d0b91bf60265ab11d1
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
static int exprListIsNoAffinity(Parse *pParse, ExprList *pRow){
  int ii;
  if( sqlite3ExprListIsConstant(pParse, pRow, 0)==0 ) return 0;
  for(ii=0; ii<pRow->nExpr; ii++){
    Expr *pExpr = pRow->a[ii].pExpr;
    assert( pExpr->op!=TK_RAISE );
    assert( pExpr->affExpr==0 );
    if( 0!=sqlite3ExprAffinity(pExpr) ) return 0;
  }
  return 1;

}

/*
** This function is called by the parser for the second and subsequent
** rows of a multi-row VALUES clause. Argument pLeft is the part of
/* …（同文件无关代码省略）… */
**       because the complex rules SQLite uses (see function 
**       sqlite3SubqueryColumnTypes() in select.c) to determine the effective
**       affinity of such a column for all rows require access to all values in
**       the column simultaneously. 
*/
Select *sqlite3MultiValues(Parse *pParse, Select *pLeft, ExprList *pRow){

/* …（同文件无关代码省略）… */
   || sqlite3ExprListIsConstant(pParse, pRow, 1)==0   /* condition (c) above */
   || (pLeft->pSrc->nSrc==0 &&
       exprListIsNoAffinity(pParse,pLeft->pEList)==0) /* condition (d) above */
   || IN_SPECIAL_PARSE
  ){
    /* The co-routine method cannot be used. Fall back to UNION ALL. */
/* …（同文件无关代码省略）… */
        pRet->pSrc->nSrc = 1;
        pRet->pPrior = pLeft->pPrior;
        pRet->op = pLeft->op;
        if( pRet->pPrior ) pRet->selFlags |= SF_Values;
        pLeft->pPrior = 0;
        pLeft->op = TK_SELECT;
        assert( pLeft->pNext==0 );
/* …（同文件无关代码省略）… */
  ** single row (the common case) then keep that one row of values
  ** and discard the other (unused) parts of the pSelect object
  */
  if( pSelect && (pSelect->selFlags & SF_Values)!=0 && pSelect->pPrior==0 ){
    pList = pSelect->pEList;
    pSelect->pEList = 0;
    sqlite3SelectDelete(db, pSelect);
