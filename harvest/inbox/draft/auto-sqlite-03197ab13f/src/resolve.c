// AUTO-DRAFT from sqlite/sqlite PR #f4ebb9943ff6fe8a69f82f1acc59f409be387fbb
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
static int resolveAsName(
  Parse *pParse,     /* Parsing context for error messages */
  ExprList *pEList,  /* List of expressions to scan */
  Expr *pE           /* Expression we are trying to match */
){
  int i;             /* Loop counter */

  UNUSED_PARAMETER(pParse);

  if( pE->op==TK_ID ){
    const char *zCol;
    assert( !ExprHasProperty(pE, EP_IntValue) );
    zCol = pE->u.zToken;
    for(i=0; i<pEList->nExpr; i++){
      if( pEList->a[i].fg.eEName==ENAME_NAME
       && sqlite3_stricmp(pEList->a[i].zEName, zCol)==0
      ){
        return i+1;
      }
    }
  }
  return 0;
}
/* …（同文件无关代码省略）… */
  return 0;
}

/*
** Generate an ORDER BY or GROUP BY term out-of-range error.
*/
/* …（同文件无关代码省略）… */
        }
      }else{
        iCol = resolveAsName(pParse, pEList, pE);
        if( iCol==0 ){
          /* Now test if expression pE matches one of the values returned
          ** by pSelect. In the usual case this is done by duplicating the
          ** expression, resolving any symbols in it, and then comparing
/* …（同文件无关代码省略）… */
    if( zType[0]!='G' ){
      iCol = resolveAsName(pParse, pSelect->pEList, pE2);
      if( iCol>0 ){
        /* If an AS-name match is found, mark this ORDER BY column as being
        ** a copy of the iCol-th result-set column.  The subsequent call to
        ** sqlite3ResolveOrderGroupBy() will convert the expression to a
