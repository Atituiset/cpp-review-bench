// AUTO-DRAFT from sqlite/sqlite PR #c2d796b3f2a277e3dcb8f2905f2908ac1de71cc9
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
/* …（同文件无关代码省略）… */
char sqlite3CompareAffinity(const Expr *pExpr, char aff2){
  char aff1 = sqlite3ExprAffinity(pExpr);
  if( aff1>SQLITE_AFF_NONE && aff2>SQLITE_AFF_NONE ){
    /* Both sides of the comparison are columns. If one has numeric
    ** affinity, use that. Otherwise use no affinity.
    */
    if( sqlite3IsNumericAffinity(aff1) || sqlite3IsNumericAffinity(aff2) ){
      return SQLITE_AFF_NUMERIC;
    }else{
      return SQLITE_AFF_BLOB;
    }
  }else{
    /* One side is a column, the other is not. Use the columns affinity. */
    assert( aff1<=SQLITE_AFF_NONE || aff2<=SQLITE_AFF_NONE );
    return (aff1<=SQLITE_AFF_NONE ? aff2 : aff1) | SQLITE_AFF_NONE;
  }
}
/* …（同文件无关代码省略）… */
          pExpr->op==TK_GT || pExpr->op==TK_GE || pExpr->op==TK_LE ||
          pExpr->op==TK_NE || pExpr->op==TK_IS || pExpr->op==TK_ISNOT );
  assert( pExpr->pLeft );
  aff = sqlite3ExprAffinity(pExpr->pLeft);
  if( pExpr->pRight ){
    aff = sqlite3CompareAffinity(pExpr->pRight, aff);
  }else if( ExprUseXSelect(pExpr) ){
    aff = sqlite3CompareAffinity(pExpr->x.pSelect->pEList->a[0].pExpr, aff);
  }else if( aff==0 ){
    aff = SQLITE_AFF_BLOB;
  }
