// AUTO-DRAFT from sqlite/sqlite PR #1059bc8eace8e82db4cdafade29907f7fccebe83
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define ISSPACE(X) isspace((unsigned char)(X))
/* …（同文件无关代码省略）… */
# define sqlite3_int64 sqlite_int64
/* …（同文件无关代码省略）… */
static void fatal_error(const char *zMsg, ...){
  va_list ap;
  va_start(ap, zMsg);
  vfprintf(stderr, zMsg, ap);
  va_end(ap);
#ifdef SQLITE_SPEEDTEST1_WASM
  /* Emscripten complains when exit() is called and anything is left
     in the I/O buffers. */
  fflush(stdout);
  fflush(stderr);
#endif
  exit(1);
}
/* …（同文件无关代码省略）… */
sqlite3_int64 speedtest1_timestamp(void){
#if SQLITE_VERSION_NUMBER<3005000
  return 0;
#else
  static sqlite3_vfs *clockVfs = 0;
  sqlite3_int64 t;
  if( clockVfs==0 ) clockVfs = sqlite3_vfs_find(0);
#if SQLITE_VERSION_NUMBER>=3007000
  if( clockVfs->iVersion>=2 && clockVfs->xCurrentTimeInt64!=0 ){
    clockVfs->xCurrentTimeInt64(clockVfs, &t);
  }else
#endif
  {
    double r;
    clockVfs->xCurrentTime(clockVfs, &r);
    t = (sqlite3_int64)(r*86400000.0);
  }
  return t;
#endif
}
/* …（同文件无关代码省略）… */
#define NAMEWIDTH 60
/* …（同文件无关代码省略）… */
void speedtest1_begin_test(int iTestNum, const char *zTestName, ...){
  int n = (int)strlen(zTestName);
  char *zName;
  va_list ap;
  iTestNumber = iTestNum;
  va_start(ap, zTestName);
  zName = sqlite3_vmprintf(zTestName, ap);
  va_end(ap);
  n = (int)strlen(zName);
  if( n>NAMEWIDTH ){
    zName[NAMEWIDTH] = 0;
    n = NAMEWIDTH;
  }
  if( g.pScript ){
    fprintf(g.pScript,"-- begin test %d %.*s\n", iTestNumber, n, zName)
      /* maintenance reminder: ^^^ code in ext/wasm expects %d to be
      ** field #4 (as in: cut -d' ' -f4). */;
  }
  if( g.bSqlOnly ){
    printf("/* %4d - %s%.*s */\n", iTestNum, zName, NAMEWIDTH-n, zDots);
  }else{
    printf("%4d - %s%.*s ", iTestNum, zName, NAMEWIDTH-n, zDots);
    fflush(stdout);
  }
  sqlite3_free(zName);
  g.nResult = 0;
  g.iStart = speedtest1_timestamp();
  g.x = 0xad131d0b;
  g.y = 0x44f9eac8;
}
/* …（同文件无关代码省略）… */
void speedtest1_end_test(void){
  sqlite3_int64 iElapseTime = speedtest1_timestamp() - g.iStart;
  if( g.doCheckpoint ) speedtest1_exec("PRAGMA wal_checkpoint;");
  assert( iTestNumber > 0 );
  if( g.pScript ){
    fprintf(g.pScript,"-- end test %d\n", iTestNumber);
  }
  if( !g.bSqlOnly ){
    g.iTotal += iElapseTime;
    printf("%4d.%03ds\n", (int)(iElapseTime/1000), (int)(iElapseTime%1000));
  }
  if( g.pStmt ){
    sqlite3_finalize(g.pStmt);
    g.pStmt = 0;
  }
  iTestNumber = 0;
}
/* …（同文件无关代码省略）… */
static void printSql(const char *zSql){
  int n = (int)strlen(zSql);
  while( n>0 && (zSql[n-1]==';' || ISSPACE(zSql[n-1])) ){ n--; }
  if( g.bExplain ) printf("EXPLAIN ");
  printf("%.*s;\n", n, zSql);
  if( g.bExplain
#if SQLITE_VERSION_NUMBER>=3007017 
   && ( sqlite3_strglob("CREATE *", zSql)==0
     || sqlite3_strglob("DROP *", zSql)==0
     || sqlite3_strglob("ALTER *", zSql)==0
      )
#endif
  ){
    printf("%.*s;\n", n, zSql);
  }
}
/* …（同文件无关代码省略）… */
void speedtest1_shrink_memory(void){
#if SQLITE_VERSION_NUMBER>=3007010
  if( g.bMemShrink ) sqlite3_db_release_memory(g.db);
#endif
}
/* …（同文件无关代码省略）… */
void speedtest1_exec(const char *zFormat, ...){
  va_list ap;
  char *zSql;
  va_start(ap, zFormat);
  zSql = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  if( g.bSqlOnly ){
    printSql(zSql);
  }else{
    char *zErrMsg = 0;
    int rc;
    if( g.pScript ){
      fprintf(g.pScript,"%s;\n",zSql);
    }
    rc = sqlite3_exec(g.db, zSql, 0, 0, &zErrMsg);
    if( zErrMsg ) fatal_error("SQL error: %s\n%s\n", zErrMsg, zSql);
    if( rc!=SQLITE_OK ) fatal_error("exec error: %s\n", sqlite3_errmsg(g.db));
  }
  sqlite3_free(zSql);
  speedtest1_shrink_memory();
}
/* …（同文件无关代码省略）… */
   );
  speedtest1_end_test();

  speedtest1_begin_test(130, "Star query with LEFT JOINs");
  speedtest1_exec(
    "SELECT count(*), max(content04), min(content03), sum(rate04), avg(rate05)"
    " FROM facttab LEFT JOIN dimension01 ON attr01=beta01"
