// AUTO-DRAFT from sqlite/sqlite PR #90687aa700d1d0a7af8fe95f8a4c841df6fa15ba
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR

  assert( pTerm->eOperator & WO_IN );

  if( sqlite3ExprIsVector(pX->pLeft) ){
    int iField = pTerm->u.x.iField - 1;
    inexpr.flags = 0;
    inexpr.op = TK_EQ;
    inexpr.pLeft = pX->pLeft->x.pList->a[iField].pExpr;
    assert( ExprUseXSelect(pX) );
    inexpr.pRight = pX->x.pSelect->pEList->a[iField].pExpr;
    pX = &inexpr;
  }
