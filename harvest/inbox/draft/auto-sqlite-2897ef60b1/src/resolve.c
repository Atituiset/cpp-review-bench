// AUTO-DRAFT from sqlite/sqlite PR #f852f389434dd9e7a6376dccb5e0001dac81bcf3
){  // <<< BUG ANCHOR
  if( IN_RENAME_OBJECT && pEList->a[iCol-1].fg.bStarName ){
    Expr *pX = pEList->a[iCol-1].pExpr;
    if( ALWAYS(pX->op==TK_COLUMN) && ALWAYS(pE->op==TK_ID) ){
      pE->op = TK_COLUMN;
      pE->iTable = pX->iTable;
      pE->iColumn = pX->iColumn;
