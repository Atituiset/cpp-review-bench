// AUTO-DRAFT from sqlite/sqlite PR #99774ca0025a977a5364f3b753ef1792022dfbf5
static int whereUsablePartialIndex(
  int iTab,             /* The table for which we want an index */
  u8 jointype,          /* The JT_* flags on the join */
  WhereClause *pWC,     /* The WHERE clause of the query */
  Expr *pWhere          /* The WHERE clause from the partial index */
){  // <<< BUG ANCHOR
  int i;
  WhereTerm *pTerm;
  Parse *pParse;

  if( jointype & JT_LTORJ ) return 0;
  pParse = pWC->pWInfo->pParse;
  while( pWhere->op==TK_AND ){
    if( !whereUsablePartialIndex(iTab,jointype,pWC,pWhere->pLeft) ) return 0;
    pWhere = pWhere->pRight;
  }
  for(i=0, pTerm=pWC->a; i<pWC->nTerm; i++, pTerm++){
    Expr *pExpr;
    pExpr = pTerm->pExpr;
    if( (!ExprHasProperty(pExpr, EP_OuterON) || pExpr->w.iJoin==iTab)
     && ((jointype & JT_OUTER)==0 || ExprHasProperty(pExpr, EP_OuterON))
     && sqlite3ExprImpliesExpr(pParse, pExpr, pWhere, iTab)
     && !sqlite3ExprImpliesExpr(pParse, pExpr, pWhere, -1)
     && (pTerm->wtFlags & TERM_VNULL)==0
    ){
      return 1;
    }
  }
  return 0;
}
/* …（同文件无关代码省略）… */
  SrcItem *pSrc;              /* The FROM clause btree term to add */
  WhereLoop *pNew;            /* Template WhereLoop object */
  int rc = SQLITE_OK;         /* Return code */
  int iSortIdx = 1;           /* Index number */
  int b;                      /* A boolean value */
  LogEst rSize;               /* number of rows in the table */
  WhereClause *pWC;           /* The parsed WHERE clause */
/* …（同文件无关代码省略）… */
    /* An INDEXED BY clause specifies a particular index to use */
    pProbe = pSrc->u2.pIBIndex;
  }else if( !HasRowid(pTab) ){
    pProbe = pTab->pIndex;
  }else{
    /* There is no INDEXED BY clause.  Create a fake Index object in local
    ** variable sPk to represent the rowid primary key index.  Make this
/* …（同文件无关代码省略）… */
    aiRowEstPk[0] = pTab->nRowLogEst;
    aiRowEstPk[1] = 0;
    pFirst = pSrc->pSTab->pIndex;
    if( pSrc->fg.notIndexed==0 ){
      /* The real indices of the table are only considered if the
      ** NOT INDEXED qualifier is omitted from the FROM clause */
      sPk.pNext = pFirst;
    }
    pProbe = &sPk;
  }
  rSize = pTab->nRowLogEst;
/* …（同文件无关代码省略）… */

  /* Loop over all indices. If there was an INDEXED BY clause, then only
  ** consider index pProbe.  */
  for(; rc==SQLITE_OK && pProbe;
      pProbe=(pSrc->fg.isIndexedBy ? 0 : pProbe->pNext), iSortIdx++
  ){
    if( pProbe->pPartIdxWhere!=0
     && !whereUsablePartialIndex(pSrc->iCursor, pSrc->fg.jointype, pWC,
