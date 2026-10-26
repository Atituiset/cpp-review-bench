// AUTO-DRAFT from sqlite/sqlite PR #6fd45c5ce98159eeb49421092672b3d31447a173
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define FTS5_DATA_DLI_B     1     /* Doclist-index flag (1 bit) */
#define FTS5_DATA_HEIGHT_B  5     /* Max dlidx tree height of 32 */
#define FTS5_DATA_PAGE_B   31     /* 31-bit page numbers */
/* …（同文件无关代码省略）… */
#define fts5_dri(segid, dlidx, height, pgno) (                                 \
 ((i64)(segid)  << (FTS5_DATA_PAGE_B+FTS5_DATA_HEIGHT_B+FTS5_DATA_DLI_B)) +    \
 ((i64)(dlidx)  << (FTS5_DATA_PAGE_B + FTS5_DATA_HEIGHT_B)) +                  \
 ((i64)(height) << (FTS5_DATA_PAGE_B)) +                                       \
 ((i64)(pgno))                                                                 \
)
/* …（同文件无关代码省略）… */
#define FTS5_SEGMENT_ROWID(segid, pgno)       fts5_dri(segid, 0, 0, pgno)
/* …（同文件无关代码省略）… */
#define FTS5_DATA_PADDING 20
/* …（同文件无关代码省略）… */
typedef struct Fts5Data Fts5Data;
typedef struct Fts5DlidxIter Fts5DlidxIter;
/* …（同文件无关代码省略）… */
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
static int fts5IndexCorruptRowid(Fts5Index *pIdx, i64 iRowid){
  pIdx->rc = FTS5_CORRUPT;
  sqlite3Fts5ConfigErrmsg(pIdx->pConfig, 
      "fts5: corruption found reading blob %lld from table \"%s\"", 
      iRowid, pIdx->pConfig->zName
  );
  return SQLITE_CORRUPT_VTAB;
}
#define FTS5_CORRUPT_ROWID(pIdx, iRowid) fts5IndexCorruptRowid(pIdx, iRowid)
/* …（同文件无关代码省略）… */
static u16 fts5GetU16(const u8 *aIn){
  return ((u16)aIn[0] << 8) + aIn[1];
} 
/* …（同文件无关代码省略）… */
static void fts5IndexCloseReader(Fts5Index *p){
  if( p->pReader ){
    int rc;
    sqlite3_blob *pReader = p->pReader;
    p->pReader = 0;
    rc = sqlite3_blob_close(pReader);
    if( p->rc==SQLITE_OK ) p->rc = rc;
  }
}
/* …（同文件无关代码省略）… */
static Fts5Data *fts5DataRead(Fts5Index *p, i64 iRowid){
  Fts5Data *pRet = 0;
  if( p->rc==SQLITE_OK ){
    int rc = SQLITE_OK;

    if( p->pReader ){
      /* This call may return SQLITE_ABORT if there has been a savepoint
      ** rollback since it was last used. In this case a new blob handle
      ** is required.  */
      sqlite3_blob *pBlob = p->pReader;
      p->pReader = 0;
      rc = sqlite3_blob_reopen(pBlob, iRowid);
      assert( p->pReader==0 );
      p->pReader = pBlob;
      if( rc!=SQLITE_OK ){
        fts5IndexCloseReader(p);
      }
      if( rc==SQLITE_ABORT ) rc = SQLITE_OK;
    }

    /* If the blob handle is not open at this point, open it and seek 
    ** to the requested entry.  */
    if( p->pReader==0 && rc==SQLITE_OK ){
      Fts5Config *pConfig = p->pConfig;
      rc = sqlite3_blob_open(pConfig->db, 
          pConfig->zDb, p->zDataTbl, "block", iRowid, 0, &p->pReader
      );
    }

    /* If either of the sqlite3_blob_open() or sqlite3_blob_reopen() calls
    ** above returned SQLITE_ERROR, return SQLITE_CORRUPT_VTAB instead.
    ** All the reasons those functions might return SQLITE_ERROR - missing
    ** table, missing row, non-blob/text in block column - indicate 
    ** backing store corruption.  */
    if( rc==SQLITE_ERROR ) rc = FTS5_CORRUPT_ROWID(p, iRowid);

    if( rc==SQLITE_OK ){
      u8 *aOut = 0;               /* Read blob data into this buffer */
      i64 nByte = sqlite3_blob_bytes(p->pReader);
      i64 szData = (sizeof(Fts5Data) + 7) & ~7;
      i64 nAlloc = szData + nByte + FTS5_DATA_PADDING;
      pRet = (Fts5Data*)sqlite3_malloc64(nAlloc);
      if( pRet ){
        pRet->nn = nByte;
        pRet->szLeaf = 0;
        aOut = pRet->p = (u8*)pRet + szData;
      }else{
        rc = SQLITE_NOMEM;
      }

      if( rc==SQLITE_OK ){
        rc = sqlite3_blob_read(p->pReader, aOut, nByte, 0);
      }
      if( rc!=SQLITE_OK ){
        sqlite3_free(pRet);
        pRet = 0;
      }else{
        memset(&pRet->p[nByte], 0, FTS5_DATA_PADDING);
      }
    }
    p->rc = rc;
    p->nRead++;
  }

  assert( (pRet==0)==(p->rc!=SQLITE_OK) );
  assert( pRet==0 || EIGHT_BYTE_ALIGNMENT( pRet->p ) );
  return pRet;
}
/* …（同文件无关代码省略）… */
static void fts5DataRelease(Fts5Data *pData){
  sqlite3_free(pData);
}
/* …（同文件无关代码省略）… */
static Fts5Data *fts5LeafRead(Fts5Index *p, i64 iRowid){
  Fts5Data *pRet = fts5DataRead(p, iRowid);
  if( pRet ){
    assert( pRet->szLeaf==0 );
    if( pRet->nn>=4 ){
      pRet->szLeaf = fts5GetU16(&pRet->p[2]);
    }
    if( pRet->szLeaf<4 || pRet->szLeaf>pRet->nn ){
      FTS5_CORRUPT_ROWID(p, iRowid);
      fts5DataRelease(pRet);
      pRet = 0;
    }
  }
  return pRet;
}
/* …（同文件无关代码省略）… */
  }
}


/*
** Advance the iterator passed as the only argument. If the end of the 
/* …（同文件无关代码省略）… */

    if( iOff<pData->nn ){
      u64 iVal;
      pLvl->iLeafPgno += (iOff - pLvl->iOff) + 1;
      iOff += fts5GetVarint(&pData->p[iOff], &iVal);
      pLvl->iRowid += iVal;
      pLvl->iOff = iOff;
/* …（同文件无关代码省略）… */
      ii += sqlite3Fts5GetVarint(&a[ii], &delta);

      if( ii>=iOff ) break;
      pLvl->iLeafPgno += nZero+1;
      pLvl->iRowid += delta;
      pLvl->iOff = ii;
    }
/* …（同文件无关代码省略）… */
static int fts5DlidxIterPgno(Fts5DlidxIter *pIter){
  return pIter->aLvl[0].iLeafPgno;
}
/* …（同文件无关代码省略）… */
      ){

        /* Check any rowid-less pages that occur before the current leaf. */
        for(iPg=iPrevLeaf+1; iPg<fts5DlidxIterPgno(pDlidx); iPg++){
          iKey = FTS5_SEGMENT_ROWID(iSegid, iPg);
          pLeaf = fts5LeafRead(p, iKey);
          if( pLeaf ){
