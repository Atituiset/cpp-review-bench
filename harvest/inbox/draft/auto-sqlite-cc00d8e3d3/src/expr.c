// AUTO-DRAFT from sqlite/sqlite PR #cd9c337091af53b1199c6243681cee5ead7d6718
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
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
  return w.eCode;
}


/*
** Check to see if pExpr is one of the indexed expressions on pParse->pIdxEpr.
/* …（同文件无关代码省略）… */
  IndexedExpr *p;
  Vdbe *v;
  for(p=pParse->pIdxEpr; p; p=p->pIENext){
    u8 exprAff;
    int iDataCur = p->iDataCur;
    if( iDataCur<0 ) continue;
    if( pParse->iSelfTab ){
      if( p->iDataCur!=pParse->iSelfTab-1 ) continue;
      iDataCur = -1;
    }
    if( sqlite3ExprCompare(0, pExpr, p->pExpr, iDataCur)!=0 ) continue;
    assert( p->aff>=SQLITE_AFF_BLOB && p->aff<=SQLITE_AFF_NUMERIC );
    exprAff = sqlite3ExprAffinity(pExpr);
    if( (exprAff<=SQLITE_AFF_BLOB && p->aff!=SQLITE_AFF_BLOB)
     || (exprAff==SQLITE_AFF_TEXT && p->aff!=SQLITE_AFF_TEXT)
     || (exprAff>=SQLITE_AFF_NUMERIC && p->aff!=SQLITE_AFF_NUMERIC)
    ){
      /* Affinity mismatch on a generated column */
      continue;
    }


    /* Functions that might set a subtype should not be replaced by the
    ** value taken from an expression index if they are themselves an
/* …（同文件无关代码省略）… */
static SQLITE_NOINLINE int exprCompareVariable(
  const Parse *pParse,
  const Expr *pVar,
  const Expr *pExpr
){
  int res = 2;
  int iVar;
  sqlite3_value *pL, *pR = 0;
 
  if( pExpr->op==TK_VARIABLE && pVar->iColumn==pExpr->iColumn ){
    return 0;
  }
  if( (pParse->db->flags & SQLITE_EnableQPSG)!=0 ) return 2;
  sqlite3ValueFromExpr(pParse->db, pExpr, SQLITE_UTF8, SQLITE_AFF_BLOB, &pR);
  if( pR ){
    iVar = pVar->iColumn;
    sqlite3VdbeReprepareOnBind(pParse->pVdbe, iVar, 0);
    pL = sqlite3VdbeGetBoundValue(pParse->pReprepare, iVar, SQLITE_AFF_BLOB);
    if( pL ){
      if( sqlite3_value_type(pL)==SQLITE_TEXT ){
        sqlite3_value_text(pL); /* Make sure the encoding is UTF-8 */
      }
      res = sqlite3MemCompare(pL, pR, 0) ? 2 : 0;
    }
    sqlite3ValueFree(pR);
    sqlite3ValueFree(pL);
  }
  return res;
}
/* …（同文件无关代码省略）… */
int sqlite3ExprCompare(
  const Parse *pParse,
  const Expr *pA,
  const Expr *pB,
  int iTab
){
  u32 combinedFlags;
  if( pA==0 || pB==0 ){
    return pB==pA ? 0 : 2;
  }
  if( pParse && pA->op==TK_VARIABLE ){
    return exprCompareVariable(pParse, pA, pB);
  }
  combinedFlags = pA->flags | pB->flags;
  if( combinedFlags & EP_IntValue ){
    if( (pA->flags&pB->flags&EP_IntValue)!=0 && pA->u.iValue==pB->u.iValue ){
      return 0;
    }
    return 2;
  }
  if( pA->op!=pB->op || pA->op==TK_RAISE ){
    if( pA->op==TK_COLLATE && sqlite3ExprCompare(pParse, pA->pLeft,pB,iTab)<2 ){
      return 1;
    }
    if( pB->op==TK_COLLATE && sqlite3ExprCompare(pParse, pA,pB->pLeft,iTab)<2 ){
      return 1;
    }
    if( pA->op==TK_AGG_COLUMN && pB->op==TK_COLUMN
     && pB->iTable<0 && pA->iTable==iTab
    ){
      /* fall through */
    }else{
      return 2;
    }
  }
  assert( !ExprHasProperty(pA, EP_IntValue) );
  assert( !ExprHasProperty(pB, EP_IntValue) );
  if( pA->u.zToken ){
    if( pA->op==TK_FUNCTION || pA->op==TK_AGG_FUNCTION ){
      if( sqlite3StrICmp(pA->u.zToken,pB->u.zToken)!=0 ) return 2;
#ifndef SQLITE_OMIT_WINDOWFUNC
      assert( pA->op==pB->op );
      if( ExprHasProperty(pA,EP_WinFunc)!=ExprHasProperty(pB,EP_WinFunc) ){
        return 2;
      }
      if( ExprHasProperty(pA,EP_WinFunc) ){
        if( sqlite3WindowCompare(pParse, pA->y.pWin, pB->y.pWin, 1)!=0 ){
          return 2;
        }
      }
#endif
    }else if( pA->op==TK_NULL ){
      return 0;
    }else if( pA->op==TK_COLLATE ){
      if( sqlite3_stricmp(pA->u.zToken,pB->u.zToken)!=0 ) return 2;
    }else
    if( pB->u.zToken!=0
     && pA->op!=TK_COLUMN
     && pA->op!=TK_AGG_COLUMN
     && strcmp(pA->u.zToken,pB->u.zToken)!=0
    ){
      return 2;
    }
  }
  if( (pA->flags & (EP_Distinct|EP_Commuted))
     != (pB->flags & (EP_Distinct|EP_Commuted)) ) return 2;
  if( ALWAYS((combinedFlags & EP_TokenOnly)==0) ){
    if( combinedFlags & EP_xIsSelect ) return 2;
    if( (combinedFlags & EP_FixedCol)==0
     && sqlite3ExprCompare(pParse, pA->pLeft, pB->pLeft, iTab) ) return 2;
    if( sqlite
