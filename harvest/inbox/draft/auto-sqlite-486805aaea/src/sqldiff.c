// AUTO-DRAFT from sqlite/sqlite PR #cb547ab3e931c7766e24834af5ef6c4578863e3d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
  int bSchemaPK;            /* Use the schema-defined PK, not the true PK */
  int bHandleVtab;          /* Handle fts3, fts4, fts5 and rtree vtabs */
  unsigned fDebug;          /* Debug flags */
  int bSchemaCompare;       /* Doing single-table sqlite_schema compare */
  sqlite3 *db;              /* The database connection */
} g;
/* …（同文件无关代码省略）… */
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

  pStmt = db_prepare("SELECT sql FROM aux.sqlite_schema WHERE name=%Q", zTab);
  if( SQLITE_ROW==sqlite3_step(pStmt) ){
    sqlite3_fprintf(out, "%s;\n", sqlite3_column_text(pStmt,0));
  }
/* …（同文件无关代码省略）… */
    strFree(pIns);
  } /* endif !g.bSchemaOnly */
  pStmt = db_prepare("SELECT sql FROM aux.sqlite_schema"
                     " WHERE type='index' AND tbl_name=%Q AND sql IS NOT NULL",
                     zTab);
  while( SQLITE_ROW==sqlite3_step(pStmt) ){
    sqlite3_fprintf(out, "%s;\n", sqlite3_column_text(pStmt,0));
/* …（同文件无关代码省略）… */
  /* Drop indexes that are missing in the destination */
  pStmt = db_prepare(
    "SELECT name FROM main.sqlite_schema"
    " WHERE type='index' AND tbl_name=%Q"
    "   AND sql IS NOT NULL"
    "   AND sql NOT IN (SELECT sql FROM aux.sqlite_schema"
    "                    WHERE type='index' AND tbl_name=%Q"
    "                      AND sql IS NOT NULL)",
    zTab, zTab);
  while( SQLITE_ROW==sqlite3_step(pStmt) ){
/* …（同文件无关代码省略）… */
  /* Create indexes that are missing in the source */
  pStmt = db_prepare(
    "SELECT sql FROM aux.sqlite_schema"
    " WHERE type='index' AND tbl_name=%Q"
    "   AND sql IS NOT NULL"
    "   AND sql NOT IN (SELECT sql FROM main.sqlite_schema"
    "                    WHERE type='index' AND tbl_name=%Q"
    "                      AND sql IS NOT NULL)",
    zTab, zTab);
  while( SQLITE_ROW==sqlite3_step(pStmt) ){
/* …（同文件无关代码省略）… */
static void checkSchemasMatch(const char *zTab){
  sqlite3_stmt *pStmt = db_prepare(
      "SELECT A.sql=B.sql FROM main.sqlite_schema A, aux.sqlite_schema B"
      " WHERE A.name=%Q AND B.name=%Q", zTab, zTab
  );
  if( SQLITE_ROW==sqlite3_step(pStmt) ){
    if( sqlite3_column_int(pStmt,0)==0 ){
/* …（同文件无关代码省略）… */
      if( strcmp(z,"transaction")==0 ){
        useTransaction = 1;
      }else
      if( strcmp(z,"vtab")==0 ){
        g.bHandleVtab = 1;
      }else
/* …（同文件无关代码省略）… */
  if( rc ){
    cmdlineError("cannot open database file \"%s\"", zDb1);
  }
  rc = sqlite3_exec(g.db, "SELECT * FROM sqlite_schema", 0, 0, &zErrMsg);
  if( rc || zErrMsg ){
    cmdlineError("\"%s\" does not appear to be a valid SQLite database", zDb1);
