// AUTO-DRAFT from sqlite/sqlite PR #6590ca855f645a1d163ba99eae4a611acb102f30
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
char sqlite3TableColumnAffinity(const Table *pTab, int iCol){
  if( iCol<0 || NEVER(iCol>=pTab->nCol) ) return SQLITE_AFF_INTEGER;
  return pTab->aCol[iCol].affinity;
}
/* …（同文件无关代码省略）… */
char sqlite3ExprAffinity(const Expr *pExpr){
  int op;
  op = pExpr->op;
  while( 1 /* exit-by-break */ ){
    if( op==TK_COLUMN || (op==TK_AGG_COLUMN && pExpr->y.pTab!=0) ){
      assert( ExprUseYTab(pExpr) );
      assert( pExpr->y.pTab!=0 );
      return sqlite3TableColumnAffinity(pExpr->y.pTab, pExpr->iColumn);
    }
    if( op==TK_SELECT ){
      assert( ExprUseXSelect(pExpr) );
      assert( pExpr->x.pSelect!=0 );
      assert( pExpr->x.pSelect->pEList!=0 );
      assert( pExpr->x.pSelect->pEList->a[0].pExpr!=0 );
      return sqlite3ExprAffinity(pExpr->x.pSelect->pEList->a[0].pExpr);
    }
#ifndef SQLITE_OMIT_CAST
    if( op==TK_CAST ){
      assert( !ExprHasProperty(pExpr, EP_IntValue) );
      return sqlite3AffinityType(pExpr->u.zToken, 0);
    }
#endif
    if( op==TK_SELECT_COLUMN ){
      assert( pExpr->pLeft!=0 && ExprUseXSelect(pExpr->pLeft) );
      assert( pExpr->iColumn < pExpr->iTable );
      assert( pExpr->iColumn >= 0 );
      assert( pExpr->iTable==pExpr->pLeft->x.pSelect->pEList->nExpr );
      return sqlite3ExprAffinity(
          pExpr->pLeft->x.pSelect->pEList->a[pExpr->iColumn].pExpr
      );
    }
    if( op==TK_VECTOR
     || (op==TK_FUNCTION && pExpr->affExpr==SQLITE_AFF_DEFER)
    ){
      assert( ExprUseXList(pExpr) );
      return sqlite3ExprAffinity(pExpr->x.pList->a[0].pExpr);
    }
    if( ExprHasProperty(pExpr, EP_Skip|EP_IfNullRow) ){
      assert( pExpr->op==TK_COLLATE
           || pExpr->op==TK_IF_NULL_ROW
           || (pExpr->op==TK_REGISTER && pExpr->op2==TK_IF_NULL_ROW) );
      pExpr = pExpr->pLeft;
      op = pExpr->op;
      continue;
    }
    if( op!=TK_REGISTER ) break;
    op = pExpr->op2;
    if( NEVER( op==TK_REGISTER ) ) break;
  }
  return pExpr->affExpr;
}
