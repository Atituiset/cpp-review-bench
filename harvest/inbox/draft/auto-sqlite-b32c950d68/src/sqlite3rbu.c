// AUTO-DRAFT from sqlite/sqlite PR #dd33a69869fa628f572cd6b04d4487f19a526f3a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
# define SWAP(TYPE,A,B) {TYPE t=A; A=B; B=t;}
/* …（同文件无关代码省略）… */
typedef struct RbuFrame RbuFrame;
typedef struct RbuObjIter RbuObjIter;
/* …（同文件无关代码省略）… */
typedef struct rbu_file rbu_file;
/* …（同文件无关代码省略）… */
typedef unsigned int u32;
/* …（同文件无关代码省略）… */
typedef unsigned char u8;
typedef sqlite3_int64 i64;
/* …（同文件无关代码省略）… */
  int nTblCol;                    /* Size of azTblCol[] array */
  char **azTblCol;                /* Array of unquoted target column names */
  char **azTblType;               /* Array of target column types */
  int *aiSrcOrder;                /* src table col -> target table col */
  u8 *abTblPk;                    /* Array of flags, set on target PK columns */
  u8 *abNotNull;                  /* Array of flags, set on NOT NULL columns */
/* …（同文件无关代码省略）… */
struct sqlite3rbu {
  int eStage;                     /* Value of RBU_STATE_STAGE field */
  sqlite3 *dbMain;                /* target database handle */
  sqlite3 *dbRbu;                 /* rbu database handle */
  char *zTarget;                  /* Path to target db */
  char *zRbu;                     /* Path to rbu db */
  char *zState;                   /* Path to state db (or NULL if zRbu) */
  char zStateDb[5];               /* Db name for state ("stat" or "main") */
  int rc;                         /* Value returned by last rbu_step() call */
  char *zErrmsg;                  /* Error message if rc!=SQLITE_OK */
  int nStep;                      /* Rows processed for current object */
  sqlite3_int64 nProgress;        /* Rows processed for all objects */
  RbuObjIter objiter;             /* Iterator for skipping through tbl/idx */
  const char *zVfsName;           /* Name of automatically created rbu vfs */
  rbu_file *pTargetFd;            /* File handle open on target db */
  int nPagePerSector;             /* Pages per sector for pTargetFd */
  i64 iOalSz;
  i64 nPhaseOneStep;
  void *pRenameArg;
  int (*xRename)(void*, const char*, const char*);

  /* The following state variables are used as part of the incremental
  ** checkpoint stage (eStage==RBU_STAGE_CKPT). See comments surrounding
  ** function rbuSetupCheckpoint() for details.  */
  u32 iMaxFrame;                  /* Largest iWalFrame value in aFrame[] */
  u32 mLock;
  int nFrame;                     /* Entries in aFrame[] array */
  int nFrameAlloc;                /* Allocated size of aFrame[] array */
  RbuFrame *aFrame;
  int pgsz;
  u8 *aBuf;
  i64 iWalCksum;
  i64 szTemp;                     /* Current size of all temp files in use */
  i64 szTempLimit;                /* Total size limit for temp files */

  /* Used in RBU vacuum mode only */
  int nRbu;                       /* Number of RBU VFS in the stack */
  rbu_file *pRbuFd;               /* Fd for main db of dbRbu */
};
/* …（同文件无关代码省略）… */
static int prepareAndCollectError(
  sqlite3 *db, 
  sqlite3_stmt **ppStmt,
  char **pzErrmsg,
  const char *zSql
){
  int rc = sqlite3_prepare_v2(db, zSql, -1, ppStmt, 0);
  if( rc!=SQLITE_OK ){
    *pzErrmsg = sqlite3_mprintf("%s", sqlite3_errmsg(db));
    *ppStmt = 0;
  }
  return rc;
}
/* …（同文件无关代码省略）… */
static int prepareFreeAndCollectError(
  sqlite3 *db, 
  sqlite3_stmt **ppStmt,
  char **pzErrmsg,
  char *zSql
){
  int rc;
  assert( *pzErrmsg==0 );
  if( zSql==0 ){
    rc = SQLITE_NOMEM;
    *ppStmt = 0;
  }else{
    rc = prepareAndCollectError(db, ppStmt, pzErrmsg, zSql);
    sqlite3_free(zSql);
  }
  return rc;
}
/* …（同文件无关代码省略）… */
  for(i=0; i<pIter->nTblCol; i++){
    sqlite3_free(pIter->azTblCol[i]);
    sqlite3_free(pIter->azTblType[i]);
  }
  sqlite3_free(pIter->azTblCol);
  pIter->azTblCol = 0;
  pIter->azTblType = 0;
  pIter->aiSrcOrder = 0;
  pIter->abTblPk = 0;
  pIter->abNotNull = 0;
/* …（同文件无关代码省略）… */
static char *rbuMPrintf(sqlite3rbu *p, const char *zFmt, ...){
  char *zSql = 0;
  va_list ap;
  va_start(ap, zFmt);
  zSql = sqlite3_vmprintf(zFmt, ap);
  if( p->rc==SQLITE_OK ){
    if( zSql==0 ) p->rc = SQLITE_NOMEM;
  }else{
    sqlite3_free(zSql);
    zSql = 0;
  }
  va_end(ap);
  return zSql;
}
/* …（同文件无关代码省略）… */
static void *rbuMalloc(sqlite3rbu *p, sqlite3_int64 nByte){
  void *pRet = 0;
  if( p->rc==SQLITE_OK ){
    assert( nByte>0 );
    pRet = sqlite3_malloc64(nByte);
    if( pRet==0 ){
      p->rc = SQLITE_NOMEM;
    }else{
      memset(pRet, 0, nByte);
    }
  }
  return pRet;
}
/* …（同文件无关代码省略）… */
** error code in the RBU handle passed as the first argument.
*/
static void rbuAllocateIterArrays(sqlite3rbu *p, RbuObjIter *pIter, int nCol){
  sqlite3_int64 nByte = (2*sizeof(char*) + sizeof(int) + 3*sizeof(u8)) * nCol;
  char **azNew;

  azNew = (char**)rbuMalloc(p, nByte);
  if( azNew ){
    pIter->azTblCol = azNew;
    pIter->azTblType = &azNew[nCol];
    pIter->aiSrcOrder = (int*)&pIter->azTblType[nCol];
    pIter->abTblPk = (u8*)&pIter->aiSrcOrder[nCol];
    pIter->abNotNull = (u8*)&pIter->abTblPk[nCol];
    pIter->abIndexed = (u8*)&pIter->abNotNull[nCol];
/* …（同文件无关代码省略）… */
static char *rbuStrndup(const char *zStr, int *pRc){
  char *zRet = 0;

  if( *pRc==SQLITE_OK ){
    if( zStr ){
      size_t nCopy = strlen(zStr) + 1;
      zRet = (char*)sqlite3_malloc64(nCopy);
      if( zRet ){
        memcpy(zRet, zStr, nCopy);
      }else{
        *pRc = SQLITE_NOMEM;
      }
    }
  }

  return zRet;
}
/* …（同文件无关代码省略）… */
static void rbuFinalize(sqlite3rbu *p, sqlite3_stmt *pStmt){
  sqlite3 *db = sqlite3_db_handle(pStmt);
  int rc = sqlite3_finalize(pStmt);
  if( p->rc==SQLITE_OK && rc!=SQLITE_OK ){
    p->rc = rc;
    p->zErrmsg = sqlite3_mprintf("%s", sqlite3_errmsg(db));
  }
}
/* …（同文件无关代码省略）… */
static int rbuTableIsStrict(sqlite3rbu *p, const char *zTbl){
  int bStrict = 0;
  sqlite3_stmt *pStmt = 0;
  if( p->rc==SQLITE_OK ){
    p->rc = prepareFreeAndCollectError(p->dbMain, &pStmt, &p->zErrmsg,
        sqlite3_mprintf("PRAGMA main.table_list(%Q)", zTbl)
    );
    if( 
