// AUTO-DRAFT from sqlite/sqlite PR #513c19fa3894dc96b98ee770d80df6a0fa64703c
static void renumberCursorDoMapping(Walker *pWalker, int *piCursor){
  int *aCsrMap = pWalker->u.aiCol;
  int iCsr = *piCursor;
  if( iCsr < aCsrMap[0] && aCsrMap[iCsr+1]>0 ){
    *piCursor = aCsrMap[iCsr+1];
  }
}
/* …（同文件无关代码省略）… */
  if( op==TK_COLUMN || op==TK_IF_NULL_ROW ){
    renumberCursorDoMapping(pWalker, &pExpr->iTable);
  }
  if( ExprHasProperty(pExpr, EP_OuterON) ){  // <<< BUG ANCHOR
    renumberCursorDoMapping(pWalker, &pExpr->w.iJoin);
  }
  return WRC_Continue;
