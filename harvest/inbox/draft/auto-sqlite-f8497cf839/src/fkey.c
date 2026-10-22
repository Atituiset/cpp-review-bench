// AUTO-DRAFT from sqlite/sqlite PR #61a8c1579d1b6721c4f62ea2efda1c218eff36e6
  return (aff1==aff2 || aff1==SQLITE_AFF_BLOB);
}  // <<< BUG ANCHOR

/*
** Return an expression equivalent to (pLeft = pRight), where pLeft is a
** parent key value and pRight is an unresolved reference to column
/* …（同文件无关代码省略）… */
** If the simple (pLeft = pRight) does not naturally use the affinity of
** the parent column, the comparison is coded as (pLeft = +pRight). But
** the unary + prevents any index on the child column from being used.
** So in that case an extra term is added:
**
**     (pLeft = +pRight) AND
**     (pRight COLLATE <coll> IN (+pLeft, +CAST(pLeft AS NUMERIC)))
/* …（同文件无关代码省略）… */

#ifndef SQLITE_OMIT_CAST
  {
    static const Token tNumeric = { "NUMERIC", 7 };
    const char *zColl = sqlite3ColumnColl(&pTab->aCol[iCol]);
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
          sqlite3ExprDup(db, pRight, 0), zColl ? zColl : sqlite3StrBINARY
    ), 0);
    if( pIn ){
      pIn->x.pList = pList;
      sqlite3ExprSetHeightAndFlags(pParse, pIn);
    }else{
      sqlite3ExprListDelete(db, pList);
    }
  }
#endif
