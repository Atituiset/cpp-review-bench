// AUTO-DRAFT from sqlite/sqlite PR #1820defd768c50e0ea33476c9b396de7c2c60bc4
*/
static void freeColumnList(char **azCol){
  int i;
  for(i=1; azCol[i]; i++){
    sqlite3_free(azCol[i]);
  }
/* …（同文件无关代码省略）… */
  }
  sqlite3_finalize(pStmt);
  pStmt = 0;
  azCol[nCol+1] = 0;  // <<< BUG ANCHOR

  /* The decision of whether or not a rowid really needs to be preserved
  ** is tricky.  We never need to preserve a rowid for a WITHOUT ROWID table
/* …（同文件无关代码省略）… */

  (void)azCol;
  if( nArg!=3 ) return 1;
  zTable = azArg[0];
  zType = azArg[1];
  zSql = azArg[2];
