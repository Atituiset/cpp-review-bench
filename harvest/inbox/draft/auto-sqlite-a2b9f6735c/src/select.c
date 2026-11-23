// AUTO-DRAFT from sqlite/sqlite PR #dc9b3226d01620a2087811a142f9cfb03b22728c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
static int countOfViewOptimization(Parse *pParse, Select *p){
  Select *pSub, *pPrior;
  Expr *pExpr;
  Expr *pCount;
  sqlite3 *db;
  SrcItem *pFrom;
  if( (p->selFlags & SF_Aggregate)==0 ) return 0;   /* This is an aggregate */
  if( p->pEList->nExpr!=1 ) return 0;               /* Single result column */
  if( p->pWhere ) return 0;
  if( p->pHaving ) return 0;
  if( p->pGroupBy ) return 0;
  if( p->pOrderBy ) return 0;
  pExpr = p->pEList->a[0].pExpr;
  if( pExpr->op!=TK_AGG_FUNCTION ) return 0;        /* Result is an aggregate */
  assert( ExprUseUToken(pExpr) );
  if( sqlite3_stricmp(pExpr->u.zToken,"count") ) return 0;  /* Is count() */
  assert( ExprUseXList(pExpr) );
  if( pExpr->x.pList!=0 ) return 0;                 /* Must be count(*) */
  if( p->pSrc->nSrc!=1 ) return 0;                  /* One table in FROM  */
  if( ExprHasProperty(pExpr, EP_WinFunc) ) return 0;/* Not a window function */
  pFrom = p->pSrc->a;
  if( pFrom->fg.isSubquery==0 ) return 0;           /* FROM is a subquery */
  if( (p->selFlags & SF_Correlated)!=0 ) return 0;  /* Not a correlated subq */
  pSub = pFrom->u4.pSubq->pSelect;
  if( pSub->pPrior==0 ) return 0;                   /* Must be a compound */
  if( pSub->selFlags & SF_CopyCte ) return 0;       /* Not a CTE */
  do{
    if( pSub->op!=TK_ALL && pSub->pPrior ) return 0;  /* Must be UNION ALL */
    if( pSub->pWhere ) return 0;                      /* No WHERE clause */
    if( pSub->pLimit ) return 0;                      /* No LIMIT clause */
#ifndef SQLITE_OMIT_WINDOWFUNC
    if( pSub->pWin ) return 0;                        /* No window functions */
#endif
    if( pSub->selFlags & (SF_Aggregate|SF_Distinct) ){
       testcase( pSub->selFlags & SF_Aggregate );
       testcase( pSub->selFlags & SF_Distinct );
       return 0;     /* Not an aggregate nor DISTINCT */
    }
    assert( pSub->pHaving==0 );  /* Due to the previous */
    pSub = pSub->pPrior;                              /* Repeat over compound */
  }while( pSub );

  /* If we reach this point then it is OK to perform the transformation */

  db = pParse->db;
  pCount = pExpr;
  pExpr = 0;
  pSub = sqlite3SubqueryDetach(db, pFrom);
  sqlite3SrcListDelete(db, p->pSrc);
  p->pSrc = sqlite3DbMallocZero(pParse->db, SZ_SRCLIST_1);
  while( pSub ){
    Expr *pTerm;
    pPrior = pSub->pPrior;
    pSub->pPrior = 0;
    pSub->pNext = 0;
    pSub->selFlags |= SF_Aggregate;
    pSub->selFlags &= ~(u32)SF_Compound;
    pSub->nSelectRow = 0;
    sqlite3ParserAddCleanup(pParse, sqlite3ExprListDeleteGeneric, pSub->pEList);
    pTerm = pPrior ? sqlite3ExprDup(db, pCount, 0) : pCount;
    pSub->pEList = sqlite3ExprListAppend(pParse, 0, pTerm);
    pTerm = sqlite3PExpr(pParse, TK_SELECT, 0, 0);
    sqlite3PExprAddSelect(pParse, pTerm, pSub);
    if( pExpr==0 ){
      pExpr = pTerm;
    }else{
      pExpr = sqlite3PExpr(pParse, TK_PLUS, pTerm, pExpr);
    }
    if( pParse->nErr ){
      sqlite3ExprDelete(db, pExpr);
      pExpr = 0;
    }
    pSub = pPrior;
  }
  p->pEList->a[0].pExpr = pExpr;
  p->selFlags &= ~(u32)SF_Aggregate;

#if TREETRACE_ENABLED
  if( sqlite3TreeTrace & 0x200 ){
    TREETRACE(0x200,pParse,p,("After count-of-view optimization:\n"));
    sqlite3TreeViewSelect(0, p, 0);
  }
#endif
  return 1;
}
/* …（同文件无关代码省略）… */
  }

  /* tag-select-0350 */
  if( OptimizationEnabled(db, SQLITE_QueryFlattener|SQLITE_CountOfView)
   && countOfViewOptimization(pParse, p)
  ){
    if( db->mallocFailed ) goto select_end;
