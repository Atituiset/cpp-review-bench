// AUTO-DRAFT from sqlite/sqlite PR #d246ad4b9c712d49a3a6920f3f96e269ee82fa56
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
typedef unsigned int u32;
typedef unsigned char u8;
/* …（同文件无关代码省略）… */
typedef struct RecoverTable RecoverTable;
typedef struct RecoverColumn RecoverColumn;
/* …（同文件无关代码省略）… */
**   aCol[] is an array of nCol columns. In the order in which they appear 
**   in the table.
**
** bIntkey:
**   Set to true for intkey tables, false for WITHOUT ROWID.
**
/* …（同文件无关代码省略）… */
  u32 iRoot;                      /* Root page in original database */
  char *zTab;                     /* Name of table */
  int nCol;                       /* Number of columns in table */
  RecoverColumn *aCol;            /* Array of columns */
  int bIntkey;                    /* True for intkey, false for without rowid */
  int iRowidBind;                 /* If >0, bind rowid to INSERT here */
/* …（同文件无关代码省略）… */
#define RECOVER_EHIDDEN_VIRTUAL 2      /* Virtual generated column */
#define RECOVER_EHIDDEN_STORED  3      /* Stored generated column */
/* …（同文件无关代码省略）… */
typedef struct RecoverStateW1 RecoverStateW1;
/* …（同文件无关代码省略）… */
  int nInsert;

  RecoverTable *pTab;             /* Table currently being written */
  int nMax;                       /* Max column count in any schema table */
  sqlite3_value **apVal;          /* Array of nMax values */
  int nVal;                       /* Number of valid entries in apVal[] */
  int bHaveRowid;
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
static sqlite3_stmt *recoverPrepare(
  sqlite3_recover *p,
  sqlite3 *db, 
  const char *zSql
){
  sqlite3_stmt *pStmt = 0;
  if( p->errCode==SQLITE_OK ){
    if( sqlite3_prepare_v2(db, zSql, -1, &pStmt, 0) ){
      recoverDbError(p, db);
    }
  }
  return pStmt;
}
/* …（同文件无关代码省略）… */
static sqlite3_stmt *recoverPreparePrintf(
  sqlite3_recover *p,
  sqlite3 *db, 
  const char *zFmt, ...
){
  sqlite3_stmt *pStmt = 0;
  if( p->errCode==SQLITE_OK ){
    va_list ap;
    char *z;
    va_start(ap, zFmt);
    z = sqlite3_vmprintf(zFmt, ap);
    va_end(ap);
    if( z==0 ){
      p->errCode = SQLITE_NOMEM;
    }else{
      pStmt = recoverPrepare(p, db, z);
      sqlite3_free(z);
    }
  }
  return pStmt;
}
/* …（同文件无关代码省略）… */
static void recoverFinalize(sqlite3_recover *p, sqlite3_stmt *pStmt){
  sqlite3 *db = sqlite3_db_handle(pStmt);
  int rc = sqlite3_finalize(pStmt);
  if( rc!=SQLITE_OK && p->errCode==SQLITE_OK ){
    recoverDbError(p, db);
  }
}
/* …（同文件无关代码省略）… */
static char *recoverMPrintf(sqlite3_recover *p, const char *zFmt, ...){
  va_list ap;
  char *z;
  va_start(ap, zFmt);
  z = sqlite3_vmprintf(zFmt, ap);
  va_end(ap);
  if( p->errCode==SQLITE_OK ){
    if( z==0 ) p->errCode = SQLITE_NOMEM;
  }else{
    sqlite3_free(z);
    z = 0;
  }
  return z;
}
/* …（同文件无关代码省略）… */
        csr += (n+1);
      }

      pNew->pNext = p->pTblList;
      p->pTblList = pNew;
      pNew->bIntkey = 1;
    }

    recoverFinalize(p, pStmt);

    pStmt = recoverPreparePrintf(p, p->dbOut, "PRAGMA index_xinfo(%Q)", zName);
    while( pStmt && sqlite3_step(pStmt)==SQLITE_ROW ){
      int iField = sqlite3_column_int(pStmt, 0);
      int iCol = sqlite3_column_int(pStmt, 1);

      assert( iCol<pNew->nCol );
      pNew->aCol[iCol].iField = iField;

      pNew->bIntkey = 0;
      iPk = -2;
    }
    recoverFinalize(p, pStmt);
/* …（同文件无关代码省略）… */

  if( nField<=0 ) return 0;

  assert( nField<=pTab->nCol );

  zSql = recoverMPrintf(p, "INSERT OR IGNORE INTO %Q(", pTab->zTab);

/* …（同文件无关代码省略）… */
    zSep = ", ";
  }

  for(ii=0; ii<nField; ii++){
    int eHidden = pTab->aCol[ii].eHidden;
    if( eHidden!=RECOVER_EHIDDEN_VIRTUAL
     && eHidden!=RECOVER_EHIDDEN_STORED
    ){
      assert( pTab->aCol[ii].iField>=0 && pTab->aCol[ii].iBind>=1 );
      zSql = recoverMPrintf(p, "%z%s%Q", zSql, zSep, pTab->aCol[ii].zCol);

      if( bSql ){
/* …（同文件无关代码省略）… */
  RecoverTable *
