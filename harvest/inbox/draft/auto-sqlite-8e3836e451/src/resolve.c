// AUTO-DRAFT from sqlite/sqlite PR #04f1c2cc345f605f04976d859bec2f1ead053385
){  // <<< BUG ANCHOR
  if( IN_RENAME_OBJECT && pEList->a[iCol-1].fg.bStarName ){
    Expr *pX = pEList->a[iCol-1].pExpr;
    if( pX->op==TK_COLUMN && pE->op==TK_ID ){
      pE->op = TK_COLUMN;
      pE->iTable = pX->iTable;
      pE->iColumn = pX->iColumn;
