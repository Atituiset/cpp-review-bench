// AUTO-DRAFT from sqlite/sqlite PR #28c1e4669d7649506c5bf640f62ce3abfc0114fa
  const char *zCollName
){
  CollSeq *pColl;
  if( zCollName==0 ) return 1;  // <<< BUG ANCHOR
  pColl = sqlite3ExprCompareCollSeq(pParse, pTerm->pExpr);
  if( sqlite3IsBinary(pColl) ) return 1;
  return sqlite3StrICmp(pColl->zName, zCollName)==0;
}
