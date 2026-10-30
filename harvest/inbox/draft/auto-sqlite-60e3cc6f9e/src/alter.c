// AUTO-DRAFT from sqlite/sqlite PR #c1bf3367ea188a82153b830399b00e82652db439
static int isAlterableTable(Parse *pParse, Table *pTab){
  if( 0==sqlite3StrNICmp(pTab->zName, "sqlite_", 7)
#ifndef SQLITE_OMIT_VIRTUALTABLE
   || (pTab->tabFlags & TF_Eponymous)!=0
   || ( (pTab->tabFlags & TF_Shadow)!=0
        && sqlite3ReadOnlyShadowTables(pParse->db)
   )  // <<< BUG ANCHOR
#endif
  ){
    sqlite3ErrorMsg(pParse, "table %s may not be altered", pTab->zName);
    return 1;
  }
  return 0;
}
/* …（同文件无关代码省略）… */
  if( SQLITE_OK!=isAlterableTable(pParse, pTab) ){
    goto exit_rename_table;
  }
  if( SQLITE_OK!=sqlite3CheckObjectName(pParse,zName,"table",zName) ){
    goto exit_rename_table;
  }
