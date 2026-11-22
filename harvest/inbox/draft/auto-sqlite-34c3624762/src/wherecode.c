// AUTO-DRAFT from sqlite/sqlite PR #d8d097aebb6b7641c76ff982d97038a138b0c132
  pParent = &pWInfo->a[iParent];
  if( (pParent->pWLoop->wsFlags & WHERE_AUTO_INDEX)!=0
   && (mKey & (mKey-1))==0
  ){
    for(i=0; i<pLoop->nLTerm; i++){
      Expr *pX = pLoop->aLTerm[i]->pExpr;
