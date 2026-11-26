// AUTO-DRAFT from sqlite/sqlite PR #9dcd8b5400c89571299e3b52d2f207932171c884
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
static int fkUsesLhsAffinity(Table *p1, int iCol1, Table *p2, int iCol2){
  u8 aff1 = iCol1>=0 ? p1->aCol[iCol1].affinity : SQLITE_AFF_NUMERIC;
  u8 aff2 = p2->aCol[iCol2].affinity;

  /* If either side has a numeric affinity, then the comparison uses numeric
  ** affinity. So the affinity matches if aff1 is numeric, but not if aff1
  ** is not numeric but aff2 is.  */
  if( sqlite3IsNumericAffinity(aff1) ) return 1;
  if( sqlite3IsNumericAffinity(aff2) ) return 0;

  assert( aff1==SQLITE_AFF_TEXT || aff1==SQLITE_AFF_BLOB );
  assert( aff2==SQLITE_AFF_TEXT || aff2==SQLITE_AFF_BLOB );

  /* At this point, either the affinities are the same or one is TEXT and
  ** the other BLOB. In the second case the comparison will be done with
  ** BLOB affinity, which does not match if aff1 is TEXT.  */
  return (aff1==aff2 || aff1==SQLITE_AFF_BLOB);
}
/* …（同文件无关代码省略）… */
#ifndef SQLITE_OMIT_CAST
/*
** Return non-zero if a lookup on column iCol of table pTab, using collation
** sequence zColl, can be done using either the rowid or an index. That is,
** if iCol is the INTEGER PRIMARY KEY of pTab, or if pTab has an index
** whose left-most column is iCol and whose collation sequence for that
** column is zColl. Return zero otherwise.
*/
static int fkChildKeyIsIndexed(Table *pTab, int iCol, const char *zColl){
  Index *pIdx;
  if( iCol==pTab->iPKey ) return 1;
  for(pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext){
    if( pIdx->aiColumn[0]==iCol
     && pIdx->pPartIdxWhere==0
     && sqlite3StrICmp(pIdx->azColl[0], zColl)==0
    ){
      return 1;
    }
/* …（同文件无关代码省略）… */
** If the simple (pLeft = pRight) does not naturally use the affinity of
** the parent column, the comparison is coded as (pLeft = +pRight). But
** the unary + prevents any index on the child column from being used.
** So in that case, if the child column is indexed, an extra term is added:
**
**     (pLeft = +pRight) AND
**     (pRight COLLATE <coll> IN (+pLeft, +CAST(pLeft AS NUMERIC)))
**
** Each child value that matches pLeft under the affinity of the parent
** column is either equal to pLeft itself or numerically equal to
** CAST(pLeft AS NUMERIC). So the IN term matches a superset of the
** child rows that the exact (pLeft = +pRight) term does, and since it
** compares using the affinity of the child column, it may be used with
** an index on the child column. The unary + operators on the RHS of the
** IN ensure that no affinity other than that of the child column is
** applied.
*/
static Expr *fkParentChildEq(
  Parse *pParse,            /* Parsing context */
  Expr *pLeft,              /* Parent key value */
  Expr *pRight,             /* Unresolved reference to child key column */
  Table *pTab,              /* Parent table */
  int iCol,                 /* Parent key column, or <0 for rowid */
  Table *pChild,            /* Child table */
  int iChildCol,            /* Child key column */
  int bComposite            /* This is part of a composite FK */
){
  sqlite3 *db = pParse->db;
  Expr *pEq;
#ifndef SQLITE_OMIT_CAST
  Expr *pIn = 0;
#endif

  if( fkUsesLhsAffinity(pTab, iCol, pChild, iChildCol) ){
    return sqlite3PExpr(pParse, TK_EQ, pLeft, pRight);
  }
  assert( iCol>=0 );

#ifndef SQLITE_OMIT_CAST
  {
    const char *zColl = sqlite3ColumnColl(&pTab->aCol[iCol]);
    if( zColl==0 ) zColl = sqlite3StrBINARY;

    /* The IN term is only useful if it can drive a lookup on the child
    ** table. Omit it otherwise, so that a full scan of the child table
    ** does not have to evaluate it for every row.  */
    if( bComposite || fkChildKeyIsIndexed(pChild, iChildCol, zColl) ){
      static const Token tNumeric = { "NUMERIC", 7 };
      Expr *pCast;
      ExprList *pList;

      pCast = sqlite3ExprAlloc(db, TK_CAST, &tNumeric, 0);
      sqlite3ExprAttachSubtrees(db, pCast, sqlite3ExprDup(db, pLeft, 0), 0);
      pList = sqlite3ExprListAppend(pParse, 0,
          sqlite3PExpr(pParse, TK_UPLUS, sqlite3ExprDup(db, pLeft, 0), 0)
      );
      pList = sqlite3ExprListAppend(pParse, pList,
          sqlite3PExpr(pParse, TK_UPLUS, pCast, 0)
      );
      pIn = sqlite3PExpr(pParse, TK_IN, sqlite3ExprAddCollateString(pParse,
            sqlite3ExprDup(db, pRight, 0), zColl
      ), 0);
      if( pIn ){
        pIn->x.pList = pList;
        sqlite3ExprSetHeightAndFlags(pParse, pIn);
      }else{
        sqlite3ExprListDelete(db, pList);
      }
    }
  }
#endif

  pRight = sqlite3PExpr(pParse, TK_UPLUS, pRight, 0);
  pEq = sqlite3PExpr(pParse, TK_EQ, pLeft, pRight);
#ifndef SQLITE_OMIT_CAST
  pEq = sqlite3ExprAnd(pParse, pEq, pIn);
#endif
  return pEq;
}
/* …（同文件无关代码省略）… */
    zCol = pFKey->pFrom->aCol[iCol2].zCnName;
    pRight = sqlite3Expr(db, TK_ID, zCol);
    pEq = fkParentChildEq(pParse, pLeft, pRight, pTab, iCol,
                          pFKey->pFrom, iCol2, pFKey->nCol>0);
    pWhere = sqlite3ExprAnd(pParse, pWhere, pEq);
  }
