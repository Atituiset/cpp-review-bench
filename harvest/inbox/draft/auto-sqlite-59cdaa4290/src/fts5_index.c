// AUTO-DRAFT from sqlite/sqlite PR #18e33b9a201bfce0351e8ed4a19ae6b89efd4eb6
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

typedef struct Fts5Structure Fts5Structure;
/* …（同文件无关代码省略）… */
struct Fts5Index {
  Fts5Config *pConfig;            /* Virtual table configuration */
  char *zDataTbl;                 /* Name of %_data table */
  int nWorkUnit;                  /* Leaf pages in a "unit" of work */

  /*
  ** Variables related to the accumulation of tokens and doclists within the
  ** in-memory hash tables before they are flushed to disk.
  */
  Fts5Hash *pHash;                /* Hash table for in-memory data */
  int nPendingData;               /* Current bytes of pending data */
  i64 iWriteRowid;                /* Rowid for current doc being written */
  int bDelete;                    /* Current write is a delete */
  int nContentlessDelete;         /* Number of contentless delete ops */
  int nPendingRow;                /* Number of INSERT in hash table */

  /* Error state. */
  int rc;                         /* Current error code */
  int flushRc;

  /* State used by the fts5DataXXX() functions. */
  sqlite3_blob *pReader;          /* RO incr-blob open on %_data table */
  sqlite3_stmt *pWriter;          /* "INSERT ... %_data VALUES(?,?)" */
  sqlite3_stmt *pDeleter;         /* "DELETE FROM %_data ... id>=? AND id<=?" */
  sqlite3_stmt *pIdxWriter;       /* "INSERT ... %_idx VALUES(?,?,?,?)" */
  sqlite3_stmt *pIdxDeleter;      /* "DELETE FROM %_idx WHERE segid=?" */
  sqlite3_stmt *pIdxSelect;
  sqlite3_stmt *pIdxNextSelect;
  int nRead;                      /* Total number of blocks read */

  sqlite3_stmt *pDeleteFromIdx;

  sqlite3_stmt *pDataVersion;
  i64 iStructVersion;             /* data_version when pStruct read */
  Fts5Structure *pStruct;         /* Current db structure (or NULL) */
};
/* …（同文件无关代码省略）… */
static int fts5IndexCorruptIdx(Fts5Index *pIdx){
  pIdx->rc = FTS5_CORRUPT;
  sqlite3Fts5ConfigErrmsg(pIdx->pConfig, 
      "fts5: corruption in table \"%s\"", pIdx->pConfig->zName
  );
  return SQLITE_CORRUPT_VTAB;
}
#define FTS5_CORRUPT_IDX(pIdx) fts5IndexCorruptIdx(pIdx)
/* …（同文件无关代码省略）… */
  }
}

/*
** This is called when a secure-delete operation removes a position-list
** that overflows onto segment page iPgno of segment pSeg. This function
/* …（同文件无关代码省略）… */
      if( (iKeyOff+nSuffix)>(u64)iPgIdx || (iNextOff+nSuffix2)>(u64)iPgIdx ){
        FTS5_CORRUPT_IDX(p);
      }else{
        if( iKey!=1 ){
          iOff += sqlite3Fts5PutVarint(&aPg[iOff], nPrefix);
        }
/* …（同文件无关代码省略）… */
    sqlite3_finalize(p->pIdxNextSelect);
    sqlite3_finalize(p->pDataVersion);
    sqlite3_finalize(p->pDeleteFromIdx);
    sqlite3Fts5HashFree(p->pHash);
    sqlite3_free(p->zDataTbl);
    sqlite3_free(p);
