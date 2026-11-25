// AUTO-DRAFT from sqlite/sqlite PR #1fbc68ef3f950ff2a9b54424232ce05f5817d6de
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
  // <<< BUG ANCHOR
  return 0;
}

/*
** pTerm is a WO_EQUIV term "X=Y" that whereScanNext() might use to add
** Y to the equivalence class of X. Return true if any of the following
/* …（同文件无关代码省略）… */
                }
                assert(pX->pLeft);
                if( pX->pLeft->op==TK_VECTOR ){
                  /* A vector inequality such as (a,b)>(?,?). This term
                  ** constrains only the first field, so the collation is
                  ** that of the first pair of fields only. */
                  Expr *pL = sqlite3VectorFieldSubexpr(pX->pLeft, 0);
                  Expr *pR = sqlite3VectorFieldSubexpr(pX->pRight, 0);
                  assert( pTerm->u.x.iField==0 );
                  assert( pX->op>=TK_GT && pX->op<=TK_GE );
                  if( ExprHasProperty(pX, EP_Commuted) ) SWAP(Expr*, pL, pR);
                  pColl = sqlite3BinaryCompareCollSeq(pParse, pL, pR);
                }else{
                  pColl = sqlite3ExprCompareCollSeq(pParse, pX);
                }
/* …（同文件无关代码省略）… */
static WhereTerm *termFromWhereClause(WhereClause *pWC, int iTerm){
  WhereClause *p;
  for(p=pWC; p; p=p->pOuter){
    if( iTerm<p->nTerm ) return &p->a[iTerm];
    iTerm -= p->nTerm;
  }
  return 0;
}
/* …（同文件无关代码省略）… */
    int iTerm = pIdxInfo->aConstraint[iCons].iTermOffset;
    Expr *pX = termFromWhereClause(pHidden->pWC, iTerm)->pExpr;
    if( pX->pLeft ){
      pC = sqlite3ExprCompareCollSeq(pHidden->pParse, pX);
    }
    zRet = (pC ? pC->zName : sqlite3StrBINARY);
  }
