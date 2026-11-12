// AUTO-DRAFT from sqlite/sqlite PR #bf181220a009b89d782cf6bf9a933683255f7ead
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
typedef unsigned char u8;
/* …（同文件无关代码省略）… */
typedef struct RecoverTable RecoverTable;
/* …（同文件无关代码省略）… */
typedef struct RecoverStateW1 RecoverStateW1;
/* …（同文件无关代码省略）… */
typedef struct RecoverStateLAF RecoverStateLAF;
/* …（同文件无关代码省略）… */
struct sqlite3_recover {
  /* Copies of sqlite3_recover_init[_sql]() parameters */
  sqlite3 *dbIn;                  /* Input database */
  char *zDb;                      /* Name of input db ("main" etc.) */
  char *zUri;                     /* URI for output database */
  void *pSqlCtx;                  /* SQL callback context */
  int (*xSql)(void*,const char*); /* Pointer to SQL callback function */

  /* Values configured by sqlite3_recover_config() */
  char *zStateDb;                 /* State database to use (or NULL) */
  char *zLostAndFound;            /* Name of lost-and-found table (or NULL) */
  int bFreelistCorrupt;           /* SQLITE_RECOVER_FREELIST_CORRUPT setting */
  int bRecoverRowid;              /* SQLITE_RECOVER_ROWIDS setting */
  int bSlowIndexes;               /* SQLITE_RECOVER_SLOWINDEXES setting */

  int pgsz;
  int detected_pgsz;
  int nReserve;
  u8 *pPage1Disk;
  u8 *pPage1Cache;

  /* Error code and error message */
  int errCode;                    /* For sqlite3_recover_errcode() */
  char *zErrMsg;                  /* For sqlite3_recover_errmsg() */

  int eState;
  int bCloseTransaction;

  /* Variables used with eState==RECOVER_STATE_WRITING */
  RecoverStateW1 w1;

  /* Variables used with states RECOVER_STATE_LOSTANDFOUND[123] */
  RecoverStateLAF laf;

  /* Fields used within sqlite3_recover_run() */
  sqlite3 *dbOut;                 /* Output database */
  sqlite3_stmt *pGetPage;         /* SELECT against input db sqlite_dbdata */
  RecoverTable *pTblList;         /* List of tables recovered from schema */
};
/* …（同文件无关代码省略）… */
static int recoverError(
  sqlite3_recover *p, 
  int errCode, 
  const char *zFmt, ...
){
  char *z = 0;
  va_list ap;
  va_start(ap, zFmt);
  if( zFmt ){
    z = sqlite3_vmprintf(zFmt, ap);
  }
  va_end(ap);
  sqlite3_free(p->zErrMsg);
  p->zErrMsg = z;
  p->errCode = errCode;
  return errCode;
}
/* …（同文件无关代码省略）… */
static int recoverDbError(sqlite3_recover *p, sqlite3 *db){
  return recoverError(p, sqlite3_errcode(db), "%s", sqlite3_errmsg(db));
}
/* …（同文件无关代码省略）… */
static int recoverOneStmt(sqlite3 *db, const char *zSql, const char **pzTail){
  sqlite3_stmt *pStmt = 0;
  int rc;
  if( zSql==0 ) return SQLITE_OK;
  rc = sqlite3_prepare_v2(db, zSql, -1, &pStmt, pzTail);
  if( rc ){
    sqlite3_finalize(pStmt);
    return rc;
  }
  while( SQLITE_ROW==sqlite3_step(pStmt) ){}
  return sqlite3_finalize(pStmt);
}
/* …（同文件无关代码省略）… */
static void recoverSqlCallback(sqlite3_recover *p, const char *zSql){
  if( p->errCode==SQLITE_OK && p->xSql ){
    int res = p->xSql(p->pSqlCtx, zSql);
    if( res ){
      recoverError(p, SQLITE_ERROR, "callback returned an error - %d", res);
    }
  }
}
/* …（同文件无关代码省略）… */
  if( pSelect ){
    while( sqlite3_step(pSelect)==SQLITE_ROW ){
      const char *zSql = (const char*)sqlite3_column_text(pSelect, 1);
      int rc = recoverOneStmt(p->dbOut, zSql, 0);
      if( rc==SQLITE_OK ){
        recoverSqlCallback(p, zSql);
      }else if( rc!=SQLITE_ERROR ){
        recoverDbError(p, p->dbOut);
      }
