// AUTO-DRAFT from sqlite/sqlite PR #73ed981f3e2c8b1dc2fef21d1026e692a4d6bc00
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
typedef struct PragmaVtabCursor PragmaVtabCursor;
/* …（同文件无关代码省略）… */
static void pragmaVtabCursorClear(PragmaVtabCursor *pCsr){
  int i;
  sqlite3_finalize(pCsr->pPragma);
  pCsr->pPragma = 0;
  pCsr->iRowid = 0;
  for(i=0; i<ArraySize(pCsr->azArg); i++){
    sqlite3_free(pCsr->azArg[i]);
    pCsr->azArg[i] = 0;
  }
}
/* …（同文件无关代码省略）… */
  int i, j;
  StrAccum acc;
  char *zSql;

  UNUSED_PARAMETER(idxNum);
  UNUSED_PARAMETER(idxStr);
  pragmaVtabCursorClear(pCsr);
  j = (pTab->pName->mPragFlg & PragFlg_Result1)!=0 ? 0 : 1;
  for(i=0; i<argc; i++, j++){
    const char *zText = (const char*)sqlite3_value_text(argv[i]);
    assert( j<ArraySize(pCsr->azArg) );
/* …（同文件无关代码省略）… */
  }
  sqlite3_str_appendall(&acc, pTab->pName->zName);
  if( pCsr->azArg[0] ){
    sqlite3_str_appendf(&acc, "=%Q", pCsr->azArg[0]);
  }
  zSql = sqlite3StrAccumFinish(&acc);
  if( zSql==0 ) return SQLITE_NOMEM;
