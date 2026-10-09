// AUTO-DRAFT from sqlite/sqlite PR #f19a90c56977df9e8d85e6b162c684f5bdd89467
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static void strFree(sqlite3_str *pStr){
  sqlite3_free(sqlite3_str_finish(pStr));
}
/* …（同文件无关代码省略）… */
static void cmdlineError(const char *zFormat, ...){
  sqlite3_str *pOut = sqlite3_str_new(0);
  va_list ap;
  va_start(ap, zFormat);
  sqlite3_str_vappendf(pOut, zFormat, ap);
  va_end(ap);
  sqlite3_fprintf(stderr, "%s: %s\n", g.zArgv0, sqlite3_str_value(pOut));
  strFree(pOut);
  sqlite3_fprintf(stderr, "\"%s --help\" for more help\n", g.zArgv0);
  exit(1);
}
/* …（同文件无关代码省略）… */
static void runtimeError(const char *zFormat, ...){
  sqlite3_str *pOut = sqlite3_str_new(0);
  va_list ap;
  va_start(ap, zFormat);
  sqlite3_str_vappendf(pOut, zFormat, ap);
  va_end(ap);
  sqlite3_fprintf(stderr, "%s: %s\n", g.zArgv0, sqlite3_str_value(pOut));
  strFree(pOut);
  exit(1);
}
/* …（同文件无关代码省略）… */
static sqlite3_stmt *db_vprepare(const char *zFormat, va_list ap){
  char *zSql;
  int rc;
  sqlite3_stmt *pStmt;

  zSql = sqlite3_vmprintf(zFormat, ap);
  if( zSql==0 ) runtimeError("out of memory");
  rc = sqlite3_prepare_v2(g.db, zSql, -1, &pStmt, 0);
  if( rc ){
    runtimeError("SQL statement error: %s\n\"%s\"", sqlite3_errmsg(g.db),
                 zSql);
  }
  sqlite3_free(zSql);
  return pStmt;
}
static sqlite3_stmt *db_prepare(const char *zFormat, ...){
  va_list ap;
  sqlite3_stmt *pStmt;
  va_start(ap, zFormat);
  pStmt = db_vprepare(zFormat, ap);
  va_end(ap);
  return pStmt;
}
/* …（同文件无关代码省略）… */
  const char *zSep;         /* Separator string */
  sqlite3_str *pIns;        /* Beginning of the INSERT statement */

  pStmt = db_prepare("SELECT sql FROM aux.sqlite_schema"
                     " WHERE name=%Q COLLATE nocase", zTab);
  if( SQLITE_ROW==sqlite3_step(pStmt) ){
    sqlite3_fprintf(out, "%s;\n", sqlite3_column_text(pStmt,0));
/* …（同文件无关代码省略）… */
    sqlite3_finalize(pStmt);
    strFree(pIns);
  } /* endif !g.bSchemaOnly */
  pStmt = db_prepare("SELECT sql FROM aux.sqlite_schema"
                     " WHERE type='index' AND tbl_name=%Q COLLATE nocase"
                     "   AND sql IS NOT NULL",
                     zTab);
/* …（同文件无关代码省略）… */

  /* Create indexes that are missing in the source */
  pStmt = db_prepare(
    "SELECT sql FROM aux.sqlite_schema"
    " WHERE type='index' AND tbl_name=%Q COLLATE nocase"
    "   AND sql IS NOT NULL"
    "   AND sql NOT IN (SELECT sql FROM main.sqlite_schema"
/* …（同文件无关代码省略）… */
  sqlite3_result_text(pCtx, zToken, -1, SQLITE_TRANSIENT);
}

/*
** Return the text of an SQL statement that itself returns the list of
** tables to process within the database.
/* …（同文件无关代码省略）… */
    sqlite3_db_config(g.db, SQLITE_DBCONFIG_DEFENSIVE, 1, 0);
    sqlite3_db_config(g.db, SQLITE_DBCONFIG_TRUSTED_SCHEMA, 0, 0);
  }
  rc = sqlite3_exec(g.db, "SELECT * FROM sqlite_schema", 0, 0, &zErrMsg);
  if( rc || zErrMsg ){
    cmdlineError("\"%s\" does not appear to be a valid SQLite database", zDb1);
  }
  {
    sqlite3 *db2 = 0;
    if( sqlite3_open_v2(zDb2, &db2, SQLITE_OPEN_READONLY, 0) ){
      cmdlineError("cannot open database file \"%s\"", zDb2);
