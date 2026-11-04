// AUTO-DRAFT from sqlite/sqlite PR #a1a0e82231e9192934de1fc18cd9e9851da1681a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define UNUSED_PARAMETER(x)  (void)x
/* …（同文件无关代码省略）… */
static void printWithPrefix(FILE *pOut, const char *zPrefix, const char *zMsg){
  while( zMsg && zMsg[0] ){
    int i;
    for(i=0; zMsg[i] && zMsg[i]!='\n' && zMsg[i]!='\r'; i++){}
    fprintf(pOut, "%s%.*s\n", zPrefix, i, zMsg);
    zMsg += i;
    while( zMsg[0]=='\n' || zMsg[0]=='\r' ) zMsg++;
  }
}
/* …（同文件无关代码省略）… */
static int safe_strcmp(const char *a, const char *b){
  if( a==b ) return 0;
  if( a==0 ) return -1;
  if( b==0 ) return 1;
  return strcmp(a,b);
}
/* …（同文件无关代码省略）… */
static void maybeClose(FILE *pOut){
  if( pOut!=stdout && pOut!=stderr ) fclose(pOut);
}
/* …（同文件无关代码省略）… */
static void errorMessage(const char *zFormat, ...){
  va_list ap;
  char *zMsg;
  char zPrefix[30];
  va_start(ap, zFormat);
  zMsg = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  sqlite3_snprintf(sizeof(zPrefix), zPrefix, "%s:ERROR: ", g.zName);
  if( g.pLog ){
    printWithPrefix(g.pLog, zPrefix, zMsg);
    fflush(g.pLog);
  }
  if( g.pErrLog && safe_strcmp(g.zErrLog,g.zLog) ){
    printWithPrefix(g.pErrLog, zPrefix, zMsg);
    fflush(g.pErrLog);
  }
  sqlite3_free(zMsg);
  g.nError++;
}
/* …（同文件无关代码省略）… */
static void fatalError(const char *zFormat, ...){
  va_list ap;
  char *zMsg;
  char zPrefix[30];
  va_start(ap, zFormat);
  zMsg = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  sqlite3_snprintf(sizeof(zPrefix), zPrefix, "%s:FATAL: ", g.zName);
  if( g.pLog ){
    printWithPrefix(g.pLog, zPrefix, zMsg);
    fflush(g.pLog);
    maybeClose(g.pLog);
  }
  if( g.pErrLog && safe_strcmp(g.zErrLog,g.zLog) ){
    printWithPrefix(g.pErrLog, zPrefix, zMsg);
    fflush(g.pErrLog);
    maybeClose(g.pErrLog);
  }
  sqlite3_free(zMsg);
  if( g.db ){
    int nTry = 0;
    g.iTimeout = 0;
    while( trySql("UPDATE client SET wantHalt=1;")==SQLITE_BUSY
           && (nTry++)<100 ){
      sqlite3_sleep(10);
    }
  }
  sqlite3_close(g.db);
  exit(1);  
}
/* …（同文件无关代码省略）… */
static void logMessage(const char *zFormat, ...){
  va_list ap;
  char *zMsg;
  char zPrefix[30];
  va_start(ap, zFormat);
  zMsg = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  sqlite3_snprintf(sizeof(zPrefix), zPrefix, "%s: ", g.zName);
  if( g.pLog ){
    printWithPrefix(g.pLog, zPrefix, zMsg);
    fflush(g.pLog);
  }
  sqlite3_free(zMsg);
}
/* …（同文件无关代码省略）… */
static void sqlErrorCallback(void *pArg, int iErrCode, const char *zMsg){
  UNUSED_PARAMETER(pArg);
  if( iErrCode==SQLITE_ERROR && g.bIgnoreSqlErrors ) return;
  if( (iErrCode&0xff)==SQLITE_SCHEMA && g.iTrace<3 ) return;
  if( g.iTimeout==0 && (iErrCode&0xff)==SQLITE_BUSY && g.iTrace<3 ) return;
  if( (iErrCode&0xff)==SQLITE_NOTICE ){
    logMessage("(info) %s", zMsg);
  }else{
    errorMessage("(errcode=%d) %s", iErrCode, zMsg);
  }
}
/* …（同文件无关代码省略）… */
static int trySql(const char *zFormat, ...){
  va_list ap;
  char *zSql;
  int rc;
  va_start(ap, zFormat);
  zSql = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  rc = sqlite3_exec(g.db, zSql, 0, 0, 0);
  sqlite3_free(zSql);
  return rc;
}
/* …（同文件无关代码省略）… */
    g.pLog = stdout;
  }
  
  sqlite3_config(SQLITE_CONFIG_LOG, sqlErrorCallback, 0);
  if( zClient ){
    iClient = atoi(zClient);
    if( iClient<1 ) fatalError("illegal client number: %d\n", iClient);
