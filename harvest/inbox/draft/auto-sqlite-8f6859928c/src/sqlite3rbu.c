// AUTO-DRAFT from sqlite/sqlite PR #e5cb4b09573aeb53cbb28983be7234e42c48c07a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
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
#define RBU_PK_NONE           1
/* …（同文件无关代码省略）… */
#define RBU_PK_EXTERNAL       3
#define RBU_PK_WITHOUT_ROWID  4
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
#define rbuIsVacuum(p) ((p)->zTarget==0)
/* …（同文件无关代码省略）… */
  return p->rc;
}

/*
** Attempt to allocate and return a pointer to a zeroed block of nByte 
** bytes. 
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

/*
** Finalize the statement passed as the second argument.
**
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
        /* In a STRICT table, ANY means no affinity. In the non-STRICT
        ** imposter it would mean NUMERIC, so declare it without a type. */
        if( bStrict && sqlite3_stricmp(zType, "ANY")==0 ) zType = "";
        pIter->azTblType[iOrder] = rbuStrndup(zType, &p->rc);
        pIter->azTblDflt[iOrder] = rbuStrndup(zDflt, &p->rc);
        assert( iPk>=0 );
        pIter->abTblPk[iOrder] = (u8)iPk;
/* …（同文件无关代码省略）… */
    rbuFinalize(p, pXInfo);

    sqlite3_test_control(SQLITE_TESTCTRL_IMPOSTER, p->dbMain, "main", 1, tnum);
    rbuMPrintfExec(p, p->dbMain,
        "CREATE TABLE rbu_imposter2(%z, PRIMARY KEY(%z)) WITHOUT ROWID", 
        zCols, zPk
    );
/* …（同文件无关代码省略）… */
    }

    sqlite3_test_control(SQLITE_TESTCTRL_IMPOSTER, p->dbMain, "main", 1, tnum);
    rbuMPrintfExec(p, p->dbMain, "CREATE TABLE \"rbu_imp_%w\"(%z)%s", 
        pIter->zTbl, zSql, 
        (pIter->eType==RBU_PK_WITHOUT_ROWID ? " WITHOUT ROWID" : "")
    );
/* …（同文件无关代码省略）… */
      /* Create the imposter table used to write to this index. */
      sqlite3_test_control(SQLITE_TESTCTRL_IMPOSTER, p->dbMain, "main", 0, 1);
      sqlite3_test_control(SQLITE_TESTCTRL_IMPOSTER, p->dbMain, "main", 1,tnum);
      rbuMPrintfExec(p, p->dbMain,
          "CREATE TABLE \"rbu_imp_%w\"( %s, PRIMARY KEY( %s ) ) WITHOUT ROWID",
          zTbl, zImposterCols, zImposterPK
      );
/* …（同文件无关代码省略）… */
        }

        /* Create the rbu_tmp_xxx table and the triggers to populate it. */
        rbuMPrintfExec(p, p->dbRbu,
            "CREATE TABLE IF NOT EXISTS %s.'rbu_tmp_%q' AS "
            "SELECT *%s FROM '%q' WHERE 0;"
            , p->zStateDb, pIter->zDataTbl
/* …（同文件无关代码省略）… */
        );

        if( pIter->eType==RBU_PK_EXTERNAL || pIter->eType==RBU_PK_NONE ){
          rbuMPrintfExec(p, p->dbMain,
              "CREATE TEMP TRIGGER rbu_insert_tr AFTER INSERT ON \"%s%w\" "
              "BEGIN "
              "  SELECT rbu_tmp_insert(0, %s);"
/* …（同文件无关代码省略）… */
  /* If using separate RBU and state databases, attach the state database to
  ** the RBU db handle now.  */
  if( p->zState ){
    rbuMPrintfExec(p, p->dbRbu, "ATTACH %Q AS stat", p->zState);
    memcpy(p->zStateDb, "stat", 4);
  }else{
    memcpy(p->zStateDb, "main", 4);
/* …（同文件无关代码省略）… */
  if( p->rc==SQLITE_OK ){
    p->rc = sqlite3_file_control(p->dbMain, "main", SQLITE_FCNTL_RBU, (void*)p);
  }
  rbuMPrintfExec(p, p->dbMain, "SELECT * FROM sqlite_schema");

  /* Mark the database file just opened as an RBU target database. If 
  ** this call returns SQLITE_NOTFOUND, then the RBU vfs is not in use.
/* …（同文件无关代码省
