// AUTO-DRAFT from sqlite/sqlite PR #623bc0b1d5e5a45236c8d2f46f52f96dfe90b780
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
struct Fts5Storage {
  Fts5Config *pConfig;
  Fts5Index *pIndex;
  int bTotalsValid;               /* True if nTotalRow/aTotalSize[] are valid */
  i64 nTotalRow;                  /* Total number of rows in FTS table */
  i64 *aTotalSize;                /* Total sizes of each column */ 
  sqlite3_stmt *pSavedRow;
  sqlite3_stmt *aStmt[12];
};
/* …（同文件无关代码省略）… */
  Fts5Storage *p,                 /* Storage handle */
  int eStmt,                      /* FTS5_STMT_XXX constant */
  sqlite3_stmt **ppStmt,          /* OUT: Prepared statement handle */
  char **pzErrMsg                 /* OUT: Error message (if any) */
){
  int rc = SQLITE_OK;

  /* If there is no %_docsize table, there should be no requests for 
  ** statements to operate on it.  */
  assert( p->pConfig->bColumnsize || (
