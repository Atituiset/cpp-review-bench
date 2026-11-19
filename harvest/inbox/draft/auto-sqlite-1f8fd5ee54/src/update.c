// AUTO-DRAFT from sqlite/sqlite PR #13aaeee46de74ae7e74f9905837c242bbfd1d10f
# define pTrigger 0
/* …（同文件无关代码省略）… */
  newmask = sqlite3TriggerColmask(
      pParse, pTrigger, pChanges, 1, TRIGGER_BEFORE, pTab, onError
  );
  for(i=0, k=regNew; i<pTab->nCol; i++, k++){
    if( i==pTab->iPKey ){
      sqlite3VdbeAddOp2(v, OP_Null, 0, k);
