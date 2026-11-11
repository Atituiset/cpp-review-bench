// AUTO-DRAFT from sqlite/sqlite PR #6a1f950aaeedf781df59b0a531a13774997f76c3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
static Expr *exprTableRegister(
  Parse *pParse,     /* Parsing and code generating context */
  Table *pTab,       /* The table whose content is at r[regBase]... */
  int regBase,       /* Contents of table pTab */
  i16 iCol           /* Which column of pTab is desired */
){
  Expr *pExpr;
  Column *pCol;
  const char *zColl;
  sqlite3 *db = pParse->db;

  pExpr = sqlite3Expr(db, TK_REGISTER, 0);
  if( pExpr ){
    if( iCol>=0 && iCol!=pTab->iPKey ){
      pCol = &pTab->aCol[iCol];
      pExpr->iTable = regBase + sqlite3TableColumnToStorage(pTab,iCol) + 1;
      pExpr->affExpr = pCol->affinity;
      zColl = sqlite3ColumnColl(pCol);
      if( zColl==0 ) zColl = db->pDfltColl->zName;
      pExpr = sqlite3ExprAddCollateString(pParse, pExpr, zColl);
    }else{
      pExpr->iTable = regBase;
      pExpr->affExpr = SQLITE_AFF_INTEGER;
    }
  }
  return pExpr;
}
/* …（同文件无关代码省略）… */
static Expr *exprTableColumn(
  sqlite3 *db,      /* The database connection */
  Table *pTab,      /* The table whose column is desired */
  int iCursor,      /* The open cursor on the table */
  i16 iCol          /* The column that is wanted */
){
  Expr *pExpr = sqlite3Expr(db, TK_COLUMN, 0);
  if( pExpr ){
    assert( ExprUseYTab(pExpr) );
    pExpr->y.pTab = pTab;
    pExpr->iTable = iCursor;
    pExpr->iColumn = iCol;
  }
  return pExpr;
}
/* …（同文件无关代码省略）… */
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
  }
  return 0;
}
/* …（同文件无关代码省略）… */
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
static void fkScanChildren(
  Parse *pParse,                  /* Parse context */
  SrcList *pSrc,                  /* The child table to be scanned */
  Table *pTab,                    /* The parent table */
  Index *pIdx,                    /* Index on parent covering the foreign key */
  FKey *pFKey,                    /* The foreign key linking pSrc to pTab */
  int *aiCol,                     /* Map from pIdx cols to child table cols */
  int regData,                    /* Parent row data starts here */
  int nIncr                       /* Amount to increment deferred counter by */
){
  sqlite3 *db = pParse->db;       /* Database handle */
  int i;                          /* Iterator variable */
  Expr *pWhere = 0;               /* WHERE clause to scan with */
  NameContext sNameContext;       /* Context used to resolve WHERE clause */
  WhereInfo *pWInfo;              /* Context used by sqlite3WhereXXX() */
  int iFkIfZero = 0;              /* Address of OP_FkIfZero */
  Vdbe *v = sqlite3GetVdbe(pParse);

  assert( pIdx==0 || pIdx->pTable==pTab );
  assert( pIdx==0 || pIdx->nKeyCol==pFKey->nCol );
  assert( pIdx!=0 || pFKey->nCol==1 );
  assert( pIdx!=0 || HasRowid(pTab) );

  if( nIncr<0 ){
    iFkIfZero = sqlite3VdbeAddOp2(v, OP_FkIfZero, pFKey->isDeferred, 0);
    VdbeCoverage(v);
  }

  /* Create an Expr object representing an SQL expression like:
  **
  **   <parent-key1> = <child-key1> AND <parent-key2> = <child-key2> ...
  **
  ** The collation sequence used for the comparison should be that of
  ** the parent key columns. The affinity of the parent key column should
  ** be applied to each child key value before the comparison takes place.
  */
  for(i=0; i<pFKey->nCol; i++){
    Expr *pLeft;                  /* Value from parent table row */
    Expr *pRight;                 /* Column ref to child table */
    Expr *pEq;                    /* Expression (pLeft = pRight) */
    i16 iCol;                     /* Index of column in parent table */
    i16 iCol2;                    /* Index of column in child table */
    const char *zCol;             /* Name of column in child table */

    iCol = pIdx ? pIdx->aiColumn[i] : -1;
    pLeft = exprTableRegister(pParse, pTab, regData, iCol);
    iCol2 = aiCol ? aiCol[i] : pFKey->aCol[0].iFrom;
    assert( iCol2>=0 );
    zCol = pFKey->pFrom->aCol[iCol2].zCnName;
    pRight = sqlite3Expr(db, TK_ID, zCol);
    pEq = fkParentChildEq(pParse, pLeft, pRight, pTab, iCol,
                          pFKey->pFrom, iCol2, pFKey->nCol>0);
    pWhere = sqlite3ExprAnd(pParse, pWhere, pEq);
  }

  /* If the child table is the same as the parent table, then add terms
  ** to the WHERE clause that prevent this entry from being scanned.
  ** The added WHERE clause terms are like this:
  **
  **     $current_rowid!=rowid
  **     NOT( $current_a==a AND $current_b==b AND ... )
  **
  ** The first form is used for rowid tables.  The second form is used
  ** for WITHOUT ROWID tables. In the second form, the *parent* key is
  ** (a,b,...). Either the parent or primary key could be used to 
  ** uniquely identify the current row, but the parent key is more convenient
  ** as the required values have already been loaded into registers
  ** by the caller.
  */
  if( pTab==pFKey->pFrom && nIncr>0 ){
    Expr *pNe;                    /* Expression (pLeft != pRight) */
    Expr *pLeft;                  /* Value from parent table row */
    Expr *pRight;                 /* Column ref to child table */
    if( HasRowid(pTab) ){
      pLeft = exprTableRegister(pParse, pTab, regData, -1);
      pRight = exprTableColumn(db, pTab, pSrc->a[0].iCursor, -1);
      pNe = sqlite3PExpr(pParse, TK_NE, pLeft, pRight);
    }else{
      Expr *pEq, *pAll = 0;
      assert( pIdx!=0 );
      for(i=0; i<pIdx->nKeyCol; i++){
        i16 iCol = pIdx->aiColumn[i];
        assert( iCol>=0 );
        pLeft = exprTableRegister(pParse, pTab, regData, iCol);
        pRight = sqlite3Expr(db, TK_ID, pTab->aCol[iCol].zCnName);
        pEq = sqlite3PExpr(pParse, TK_IS, pLeft, pRight);
        pAll = sqlite3ExprAnd(pParse, pAll, pEq);
      }
      pNe = sqlite3PExpr(pParse, TK_NOT, pAll, 0);
    }
    pWhere = sqlite3ExprAnd(pParse, pWhere, pNe);
  }

  /* Resolve the references in the WHERE clause. */
  memset(&sNameContext, 0, sizeof(NameContext));
  sNameContext.pSrcList = pSrc;
  sNameContext.pParse = pParse;
  sqlite3ResolveExprNames(&sNameContext, pWhere);

  /* Create VDBE to loop through the entries in pSrc that match the WHERE
  ** clause. For each row found, increment either the deferred or immediate
  ** foreign key constraint counter. */
  if( pParse->nErr==0 ){
    pWInfo = sqlite3WhereBegin(pParse, pSrc, pWhere, 0, 0, 0, 0, 0);
    sqlite3VdbeAddOp2(v, OP_FkCounter, pFKey->isDeferred, nIncr);
    if( pWInfo ){
      sqlite3WhereEnd(pWInfo);
    }
  }

  /* Clean up the WHERE clause constructed above. */
  sqlite3ExprDelete(db, pWhere);
  if( iFkIfZero ){
    sqlite3VdbeJumpHereOrPopInst(v, iFkIfZero);
  }
}
/* …（同文件无关代码省略）… */
  return 0;
}

/*
** This function is called when inserting, deleting or updating a row of
** table pTab to generate VDBE code to perform foreign key constraint 
/* …（同文件无关代码省略）… */
        if( (db->flags & SQLITE_FkNoAction) ) eAction = OE_None;

        fkScanChildren(pParse, pSrc, pTab, pIdx, pFKey, aiCol, regOld, 1);
        /* If this is a deferred FK constraint, or a CASCADE or SET NULL
        ** action applies, then any foreign key violations caused by
        ** removing the parent key will be rectified by the action trigger.
        ** So do not set the "may-abort" flag in this case.
        **
        ** Note 1: If the FK is declared "ON UPDATE CASCADE", then the
        ** may-abort flag will eventually be set on this statement anyway
/* …（同文件无关代码省略）… */
        ** child table to fire. In these cases the fk constraint counters
        ** might be set incorrectly if any OP_FkCounter related scans are 
        ** omitted.  */
        if( !pFKey->isDeferred && eAction!=OE_Cascade && eAction!=OE_SetNull ){
          sqlite3MayAbort(pParse);
        }
      }
