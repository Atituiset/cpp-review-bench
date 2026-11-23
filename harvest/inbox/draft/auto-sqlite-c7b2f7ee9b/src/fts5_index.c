// AUTO-DRAFT from sqlite/sqlite PR #fe3fb0a154009420268c5b7d1afd5a860a03b4b7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
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
#define FTS5_TOMBSTONE_ROWID(segid,ipg)       fts5_dri(segid+(1<<16), 0, 0, ipg)
/* …（同文件无关代码省略）… */
#define FTS5_DATA_PADDING 20
/* …（同文件无关代码省略）… */
typedef struct Fts5Data Fts5Data;
typedef struct Fts5DlidxIter Fts5DlidxIter;
/* …（同文件无关代码省略）… */
typedef struct Fts5Iter Fts5Iter;
/* …（同文件无关代码省略）… */
typedef struct Fts5SegIter Fts5SegIter;
/* …（同文件无关代码省略）… */
typedef struct Fts5Structure Fts5Structure;
typedef struct Fts5StructureLevel Fts5StructureLevel;
typedef struct Fts5StructureSegment Fts5StructureSegment;
/* …（同文件无关代码省略）… */
typedef struct Fts5TombstoneArray Fts5TombstoneArray;
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
  int bInSync;                    /* True while inside sqlite3Fts5IndexSync() */

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
  sqlite3_stmt *pIdxKeyUpdate;    /* UPDATE %_idx SET term WHERE (segid,pgno) */

  sqlite3_stmt *pDataVersion;
  i64 iStructVersion;             /* data_version when pStruct read */
  Fts5Structure *pStruct;         /* Current db structure (or NULL) */
};
/* …（同文件无关代码省略）… */
typedef struct Fts5CResult Fts5CResult;
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
#define FTS5_CORRUPT_ITER(pIdx, pIter) fts5IndexCorruptIter(pIdx, pIter)
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
#define SZ_FTS5TOMBSTONEARRAY(N) \
  (offsetof(Fts5TombstoneArray,apTombstone)+(N)*sizeof(Fts5Data*))
/* …（同文件无关代码省略）… */
#define ASSERT_SZLEAF_OK(x) assert( \
    (x)->szLeaf==(x)->nn || (x)->szLeaf==fts5GetU16(&(x)->p[2]) \
)
/* …（同文件无关代码省略）… */
#define FTS5_SEGITER_ONETERM 0x01
#define FTS5_SEGITER_REVERSE 0x02
/* …（同文件无关代码省略）… */
#define fts5LeafIsTermless(x) ((x)->szLeaf >= (x)->nn)
/* …（同文件无关代码省略）… */
#define fts5LeafFirstRowidOff(x) (fts5GetU16((x)->p))
/* …（同文件无关代码省略）… */
#define SZ_FTS5ITER(N)  (offsetof(Fts5Iter,aSeg)+(N)*sizeof(Fts5SegIter))
/* …（同文件无关代码省略）… */
static void fts5PutU16(u8 *aOut, u16 iVal){
  aOut[0] = (iVal>>8);
  aOut[1] = (iVal&0xFF);
}
/* …（同文件无关代码省略）… */
static u16 fts5GetU16(const u8 *aIn){
  return ((u16)aIn[0] << 8) + aIn[1];
} 
/* …（同文件无关代码省略）… */
static void *fts5IdxMalloc(Fts5Index *p, sqlite3_int64 nByte){
  return sqlite3Fts5MallocZero(&p->rc, nByte);
}
/* …（同文件无关代码省略）… */
static int fts5BufferCompareBlob(
  Fts5Buffer *pLeft,              /* Left hand side of comparison */
  const u8 *pRight, int nRight    /* Right hand side of comparison */
){
  int nCmp = MIN(pLeft->n, nRight);
  int res = memcmp(pLeft->p, pRight, nCmp);
  return (res==0 ? (pLeft->n - nRight) : res);
}
/* …（同文件无关代码省略）… */
static int fts5BufferCompare(Fts5Buffer *pLeft, Fts5Buffer *pRight){
  int nCmp, res;
  nCmp = MIN(pLeft->n, pRight->n);
  assert( nCmp<=0 || pLeft->p!=0 );
  assert( nCmp<=0 || pRight->p!=0 );
  res = fts5Memcmp(pLeft->p, pRight->p, nCmp);
  return (res==0 ? (pLeft->n - pRight->n) : res);
}
/* …（同文件无关代码省略）… */
static int fts5LeafFirstTermOff(Fts5Data *pLeaf){
  int ret;
  fts5GetVarint32(&pLeaf->p[pLeaf->szLeaf], ret);
  return ret;
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
static int fts5IndexPrepareStmt(
  Fts5Index *p,
  sqlite3_stmt **ppStmt,
  char *zSql
){
  if( p->rc==SQLITE_OK ){
    if( zSql ){
      int rc = sqlite3_prepare_v3(p->pConfig->db, zSql, -1,
          SQLITE_PREPARE_PERSISTENT|SQLITE_PREPARE_NO_VTAB
          |SQLITE_PREPARE_FROM_DDL,
          ppStmt, 0);
      /* If this prepare() call fails with SQLITE_ERROR, then one of the
      ** %_idx or %_data tables has been removed or modified. Call this
      ** corruption.  */
      p->rc = (rc==SQLITE_ERROR ? SQLITE_CORRUPT : rc);
    }else{
      p->rc = SQLITE_NOMEM;
    }
  }
  sqlite3_free(zSql);
  return p->rc;
}
/* …（同文件无关代码省略）… */
static void fts5StepAndReset(int *pRc, sqlite3_stmt *pStmt){
  if( *pRc==SQLITE_OK ){
    sqlite3_step(pStmt);
    *pRc = sqlite3_reset(pStmt);
  }
}
/* …（同文件无关代码省略）… */
static void fts5DataWrite(Fts5Index *p, i64 iRowid, const u8 *pData, int nData){
  if( p->rc!=SQLITE_OK ) return;

  if( p->pWriter==0 ){
    Fts5Config *pConfig = p->pConfig;
    fts5IndexPrepareStmt(p, &p->pWriter, sqlite3_mprintf(
          "REPLACE INTO '%q'.'%q_data'(id, block) VALUES(?,?)", 
          pConfig->zDb, pConfig->zName
    ));
    if( p->rc ) return;
  }

  sqlite3_bind_int64(p->pWriter, 1, iRowid);
  p->rc = sqlite3_bind_blob(p->pWriter, 2, pData, nData, SQLITE_STATIC);
  fts5StepAndReset(&p->rc, p->pWriter);
  sqlite3_bind_null(p->pWriter, 2);
}
/* …（同文件无关代码省略）… */
static int fts5StructureCountSegments(Fts5Structure *pStruct){
  int nSegment = 0;               /* Total number of segments */
  if( pStruct ){
    int iLvl;                     /* Used to iterate through levels */
    for(iLvl=0; iLvl<pStruct->nLevel; iLvl++){
      nSegment += pStruct->aLevel[iLvl].nSeg;
    }
  }

  return nSegment;
}
/* …（同文件无关代码省略）… */
static int fts5DlidxIterNext(Fts5Index *p, Fts5DlidxIter *pIter){
  return fts5DlidxIterNextR(p, pIter, 0);
}
/* …（同文件无关代码省略）… */
static int fts5DlidxIterEof(Fts5Index *p, Fts5DlidxIter *pIter){
  return p->rc!=SQLITE_OK || pIter->aLvl[0].bEof;
}
/* …（同文件无关代码省略）… */
static int fts5DlidxIterPrev(Fts5Index *p, Fts5DlidxIter *pIter){
  return fts5DlidxIterPrevR(p, pIter, 0);
}
/* …（同文件无关代码省略）… */
static void fts5DlidxIterFree(Fts5DlidxIter *pIter){
  if( pIter ){
    int i;
    for(i=0; i<pIter->nLvl; i++){
      fts5DataRelease(pIter->aLvl[i].pData);
    }
    sqlite3_free(pIter);
  }
}
/* …（同文件无关代码省略）… */
static Fts5DlidxIter *fts5DlidxIterInit(
  Fts5Index *p,                   /* Fts5 Backend to iterate within */
  int bRev,                       /* True for ORDER BY ASC */
  int iSegid,                     /* Segment id */
  int iLeafPg                     /* Leaf page number to load dlidx for */
){
  Fts5DlidxIter *pIter = 0;
  int i;
  int bDone = 0;

  for(i=0; p->rc==SQLITE_OK && bDone==0; i++){
    sqlite3_int64 nByte = SZ_FTS5DLIDXITER(i+1);
    Fts5DlidxIter *pNew;

    pNew = (Fts5DlidxIter*)sqlite3_realloc64(pIter, nByte);
    if( pNew==0 ){
      p->rc = SQLITE_NOMEM;
    }else{
      i64 iRowid = FTS5_DLIDX_ROWID(iSegid, i, iLeafPg);
      Fts5DlidxLvl *pLvl = &pNew->aLvl[i];
      pIter = pNew;
      memset(pLvl, 0, sizeof(Fts5DlidxLvl));
      pLvl->pData = fts5DataRead(p, iRowid);
      if( pLvl->pData && (pLvl->pData->p[0] & 0x0001)==0 ){
        bDone = 1;
      }
      pIter->nLvl = i+1;
      if( pIter->nLvl > (1<<FTS5_DATA_HEIGHT_B) ){
        p->rc = FTS5_CORRUPT;
      }
    }
  }

  if( p->rc==SQLITE_OK ){
    pIter->iSegid = iSegid;
    if( bRev==0 ){
      fts5DlidxIterFirst(pIter);
    }else{
      fts5DlidxIterLast(p, pIter);
    }
  }

  if( p->rc!=SQLITE_OK ){
    fts5DlidxIterFree(pIter);
    pIter = 0;
  }

  return pIter;
}
/* …（同文件无关代码省略）… */
static i64 fts5DlidxIterRowid(Fts5DlidxIter *pIter){
  return pIter->aLvl[0].iRowid;
}
static int fts5DlidxIterPgno(Fts5DlidxIter *pIter){
  return pIter->aLvl[0].iLeafPgno;
}
/* …（同文件无关代码省略）… */
static void fts5SegIterNextPage(
  Fts5Index *p,                   /* FTS5 backend object */
  Fts5SegIter *pIter              /* Iterator to advance to next page */
){
  Fts5Data *pLeaf;
  Fts5StructureSegment *pSeg = pIter->pSeg;
  fts5DataRelease(pIter->pLeaf);
  pIter->iLeafPgno++;
  if( pIter->pNextLeaf ){
    pIter->pLeaf = pIter->pNextLeaf;
    pIter->pNextLeaf = 0;
  }else if( pIter->iLeafPgno<=pSeg->pgnoLast ){
    pIter->pLeaf = fts5LeafRead(p, 
        FTS5_SEGMENT_ROWID(pSeg->iSegid, pIter->iLeafPgno)
    );
  }else{
    pIter->pLeaf = 0;
  }
  pLeaf = pIter->pLeaf;

  if( pLeaf ){
    pIter->iPgidxOff = pLeaf->szLeaf;
    if( fts5LeafIsTermless(pLeaf) ){
      pIter->iEndofDoclist = pLeaf->nn+1;
    }else{
      pIter->iPgidxOff += fts5GetVarint32(&pLeaf->p[pIter->iPgidxOff],
          pIter->iEndofDoclist
      );
    }
  }
}
/* …（同文件无关代码省略）… */
static int fts5GetPoslistSize(const u8 *p, int *pnSz, int *pbDel){
  int nSz;
  int n = 0;
  fts5FastGetVarint32(p, n, nSz);
  assert_nc( nSz>=0 );
  *pnSz = nSz/2;
  *pbDel = nSz & 0x0001;
  return n;
}
/* …（同文件无关代码省略）… */
static void fts5SegIterLoadNPos(Fts5Index *p, Fts5SegIter *pIter){
  if( p->rc==SQLITE_OK ){
    int iOff = pIter->iLeafOffset;  /* Offset to read at */
    ASSERT_SZLEAF_OK(pIter->pLeaf);
    if( p->pConfig->eDetail==FTS5_DETAIL_NONE ){
      int iEod = MIN(pIter->iEndofDoclist, pIter->pLeaf->szLeaf);
      pIter->bDel = 0;
      pIter->nPos = 1;
      if( iOff<iEod && pIter->pLeaf->p[iOff]==0 ){
        pIter->bDel = 1;
        iOff++;
        if( iOff<iEod && pIter->pLeaf->p[iOff]==0 ){
          pIter->nPos = 1;
          iOff++;
        }else{
          pIter->nPos = 0;
        }
      }
    }else{
      int nSz;
      fts5FastGetVarint32(pIter->pLeaf->p, iOff, nSz);
      pIter->bDel = (nSz & 0x0001);
      pIter->nPos = nSz>>1;
      assert_nc( pIter->nPos>=0 );
    }
    pIter->iLeafOffset = iOff;
  }
}
/* …（同文件无关代码省略）… */
static void fts5SegIterLoadRowid(Fts5Index *p, Fts5SegIter *pIter){
  u8 *a = pIter->pLeaf->p;        /* Buffer to read data from */
  i64 iOff = pIter->iLeafOffset;

  ASSERT_SZLEAF_OK(pIter->pLeaf);
  while( iOff>=pIter->pLeaf->szLeaf ){
    fts5SegIterNextPage(p, pIter);
    if( pIter->pLeaf==0 ){
      if( p->rc==SQLITE_OK ) FTS5_CORRUPT_ITER(p, pIter);
      return;
    }
    iOff = 4;
    a = pIter->pLeaf->p;
  }
  iOff += sqlite3Fts5GetVarint(&a[iOff], (u64*)&pIter->iRowid);
  pIter->iLeafOffset = iOff;
}
/* …（同文件无关代码省略）… */
static void fts5SegIterLoadTerm(Fts5Index *p, Fts5SegIter *pIter, int nKeep){
  u8 *a = pIter->pLeaf->p;        /* Buffer to read data from */
  i64 iOff = pIter->iLeafOffset;  /* Offset to read at */
  int nNew;                       /* Bytes of new data */

  iOff += fts5GetVarint32(&a[iOff], nNew);
  if( iOff+nNew>pIter->pLeaf->szLeaf || nKeep>pIter->term.n || nNew==0 ){
    FTS5_CORRUPT_ITER(p, pIter);
    return;
  }
  pIter->term.n = nKeep;
  fts5BufferAppendBlob(&p->rc, &pIter->term, nNew, &a[iOff]);
  assert( pIter->term.n<=pIter->term.nSpace );
  iOff += nNew;
  pIter->iTermLeafOffset = iOff;
  pIter->iTermLeafPgno = pIter->iLeafPgno;
  pIter->iLeafOffset = iOff;

  if( pIter->iPgidxOff>=pIter->pLeaf->nn ){
    pIter->iEndofDoclist = pIter->pLeaf->nn+1;
  }else{
    int nExtra;
    pIter->iPgidxOff += fts5GetVarint32(&a[pIter->iPgidxOff], nExtra);
    pIter->iEndofDoclist += nExtra;
  }

  fts5SegIterLoadRowid(p, pIter);
}
/* …（同文件无关代码省略）… */
static void fts5SegIterSetNext(Fts5Index *p, Fts5SegIter *pIter){
  if( pIter->flags & FTS5_SEGITER_REVERSE ){
    pIter->xNext = fts5SegIterNext_Reverse;
  }else if( p->pConfig->eDetail==FTS5_DETAIL_NONE ){
    pIter->xNext = fts5SegIterNext_None;
  }else{
    pIter->xNext = fts5SegIterNext;
  }
}
/* …（同文件无关代码省略）… */
static void fts5SegIterAllocTombstone(Fts5Index *p, Fts5SegIter *pIter){
  const i64 nTomb = (i64)pIter->pSeg->nPgTombstone;
  if( nTomb>0 ){
    i64 nByte = SZ_FTS5TOMBSTONEARRAY(nTomb+1);
    Fts5TombstoneArray *pNew;
    pNew = (Fts5TombstoneArray*)sqlite3Fts5MallocZero(&p->rc, nByte);
    if( pNew ){
      pNew->nTombstone = nTomb;
      pNew->nRef = 1;
      pIter->pTombArray = pNew;
    }
  }
}
/* …（同文件无关代码省略）… */
static void fts5SegIterInit(
  Fts5Index *p,                   /* FTS index object */
  Fts5StructureSegment *pSeg,     /* Description of segment */
  Fts5SegIter *pIter              /* Object to populate */
){
  if( pSeg->pgnoFirst==0 ){
    /* This happens if the segment is being used as an input to an incremental
    ** merge and all data has already been "trimmed". See function
    ** fts5TrimSegments() for details. In this case leave the iterator empty.
    ** The caller will see the (pIter->pLeaf==0) and assume the iterator is
    ** at EOF already. */
    assert( pIter->pLeaf==0 );
    return;
  }

  if( p->rc==SQLITE_OK ){
    memset(pIter, 0, sizeof(*pIter));
    fts5SegIterSetNext(p, pIter);
    pIter->pSeg = pSeg;
    pIter->iLeafPgno = pSeg->pgnoFirst-1;
    do {
      fts5SegIterNextPage(p, pIter);
    }while( p->rc==SQLITE_OK && pIter->pLeaf && pIter->pLeaf->nn==4 );
  }

  if( p->rc==SQLITE_OK && pIter->pLeaf ){
    pIter->iLeafOffset = 4;
    assert( pIter->pLeaf!=0 );
    assert_nc( pIter->pLeaf->nn>4 );
    assert_nc( fts5LeafFirstTermOff(pIter->pLeaf)==4 );
    pIter->iPgidxOff = pIter->pLeaf->szLeaf+1;
    fts5SegIterLoadTerm(p, pIter, 0);
    fts5SegIterLoadNPos(p, pIter);
    fts5SegIterAllocTombstone(p, pIter);
  }
}
/* …（同文件无关代码省略）… */
static void fts5SegIterReverseInitPage(Fts5Index *p, Fts5SegIter *pIter){
  int eDetail = p->pConfig->eDetail;
  int n = pIter->pLeaf->szLeaf;
  int i = pIter->iLeafOffset;
  u8 *a = pIter->pLeaf->p;
  int iRowidOffset = 0;

  if( n>pIter->iEndofDoclist ){
    n = pIter->iEndofDoclist;
  }

  ASSERT_SZLEAF_OK(pIter->pLeaf);
  while( 1 ){
    u64 iDelta = 0;

    if( i>=n ) break;
    if( eDetail==FTS5_DETAIL_NONE ){
      /* todo */
      if( i<n && a[i]==0 ){
        i++;
        if( i<n && a[i]==0 ) i++;
      }
    }else{
      int nPos;
      int bDummy;
      i += fts5GetPoslistSize(&a[i], &nPos, &bDummy);
      i += nPos;
    }
    if( i>=n ) break;
    i += fts5GetVarint(&a[i], &iDelta);
    pIter->iRowid += iDelta;

    /* If necessary, grow the pIter->aRowidOffset[] array. */
    if( iRowidOffset>=pIter->nRowidOffset ){
      i64 nNew = pIter->nRowidOffset + 8;
      int *aNew = (int*)sqlite3_realloc64(pIter->aRowidOffset,nNew*sizeof(int));
      if( aNew==0 ){
        p->rc = SQLITE_NOMEM;
        break;
      }
      pIter->aRowidOffset = aNew;
      pIter->nRowidOffset = nNew;
    }

    pIter->aRowidOffset[iRowidOffset++] = pIter->iLeafOffset;
    pIter->iLeafOffset = i;
  }
  pIter->iRowidOffset = iRowidOffset;
  fts5SegIterLoadNPos(p, pIter);
}
/* …（同文件无关代码省略）… */
static void fts5SegIterReverseNewPage(Fts5Index *p, Fts5SegIter *pIter){
  assert( pIter->flags & FTS5_SEGITER_REVERSE );
  assert( pIter->flags & FTS5_SEGITER_ONETERM );

  fts5DataRelease(pIter->pLeaf);
  pIter->pLeaf = 0;
  while( p->rc==SQLITE_OK && pIter->iLeafPgno>pIter->iTermLeafPgno ){
    Fts5Data *pNew;
    pIter->iLeafPgno--;
    pNew = fts5LeafRead(p, FTS5_SEGMENT_ROWID(
          pIter->pSeg->iSegid, pIter->iLeafPgno
    ));
    if( pNew ){
      /* iTermLeafOffset may be equal to szLeaf if the term is the last
      ** thing on the page - i.e. the first rowid is on the following page.
      ** In this case leave pIter->pLeaf==0, this iterator is at EOF. */
      if( pIter->iLeafPgno==pIter->iTermLeafPgno ){
        assert( pIter->pLeaf==0 );
        if( pIter->iTermLeafOffset<pNew->szLeaf ){
          pIter->pLeaf = pNew;
          pIter->iLeafOffset = pIter->iTermLeafOffset;
        }
      }else{
        int iRowidOff;
        iRowidOff = fts5LeafFirstRowidOff(pNew);
        if( iRowidOff ){
          if( iRowidOff>=pNew->szLeaf ){
            FTS5_CORRUPT_ITER(p, pIter);
          }else{
            pIter->pLeaf = pNew;
            pIter->iLeafOffset = iRowidOff;
          }
        }
      }

      if( pIter->pLeaf ){
        u8 *a = &pIter->pLeaf->p[pIter->iLeafOffset];
        pIter->iLeafOffset += fts5GetVarint(a, (u64*)&pIter->iRowid);
        break;
      }else{
        fts5DataRelease(pNew);
      }
    }
  }

  if( pIter->pLeaf ){
    pIter->iEndofDoclist = pIter->pLeaf->nn+1;
    fts5SegIterReverseInitPage(p, pIter);
  }
}
/* …（同文件无关代码省略）… */
static int fts5MultiIterIsEmpty(Fts5Index *p, Fts5Iter *pIter){
  Fts5SegIter *pSeg = &pIter->aSeg[pIter->aFirst[1].iFirst];
  return (p->rc==SQLITE_OK && pSeg->pLeaf && pSeg->nPos==0);
}
/* …（同文件无关代码省略）… */
static void fts5SegIterNext_Reverse(
  Fts5Index *p,                   /* FTS5 backend object */
  Fts5SegIter *pIter,             /* Iterator to advance */
  int *pbUnused                   /* Unused */
){
  assert( pIter->flags & FTS5_SEGITER_REVERSE );
  assert( pIter->pNextLeaf==0 );
  UNUSED_PARAM(pbUnused);

  if( pIter->iRowidOffset>0 ){
    u8 *a = pIter->pLeaf->p;
    int iOff;
    u64 iDelta;

    pIter->iRowidOffset--;
    pIter->iLeafOffset = pIter->aRowidOffset[pIter->iRowidOffset];
    fts5SegIterLoadNPos(p, pIter);
    iOff = pIter->iLeafOffset;
    if( p->pConfig->eDetail!=FTS5_DETAIL_NONE ){
      iOff += pIter->nPos;
    }
    fts5GetVarint(&a[iOff], &iDelta);
    pIter->iRowid -= iDelta;
  }else{
    fts5SegIterReverseNewPage(p, pIter);
  }
}
/* …（同文件无关代码省略）… */
static void fts5SegIterNext_None(
  Fts5Index *p,                   /* FTS5 backend object */
  Fts5SegIter *pIter,             /* Iterator to advance */
  int *pbNewTerm                  /* OUT: Set for new term */
){
  int iOff;

  assert( p->rc==SQLITE_OK );
  assert( (pIter->flags & FTS5_SEGITER_REVERSE)==0 );
  assert( p->pConfig->eDetail==FTS5_DETAIL_NONE );

  ASSERT_SZLEAF_OK(pIter->pLeaf);
  iOff = pIter->iLeafOffset;

  /* Next entry is on the next page */
  while( pIter->pSeg && iOff>=pIter->pLeaf->szLeaf ){
    fts5SegIterNextPage(p, pIter);
    if( p->rc || pIter->pLeaf==0 ) return;
    pIter->iRowid = 0;
    iOff = 4;
  }

  if( iOff<pIter->iEndofDoclist ){
    /* Next entry is on the current page */
    u64 iDelta;
    iOff += sqlite3Fts5GetVarint(&pIter->pLeaf->p[iOff], (u64*)&iDelta);
    pIter->iLeafOffset = iOff;
    pIter->iRowid += iDelta;
  }else if( (pIter->flags & FTS5_SEGITER_ONETERM)==0 ){
    if( pIter->pSeg ){
      int nKeep = 0;
      if( iOff!=fts5LeafFirstTermOff(pIter->pLeaf) ){
        iOff += fts5GetVarint32(&pIter->pLeaf->p[iOff], nKeep);
      }
      pIter->iLeafOffset = iOff;
      fts5SegIterLoadTerm(p, pIter, nKeep);
    }else{
      const u8 *pList = 0;
      const char *zTerm = 0;
      int nTerm = 0;
      int nList;
      sqlite3Fts5HashScanNext(p->pHash);
      sqlite3Fts5HashScanEntry(p->pHash, &zTerm, &nTerm, &pList, &nList);
      if( pList==0 ) goto next_none_eof;
      pIter->pLeaf->p = (u8*)pList;
      pIter->pLeaf->nn = nList;
      pIter->pLeaf->szLeaf = nList;
      pIter->iEndofDoclist = nList;
      sqlite3Fts5BufferSet(&p->rc,&pIter->term, nTerm, (u8*)zTerm);
      pIter->iLeafOffset = fts5GetVarint(pList, (u64*)&pIter->iRowid);
    }

    if( pbNewTerm ) *pbNewTerm = 1;
  }else{
    goto next_none_eof;
  }

  fts5SegIterLoadNPos(p, pIter);

  return;
 next_none_eof:
  fts5DataRelease(pIter->pLeaf);
  pIter->pLeaf = 0;
}
/* …（同文件无关代码省略）… */
static void fts5SegIterNext(
  Fts5Index *p,                   /* FTS5 backend object */
  Fts5SegIter *pIter,             /* Iterator to advance */
  int *pbNewTerm                  /* OUT: Set for new term */
){
  Fts5Data *pLeaf = pIter->pLeaf;
  int iOff;
  int bNewTerm = 0;
  int nKeep = 0;
  u8 *a;
  int n;

  assert( pbNewTerm==0 || *pbNewTerm==0 );
  assert( p->pConfig->eDetail!=FTS5_DETAIL_NONE );

  /* Search for the end of the position list within the current page. */
  a = pLeaf->p;
  n = pLeaf->szLeaf;

  ASSERT_SZLEAF_OK(pLeaf);
  iOff = pIter->iLeafOffset + pIter->nPos;

  if( iOff<n ){
    /* The next entry is on the current page. */
    assert_nc( iOff<=pIter->iEndofDoclist );
    if( iOff>=pIter->iEndofDoclist ){
      bNewTerm = 1;
      if( iOff!=fts5LeafFirstTermOff(pLeaf) ){
        iOff += fts5GetVarint32(&a[iOff], nKeep);
      }
    }else{
      u64 iDelta;
      iOff += sqlite3Fts5GetVarint(&a[iOff], &iDelta);
      pIter->iRowid += iDelta;
      assert_nc( iDelta>0 );
    }
    pIter->iLeafOffset = iOff;

  }else if( pIter->pSeg==0 ){
    const u8 *pList = 0;
    const char *zTerm = 0;
    int nTerm = 0;
    int nList = 0;
    assert( (pIter->flags & FTS5_SEGITER_ONETERM) || pbNewTerm );
    if( 0==(pIter->flags & FTS5_SEGITER_ONETERM) ){
      sqlite3Fts5HashScanNext(p->pHash);
      sqlite3Fts5HashScanEntry(p->pHash, &zTerm, &nTerm, &pList, &nList);
    }
    if( pList==0 ){
      fts5DataRelease(pIter->pLeaf);
      pIter->pLeaf = 0;
    }else{
      pIter->pLeaf->p = (u8*)pList;
      pIter->pLeaf->nn = nList;
      pIter->pLeaf->szLeaf = nList;
      pIter->iEndofDoclist = nList+1;
      sqlite3Fts5BufferSet(&p->rc, &pIter->term, nTerm, (u8*)zTerm);
      pIter->iLeafOffset = fts5GetVarint(pList, (u64*)&pIter->iRowid);
      *pbNewTerm = 1;
    }
  }else{
    iOff = 0;
    /* Next entry is not on the current page */
    while( iOff==0 ){
      fts5SegIterNextPage(p, pIter);
      pLeaf = pIter->pLeaf;
      if( pLeaf==0 ) break;
      ASSERT_SZLEAF_OK(pLeaf);
      if( (iOff = fts5LeafFirstRowidOff(pLeaf)) && iOff<pLeaf->szLeaf ){
        iOff += sqlite3Fts5GetVarint(&pLeaf->p[iOff], (u64*)&pIter->iRowid);
        pIter->iLeafOffset = iOff;

        if( pLeaf->nn>pLeaf->szLeaf ){
          pIter->iPgidxOff = pLeaf->szLeaf + fts5GetVarint32(
              &pLeaf->p[pLeaf->szLeaf], pIter->iEndofDoclist
          );
        }
      }
      else if( pLeaf->nn>pLeaf->szLeaf ){
        pIter->iPgidxOff = pLeaf->szLeaf + fts5GetVarint32(
            &pLeaf->p[pLeaf->szLeaf], iOff
        );
        pIter->iLeafOffset = iOff;
        pIter->iEndofDoclist = iOff;
        bNewTerm = 1;
      }
      assert_nc( iOff<pLeaf->szLeaf );
      if( iOff>=pLeaf->szLeaf ){
        /* If iOff==szLeaf then the first rowid offset in the page header
        ** (or the first term offset) points at the page footer, and
        ** pIter->iLeafOffset still has its value from the previous page. */
        FTS5_CORRUPT_ITER(p, pIter);
        return;
      }
    }
  }

  /* Check if the iterator is now at EOF. If so, return early. */
  if( pIter->pLeaf ){
    if( bNewTerm ){
      if( pIter->flags & FTS5_SEGITER_ONETERM ){
        fts5DataRelease(pIter->pLeaf);
        pIter->pLeaf = 0;
      }else{
        fts5SegIterLoadTerm(p, pIter, nKeep);
        fts5SegIterLoadNPos(p, pIter);
        if( pbNewTerm ) *pbNewTerm = 1;
      }
    }else{
      /* The following could be done by calling fts5SegIterLoadNPos(). But
      ** this block is particularly performance critical, so equivalent
      ** code is inlined.  */
      int nSz;
      assert_nc( pIter->iLeafOffset<=pIter->pLeaf->nn );
      fts5FastGetVarint32(pIter->pLeaf->p, pIter->iLeafOffset, nSz);
      pIter->bDel = (nSz & 0x0001);
      pIter->nPos = nSz>>1;
      assert_nc( pIter->nPos>=0 );
    }
  }
}
/* …（同文件无关代码省略）… */
#define SWAPVAL(T, a, b) { T tmp; tmp=a; a=b; b=tmp; }
/* …（同文件无关代码省略）… */
#define fts5IndexSkipVarint(a, iOff) {            \
  int iEnd = iOff+9;                              \
  while( (a[iOff++] & 0x80) && iOff<iEnd );       \
}
/* …（同文件无关代码省略）… */
static void fts5SegIterReverse(Fts5Index *p, Fts5SegIter *pIter){
  Fts5DlidxIter *pDlidx = pIter->pDlidx;
  Fts5Data *pLast = 0;
  int pgnoLast = 0;

  if( pDlidx && p->pConfig->iVersion==FTS5_CURRENT_VERSION ){
    int iSegid = pIter->pSeg->iSegid;
    pgnoLast = fts5DlidxIterPgno(pDlidx);
    pLast = fts5LeafRead(p, FTS5_SEGMENT_ROWID(iSegid, pgnoLast));
  }else{
    Fts5Data *pLeaf = pIter->pLeaf;         /* Current leaf data */

    /* Currently, Fts5SegIter.iLeafOffset points to the first byte of
    ** position-list content for the current rowid. Back it up so that it
    ** points to the start of the position-list size field. */
    int iPoslist;
    if( pIter->iTermLeafPgno==pIter->iLeafPgno ){
      iPoslist = pIter->iTermLeafOffset;
    }else{
      iPoslist = 4;
    }
    fts5IndexSkipVarint(pLeaf->p, iPoslist);
    pIter->iLeafOffset = iPoslist;

    /* If this condition is true then the largest rowid for the current
    ** term may not be stored on the current page. So search forward to
    ** see where said rowid really is.  */
    if( pIter->iEndofDoclist>=pLeaf->szLeaf ){
      int pgno;
      Fts5StructureSegment *pSeg = pIter->pSeg;

      /* The last rowid in the doclist may not be on the current page. Search
      ** forward to find the page containing the last rowid.  */
      for(pgno=pIter->iLeafPgno+1; !p->rc && pgno<=pSeg->pgnoLast; pgno++){
        i64 iAbs = FTS5_SEGMENT_ROWID(pSeg->iSegid, pgno);
        Fts5Data *pNew = fts5LeafRead(p, iAbs);
        if( pNew ){
          int iRowid, bTermless;
          iRowid = fts5LeafFirstRowidOff(pNew);
          bTermless = fts5LeafIsTermless(pNew);
          if( iRowid ){
            SWAPVAL(Fts5Data*, pNew, pLast);
            pgnoLast = pgno;
          }
          fts5DataRelease(pNew);
          if( bTermless==0 ) break;
        }
      }
    }
  }

  /* If pLast is NULL at this point, then the last rowid for this doclist
  ** lies on the page currently indicated by the iterator. In this case 
  ** pIter->iLeafOffset is already set to point to the position-list size
  ** field associated with the first relevant rowid on the page.
  **
  ** Or, if pLast is non-NULL, then it is the page that contains the last
  ** rowid. In this case configure the iterator so that it points to the
  ** first rowid on this page.
  */
  if( pLast ){
    int iOff;
    fts5DataRelease(pIter->pLeaf);
    pIter->pLeaf = pLast;
    pIter->iLeafPgno = pgnoLast;
    if( p->rc==SQLITE_OK ){
      iOff = fts5LeafFirstRowidOff(pLast);
      if( iOff>pLast->szLeaf ){
        FTS5_CORRUPT_ITER(p, pIter);
        return;
      }
      iOff += fts5GetVarint(&pLast->p[iOff], (u64*)&pIter->iRowid);
      pIter->iLeafOffset = iOff;

      if( fts5LeafIsTermless(pLast) ){
        pIter->iEndofDoclist = pLast->nn+1;
      }else{
        pIter->iEndofDoclist = fts5LeafFirstTermOff(pLast);
      }
    }
  }

  fts5SegIterReverseInitPage(p, pIter);
}
/* …（同文件无关代码省略）… */
static void fts5SegIterLoadDlidx(Fts5Index *p, Fts5SegIter *pIter){
  int iSeg = pIter->pSeg->iSegid;
  int bRev = (pIter->flags & FTS5_SEGITER_REVERSE);
  Fts5Data *pLeaf = pIter->pLeaf; /* Current leaf data */

  assert( pIter->flags & FTS5_SEGITER_ONETERM );
  assert( pIter->pDlidx==0 );

  /* Check if the current doclist ends on this page. If it does, return
  ** early without loading the doclist-index (as it belongs to a different
  ** term. */
  if( pIter->iTermLeafPgno==pIter->iLeafPgno 
   && pIter->iEndofDoclist<pLeaf->szLeaf 
  ){
    return;
  }

  pIter->pDlidx = fts5DlidxIterInit(p, bRev, iSeg, pIter->iTermLeafPgno);
}
/* …（同文件无关代码省略）… */
static void fts5LeafSeek(
  Fts5Index *p,                   /* Leave any error code here */
  int bGe,                        /* True for a >= search */
  Fts5SegIter *pIter,             /* Iterator to seek */
  const u8 *pTerm, int nTerm      /* Term to search for */
){
  u32 iOff;
  const u8 *a = pIter->pLeaf->p;
  u32 n = (u32)pIter->pLeaf->nn;

  u32 nMatch = 0;
  u32 nKeep = 0;
  u32 nNew = 0;
  u32 iTermOff;
  u32 iPgidx;                     /* Current offset in pgidx */
  int bEndOfPage = 0;

  assert( p->rc==SQLITE_OK );

  iPgidx = (u32)pIter->pLeaf->szLeaf;
  iPgidx += fts5GetVarint32(&a[iPgidx], iTermOff);
  iOff = iTermOff;
  if( iOff>n ){
    FTS5_CORRUPT_ITER(p, pIter);
    return;
  }

  while( 1 ){

    /* Figure out how many new bytes are in this term */
    fts5FastGetVarint32(a, iOff, nNew);
    if( nKeep<nMatch ){
      goto search_failed;
    }
    if( (iOff+nNew)>n ){
      FTS5_CORRUPT_ITER(p, pIter);
      return;
    }

    assert( nKeep>=nMatch );
    if( nKeep==nMatch ){
      u32 nCmp;
      u32 i;
      nCmp = (u32)MIN(nNew, nTerm-nMatch);
      for(i=0; i<nCmp; i++){
        if( a[iOff+i]!=pTerm[nMatch+i] ) break;
      }
      nMatch += i;

      if( (u32)nTerm==nMatch ){
        if( i==nNew ){
          goto search_success;
        }else{
          goto search_failed;
        }
      }else if( i<nNew && a[iOff+i]>pTerm[nMatch] ){
        goto search_failed;
      }
    }

    if( iPgidx>=n ){
      bEndOfPage = 1;
      break;
    }

    iPgidx += fts5GetVarint32(&a[iPgidx], nKeep);
    iTermOff += nKeep;
    iOff = iTermOff;

    if( iOff>=n ){
      FTS5_CORRUPT_ITER(p, pIter);
      return;
    }

    /* Read the nKeep field of the next term. */
    fts5FastGetVarint32(a, iOff, nKeep);
  }

 search_failed:
  if( bGe==0 ){
    fts5DataRelease(pIter->pLeaf);
    pIter->pLeaf = 0;
    return;
  }else if( bEndOfPage ){
    do {
      fts5SegIterNextPage(p, pIter);
      if( pIter->pLeaf==0 ) return;
      a = pIter->pLeaf->p;
      if( fts5LeafIsTermless(pIter->pLeaf)==0 ){
        iPgidx = (u32)pIter->pLeaf->szLeaf;
        iPgidx += fts5GetVarint32(&pIter->pLeaf->p[iPgidx], iOff);
        if( iOff<4 || (i64)iOff>=pIter->pLeaf->szLeaf ){
          FTS5_CORRUPT_ITER(p, pIter);
          return;
        }else{
          nKeep = 0;
          iTermOff = iOff;
          n = (u32)pIter->pLeaf->nn;
          iOff += fts5GetVarint32(&a[iOff], nNew);
          break;
        }
      }
    }while( 1 );
  }

 search_success:
  if( (i64)iOff+nNew>n || nNew<1 ){
    FTS5_CORRUPT_ITER(p, pIter);
    return;
  }
  pIter->iLeafOffset = iOff + nNew;
  pIter->iTermLeafOffset = pIter->iLeafOffset;
  pIter->iTermLeafPgno = pIter->iLeafPgno;

  fts5BufferSet(&p->rc, &pIter->term, nKeep, pTerm);
  fts5BufferAppendBlob(&p->rc, &pIter->term, nNew, &a[iOff]);

  if( iPgidx>=n ){
    pIter->iEndofDoclist = pIter->pLeaf->nn+1;
  }else{
    int nExtra;
    iPgidx += fts5GetVarint32(&a[iPgidx], nExtra);
    pIter->iEndofDoclist = iTermOff + nExtra;
  }
  pIter->iPgidxOff = iPgidx;

  fts5SegIterLoadRowid(p, pIter);
  fts5SegIterLoadNPos(p, pIter);
}
/* …（同文件无关代码省略）… */
static sqlite3_stmt *fts5IdxSelectStmt(Fts5Index *p){
  if( p->pIdxSelect==0 ){
    Fts5Config *pConfig = p->pConfig;
    fts5IndexPrepareStmt(p, &p->pIdxSelect, sqlite3_mprintf(
          "SELECT pgno FROM '%q'.'%q_idx' WHERE "
          "segid=? AND term<=? ORDER BY term DESC LIMIT 1",
          pConfig->zDb, pConfig->zName
    ));
  }
  return p->pIdxSelect;
}
/* …（同文件无关代码省略）… */
static void fts5SegIterSeekInit(
  Fts5Index *p,                   /* FTS5 backend */
  const u8 *pTerm, int nTerm,     /* Term to seek to */
  int flags,                      /* Mask of FTS5INDEX_XXX flags */
  Fts5StructureSegment *pSeg,     /* Description of segment */
  Fts5SegIter *pIter              /* Object to populate */
){
  int iPg = 1;
  int bGe = (flags & FTS5INDEX_QUERY_SCAN);
  int bDlidx = 0;                 /* True if there is a doclist-index */
  sqlite3_stmt *pIdxSelect = 0;

  assert( bGe==0 || (flags & FTS5INDEX_QUERY_DESC)==0 );
  assert( pTerm && nTerm );
  memset(pIter, 0, sizeof(*pIter));
  pIter->pSeg = pSeg;

  /* This block sets stack variable iPg to the leaf page number that may
  ** contain term (pTerm/nTerm), if it is present in the segment. */
  pIdxSelect = fts5IdxSelectStmt(p);
  if( p->rc ) return;
  sqlite3_bind_int(pIdxSelect, 1, pSeg->iSegid);
  p->rc = sqlite3_bind_blob(pIdxSelect, 2, pTerm, nTerm, SQLITE_STATIC);
  if( p->rc ) return;
  if( SQLITE_ROW==sqlite3_step(pIdxSelect) ){
    i64 val = sqlite3_column_int(pIdxSelect, 0);
    iPg = (int)(val>>1);
    bDlidx = (val & 0x0001);
  }
  p->rc = sqlite3_reset(pIdxSelect);
  sqlite3_bind_null(pIdxSelect, 2);

  if( iPg<pSeg->pgnoFirst ){
    iPg = pSeg->pgnoFirst;
    bDlidx = 0;
  }

  pIter->iLeafPgno = iPg - 1;
  fts5SegIterNextPage(p, pIter);

  if( pIter->pLeaf ){
    fts5LeafSeek(p, bGe, pIter, pTerm, nTerm);
  }

  if( p->rc==SQLITE_OK && (bGe==0 || (flags & FTS5INDEX_QUERY_SCANONETERM)) ){
    pIter->flags |= FTS5_SEGITER_ONETERM;
    if( pIter->pLeaf ){
      if( flags & FTS5INDEX_QUERY_DESC ){
        pIter->flags |= FTS5_SEGITER_REVERSE;
      }
      if( bDlidx && iPg==pIter->iTermLeafPgno ){
        fts5SegIterLoadDlidx(p, pIter);
      }
      if( flags & FTS5INDEX_QUERY_DESC ){
        fts5SegIterReverse(p, pIter);
      }
    }
  }

  fts5SegIterSetNext(p, pIter);
  if( 0==(flags & FTS5INDEX_QUERY_SCANONETERM) ){
    fts5SegIterAllocTombstone(p, pIter);
  }

  /* Either:
  **
  **   1) an error has occurred, or
  **   2) the iterator points to EOF, or
  **   3) the iterator points to an entry with term (pTerm/nTerm), or
  **   4) the FTS5INDEX_QUERY_SCAN flag was set and the iterator points
  **      to an entry with a term greater than or equal to (pTerm/nTerm).
  */
  assert_nc( p->rc!=SQLITE_OK                                       /* 1 */
   || pIter->pLeaf==0                                               /* 2 */
   || fts5BufferCompareBlob(&pIter->term, pTerm, nTerm)==0          /* 3 */
   || (bGe && fts5BufferCompareBlob(&pIter->term, pTerm, nTerm)>0)  /* 4 */
  );
}
/* …（同文件无关代码省略）… */
static void fts5SegIterHashInit(
  Fts5Index *p,                   /* FTS5 backend */
  const u8 *pTerm, int nTerm,     /* Term to seek to */
  int flags,                      /* Mask of FTS5INDEX_XXX flags */
  Fts5SegIter *pIter              /* Object to populate */
){
  int nList = 0;
  const u8 *z = 0;
  int n = 0;
  Fts5Data *pLeaf = 0;

  assert( p->pHash );
  assert( p->rc==SQLITE_OK );

  if( pTerm==0 || (flags & FTS5INDEX_QUERY_SCAN) ){
    const u8 *pList = 0;

    p->rc = sqlite3Fts5HashScanInit(p->pHash, (const char*)pTerm, nTerm);
    sqlite3Fts5HashScanEntry(p->pHash, (const char**)&z, &n, &pList, &nList);
    if( pList ){
      pLeaf = fts5IdxMalloc(p, sizeof(Fts5Data));
      if( pLeaf ){
        pLeaf->p = (u8*)pList;
      }
    }

    /* The call to sqlite3Fts5HashScanInit() causes the hash table to
    ** fill the size field of all existing position lists. This means they
    ** can no longer be appended to. Since the only scenario in which they
    ** can be appended to is if the previous operation on this table was
    ** a DELETE, by clearing the Fts5Index.bDelete flag we can avoid this
    ** possibility altogether.  */
    p->bDelete = 0;
  }else{
    p->rc = sqlite3Fts5HashQuery(p->pHash, sizeof(Fts5Data), 
        (const char*)pTerm, nTerm, (void**)&pLeaf, &nList
    );
    if( pLeaf ){
      pLeaf->p = (u8*)&pLeaf[1];
    }
    z = pTerm;
    n = nTerm;
    pIter->flags |= FTS5_SEGITER_ONETERM;
  }

  if( pLeaf ){
    sqlite3Fts5BufferSet(&p->rc, &pIter->term, n, z);
    pLeaf->nn = pLeaf->szLeaf = nList;
    pIter->pLeaf = pLeaf;
    pIter->iLeafOffset = fts5GetVarint(pLeaf->p, (u64*)&pIter->iRowid);
    pIter->iEndofDoclist = pLeaf->nn;

    if( flags & FTS5INDEX_QUERY_DESC ){
      pIter->flags |= FTS5_SEGITER_REVERSE;
      fts5SegIterReverseInitPage(p, pIter);
    }else{
      fts5SegIterLoadNPos(p, pIter);
    }
  }

  fts5SegIterSetNext(p, pIter);
}
/* …（同文件无关代码省略）… */
static void fts5TombstoneArrayDelete(Fts5TombstoneArray *p){
  if( p ){
    p->nRef--;
    if( p->nRef<=0 ){
      int ii;
      for(ii=0; ii<p->nTombstone; ii++){
        fts5DataRelease(p->apTombstone[ii]);
      }
      sqlite3_free(p);
    }
  }
}
/* …（同文件无关代码省略）… */
static void fts5SegIterClear(Fts5SegIter *pIter){
  fts5BufferFree(&pIter->term);
  fts5DataRelease(pIter->pLeaf);
  fts5DataRelease(pIter->pNextLeaf);
  fts5TombstoneArrayDelete(pIter->pTombArray);
  fts5DlidxIterFree(pIter->pDlidx);
  sqlite3_free(pIter->aRowidOffset);
  memset(pIter, 0, sizeof(Fts5SegIter));
}
/* …（同文件无关代码省略）… */
# define fts5AssertMultiIterSetup(x,y)
/* …（同文件无关代码省略）… */
static int fts5MultiIterDoCompare(Fts5Iter *pIter, int iOut){
  int i1;                         /* Index of left-hand Fts5SegIter */
  int i2;                         /* Index of right-hand Fts5SegIter */
  int iRes;
  Fts5SegIter *p1;                /* Left-hand Fts5SegIter */
  Fts5SegIter *p2;                /* Right-hand Fts5SegIter */
  Fts5CResult *pRes = &pIter->aFirst[iOut];

  assert( iOut<pIter->nSeg && iOut>0 );
  assert( pIter->bRev==0 || pIter->bRev==1 );

  if( iOut>=(pIter->nSeg/2) ){
    i1 = (iOut - pIter->nSeg/2) * 2;
    i2 = i1 + 1;
  }else{
    i1 = pIter->aFirst[iOut*2].iFirst;
    i2 = pIter->aFirst[iOut*2+1].iFirst;
  }
  p1 = &pIter->aSeg[i1];
  p2 = &pIter->aSeg[i2];

  pRes->bTermEq = 0;
  if( p1->pLeaf==0 ){           /* If p1 is at EOF */
    iRes = i2;
  }else if( p2->pLeaf==0 ){     /* If p2 is at EOF */
    iRes = i1;
  }else{
    int res = fts5BufferCompare(&p1->term, &p2->term);
    if( res==0 ){
      assert_nc( i2>i1 );
      assert_nc( i2!=0 );
      pRes->bTermEq = 1;
      if( p1->iRowid==p2->iRowid ){
        /* p2 is about to be advanced past this entry. If it, or an entry
        ** it shadowed, has the delete flag set, record this on p1.  */
        p1->bDelDup |= (p2->bDel | p2->bDelDup);
        p2->bDelDup = 0;
        return i2;
      }
      res = ((p1->iRowid > p2->iRowid)==pIter->bRev) ? -1 : +1;
    }
    assert( res!=0 );
    if( res<0 ){
      iRes = i1;
    }else{
      iRes = i2;
    }
  }

  pRes->iFirst = (u16)iRes;
  return 0;
}
/* …（同文件无关代码省略）… */
static void fts5SegIterGotoPage(
  Fts5Index *p,                   /* FTS5 backend object */
  Fts5SegIter *pIter,             /* Iterator to advance */
  int iLeafPgno
){
  assert( iLeafPgno>pIter->iLeafPgno );

  if( iLeafPgno>pIter->pSeg->pgnoLast ){
    FTS5_CORRUPT_IDX(p);
  }else{
    fts5DataRelease(pIter->pNextLeaf);
    pIter->pNextLeaf = 0;
    pIter->iLeafPgno = iLeafPgno-1;

    while( p->rc==SQLITE_OK ){
      int iOff;
      fts5SegIterNextPage(p, pIter);
      if( pIter->pLeaf==0 ) break;
      iOff = fts5LeafFirstRowidOff(pIter->pLeaf);
      if( iOff>0 ){
        u8 *a = pIter->pLeaf->p;
        int n = pIter->pLeaf->szLeaf;
        if( iOff<4 || iOff>=n ){
          FTS5_CORRUPT_IDX(p);
        }else{
          iOff += fts5GetVarint(&a[iOff], (u64*)&pIter->iRowid);
          pIter->iLeafOffset = iOff;
          fts5SegIterLoadNPos(p, pIter);
        }
        break;
      }else if( !fts5LeafIsTermless(pIter->pLeaf) ){
        /* There are no rowids on this page, but there is another term.
        ** Since there are no more rowids for the current term, set the
        ** iterator to EOF.  */
        fts5DataRelease(pIter->pLeaf);
        pIter->pLeaf = 0;
        break;
      }
    }
  }
}
/* …（同文件无关代码省略）… */
static void fts5SegIterNextFrom(
  Fts5Index *p,                   /* FTS5 backend object */
  Fts5SegIter *pIter,             /* Iterator to advance */
  i64 iMatch                      /* Advance iterator at least this far */
){
  int bRev = (pIter->flags & FTS5_SEGITER_REVERSE);
  Fts5DlidxIter *pDlidx = pIter->pDlidx;
  int iLeafPgno = pIter->iLeafPgno;
  int bMove = 1;

  assert( pIter->flags & FTS5_SEGITER_ONETERM );
  assert( pIter->pDlidx );
  assert( pIter->pLeaf );

  if( bRev==0 ){
    while( !fts5DlidxIterEof(p, pDlidx) && iMatch>fts5DlidxIterRowid(pDlidx) ){
      iLeafPgno = fts5DlidxIterPgno(pDlidx);
      fts5DlidxIterNext(p, pDlidx);
    }
    assert_nc( iLeafPgno>=pIter->iLeafPgno || p->rc );
    if( iLeafPgno>pIter->iLeafPgno ){
      fts5SegIterGotoPage(p, pIter, iLeafPgno);
      bMove = 0;
    }
  }else{
    assert( pIter->pNextLeaf==0 );
    assert( iMatch<pIter->iRowid );
    while( !fts5DlidxIterEof(p, pDlidx) && iMatch<fts5DlidxIterRowid(pDlidx) ){
      fts5DlidxIterPrev(p, pDlidx);
    }
    iLeafPgno = fts5DlidxIterPgno(pDlidx);

    assert( fts5DlidxIterEof(p, pDlidx) || iLeafPgno<=pIter->iLeafPgno );

    if( iLeafPgno<pIter->iLeafPgno ){
      pIter->iLeafPgno = iLeafPgno+1;
      fts5SegIterReverseNewPage(p, pIter);
      bMove = 0;
    }
  }

  do{
    if( bMove && p->rc==SQLITE_OK ) pIter->xNext(p, pIter, 0);
    if( pIter->pLeaf==0 ) break;
    if( bRev==0 && pIter->iRowid>=iMatch ) break;
    if( bRev!=0 && pIter->iRowid<=iMatch ) break;
    bMove = 1;
  }while( p->rc==SQLITE_OK );
}
/* …（同文件无关代码省略）… */
static void fts5MultiIterFree(Fts5Iter *pIter){
  if( pIter ){
    int i;
    for(i=0; i<pIter->nSeg; i++){
      fts5SegIterClear(&pIter->aSeg[i]);
    }
    fts5BufferFree(&pIter->poslist);
    sqlite3_free(pIter);
  }
}
/* …（同文件无关代码省略）… */
static void fts5MultiIterAdvanced(
  Fts5Index *p,                   /* FTS5 backend to iterate within */
  Fts5Iter *pIter,                /* Iterator to update aFirst[] array for */
  int iChanged,                   /* Index of sub-iterator just advanced */
  int iMinset                     /* Minimum entry in aFirst[] to set */
){
  int i;
  for(i=(pIter->nSeg+iChanged)/2; i>=iMinset && p->rc==SQLITE_OK; i=i/2){
    int iEq;
    if( (iEq = fts5MultiIterDoCompare(pIter, i)) ){
      Fts5SegIter *pSeg = &pIter->aSeg[iEq];
      assert( p->rc==SQLITE_OK );
      pSeg->xNext(p, pSeg, 0);
      i = pIter->nSeg + iEq;
    }
  }
}
/* …（同文件无关代码省略）… */
static int fts5MultiIterAdvanceRowid(
  Fts5Iter *pIter,                /* Iterator to update aFirst[] array for */
  int iChanged,                   /* Index of sub-iterator just advanced */
  Fts5SegIter **ppFirst
){
  Fts5SegIter *pNew = &pIter->aSeg[iChanged];

  if( pNew->iRowid==pIter->iSwitchRowid
   || (pNew->iRowid<pIter->iSwitchRowid)==pIter->bRev
  ){
    int i;
    Fts5SegIter *pOther = &pIter->aSeg[iChanged ^ 0x0001];
    pIter->iSwitchRowid = pIter->bRev ? SMALLEST_INT64 : LARGEST_INT64;
    for(i=(pIter->nSeg+iChanged)/2; 1; i=i/2){
      Fts5CResult *pRes = &pIter->aFirst[i];

      assert( pNew->pLeaf );
      assert( pRes->bTermEq==0 || pOther->pLeaf );

      if( pRes->bTermEq ){
        if( pNew->iRowid==pOther->iRowid ){
          return 1;
        }else if( (pOther->iRowid>pNew->iRowid)==pIter->bRev ){
          pIter->iSwitchRowid = pOther->iRowid;
          pNew = pOther;
        }else if( (pOther->iRowid>pIter->iSwitchRowid)==pIter->bRev ){
          pIter->iSwitchRowid = pOther->iRowid;
        }
      }
      pRes->iFirst = (u16)(pNew - pIter->aSeg);
      if( i==1 ) break;

      pOther = &pIter->aSeg[ pIter->aFirst[i ^ 0x0001].iFirst ];
    }
  }

  *ppFirst = pNew;
  return 0;
}
/* …（同文件无关代码省略）… */
static void fts5MultiIterSetEof(Fts5Iter *pIter){
  Fts5SegIter *pSeg = &pIter->aSeg[ pIter->aFirst[1].iFirst ];
  pIter->base.bEof = pSeg->pLeaf==0;
  pIter->iSwitchRowid = pSeg->iRowid;
}
/* …（同文件无关代码省略）… */
static int fts5IndexTombstoneQuery(
  Fts5Data *pHash,                /* Hash table page to query */
  int nHashTable,                 /* Number of pages attached to segment */
  u64 iRowid                      /* Rowid to query hash for */
){
  const int szKey = TOMBSTONE_KEYSIZE(pHash);
  const int nSlot = TOMBSTONE_NSLOT(pHash);
  int iSlot = (iRowid / nHashTable) % nSlot;
  int nCollide = nSlot;

  if( iRowid==0 ){
    return pHash->p[1];
  }else if( szKey==4 ){
    u32 *aSlot = (u32*)&pHash->p[8];
    while( aSlot[iSlot] ){
      if( fts5GetU32((u8*)&aSlot[iSlot])==iRowid ) return 1;
      if( nCollide--==0 ) break;
      iSlot = (iSlot+1)%nSlot;
    }
  }else{
    u64 *aSlot = (u64*)&pHash->p[8];
    while( aSlot[iSlot] ){
      if( fts5GetU64((u8*)&aSlot[iSlot])==iRowid ) return 1;
      if( nCollide--==0 ) break;
      iSlot = (iSlot+1)%nSlot;
    }
  }

  return 0;
}
/* …（同文件无关代码省略）… */
static int fts5MultiIterIsDeleted(Fts5Iter *pIter){
  int iFirst = pIter->aFirst[1].iFirst;
  Fts5SegIter *pSeg = &pIter->aSeg[iFirst];
  Fts5TombstoneArray *pArray = pSeg->pTombArray;

  if( pSeg->pLeaf && pArray ){
    /* Figure out which page the rowid might be present on. */
    int iPg = ((u64)pSeg->iRowid) % pArray->nTombstone;
    assert( iPg>=0 );

    /* If tombstone hash page iPg has not yet been loaded from the 
    ** database, load it now. */
    if( pArray->apTombstone[iPg]==0 ){
      pArray->apTombstone[iPg] = fts5DataRead(pIter->pIndex,
          FTS5_TOMBSTONE_ROWID(pSeg->pSeg->iSegid, iPg)
      );
      if( pArray->apTombstone[iPg]==0 ) return 0;
    }

    return fts5IndexTombstoneQuery(
        pArray->apTombstone[iPg],
        pArray->nTombstone,
        pSeg->iRowid
    );
  }

  return 0;
}
/* …（同文件无关代码省略）… */
static void fts5MultiIterNext(
  Fts5Index *p, 
  Fts5Iter *pIter,
  int bFrom,                      /* True if argument iFrom is valid */
  i64 iFrom                       /* Advance at least as far as this */
){
  int bUseFrom = bFrom;
  assert( pIter->base.bEof==0 );
  while( p->rc==SQLITE_OK ){
    int iFirst = pIter->aFirst[1].iFirst;
    int bNewTerm = 0;
    Fts5SegIter *pSeg = &pIter->aSeg[iFirst];
    assert( p->rc==SQLITE_OK );
    if( bUseFrom && pSeg->pDlidx ){
      fts5SegIterNextFrom(p, pSeg, iFrom);
    }else{
      pSeg->xNext(p, pSeg, &bNewTerm);
    }

    if( pSeg->pLeaf==0 || bNewTerm 
     || fts5MultiIterAdvanceRowid(pIter, iFirst, &pSeg)
    ){
      fts5MultiIterAdvanced(p, pIter, iFirst, 1);
      fts5MultiIterSetEof(pIter);
      pSeg = &pIter->aSeg[pIter->aFirst[1].iFirst];
      if( pSeg->pLeaf==0 ) return;
    }

    fts5AssertMultiIterSetup(p, pIter);
    assert( pSeg==&pIter->aSeg[pIter->aFirst[1].iFirst] && pSeg->pLeaf );
    if( (pIter->bSkipEmpty==0 || pSeg->nPos) 
      && 0==fts5MultiIterIsDeleted(pIter)
    ){
      pIter->xSetOutputs(pIter, pSeg);
      return;
    }
    bUseFrom = 0;
  }
}
/* …（同文件无关代码省略）… */
static void fts5IterSetOutputs_Noop(Fts5Iter *pUnused1, Fts5SegIter *pUnused2){
  UNUSED_PARAM2(pUnused1, pUnused2);
}
/* …（同文件无关代码省略）… */
static Fts5Iter *fts5MultiIterAlloc(
  Fts5Index *p,                   /* FTS5 backend to iterate within */
  int nSeg
){
  Fts5Iter *pNew;
  i64 nSlot;                      /* Power of two >= nSeg */

  for(nSlot=2; nSlot<nSeg; nSlot=nSlot*2);
  pNew = fts5IdxMalloc(p, 
      SZ_FTS5ITER(nSlot) +                /* pNew + pNew->aSeg[] */
      sizeof(Fts5CResult) * nSlot         /* pNew->aFirst[] */
  );
  if( pNew ){
    pNew->nSeg = nSlot;
    pNew->aFirst = (Fts5CResult*)&pNew->aSeg[nSlot];
    pNew->pIndex = p;
    pNew->xSetOutputs = fts5IterSetOutputs_Noop;
  }
  return pNew;
}
/* …（同文件无关代码省略）… */
static void fts5SegiterPoslist(
  Fts5Index *p,
  Fts5SegIter *pSeg,
  Fts5Colset *pColset,
  Fts5Buffer *pBuf
){
  assert( pBuf!=0 );
  assert( pSeg!=0 );
  if( 0==fts5BufferGrow(&p->rc, pBuf, pSeg->nPos+FTS5_DATA_ZERO_PADDING) ){
    assert( pBuf->p!=0 );
    assert( pBuf->nSpace >= pBuf->n+pSeg->nPos+FTS5_DATA_ZERO_PADDING );
    memset(&pBuf->p[pBuf->n+pSeg->nPos], 0, FTS5_DATA_ZERO_PADDING);
    if( pColset==0 ){
      fts5ChunkIterate(p, pSeg, (void*)pBuf, fts5PoslistCallback);
    }else{
      if( p->pConfig->eDetail==FTS5_DETAIL_FULL ){
        PoslistCallbackCtx sCtx;
        sCtx.pBuf = pBuf;
        sCtx.pColset = pColset;
        sCtx.eState = fts5IndexColsetTest(pColset, 0);
        assert( sCtx.eState==0 || sCtx.eState==1 );
        fts5ChunkIterate(p, pSeg, (void*)&sCtx, fts5PoslistFilterCallback);
      }else{
        PoslistOffsetsCtx sCtx;
        memset(&sCtx, 0, sizeof(sCtx));
        sCtx.pBuf = pBuf;
        sCtx.pColset = pColset;
        fts5ChunkIterate(p, pSeg, (void*)&sCtx, fts5PoslistOffsetsCallback);
      }
    }
  }
}
/* …（同文件无关代码省略）… */
static void fts5IndexExtractColset(
  int *pRc,
  Fts5Colset *pColset,            /* Colset to filter on */
  const u8 *pPos, int nPos,       /* Position list */
  Fts5Iter *pIter
){
  if( *pRc==SQLITE_OK ){
    const u8 *p = pPos;
    const u8 *aCopy = p;
    const u8 *pEnd = &p[nPos];    /* One byte past end of position list */
    int i = 0;
    int iCurrent = 0;

    if( pColset->nCol>1 && sqlite3Fts5BufferSize(pRc, &pIter->poslist, nPos) ){
      return;
    }

    while( 1 ){
      while( pColset->aiCol[i]<iCurrent ){
        i++;
        if( i==pColset->nCol ){
          pIter->base.pData = pIter->poslist.p;
          pIter->base.nData = pIter->poslist.n;
          return;
        }
      }

      /* Advance pointer p until it points to pEnd or an 0x01 byte that is
      ** not part of a varint */
      while( p<pEnd && *p!=0x01 ){
        while( p<pEnd && (*p++ & 0x80) );
      }

      if( pColset->aiCol[i]==iCurrent ){
        if( pColset->nCol==1 ){
          pIter->base.pData = aCopy;
          pIter->base.nData = p-aCopy;
          return;
        }
        fts5BufferSafeAppendBlob(&pIter->poslist, aCopy, p-aCopy);
      }
      if( p>=pEnd ){
        pIter->base.pData = pIter->poslist.p;
        pIter->base.nData = pIter->poslist.n;
        return;
      }
      aCopy = p++;
      iCurrent = *p++;
      if( iCurrent & 0x80 ){
        p--;
        p += fts5GetVarint32(p, iCurrent);
      }
    }
  }

}
/* …（同文件无关代码省略）… */
static void fts5IterSetOutputs_None(Fts5Iter *pIter, Fts5SegIter *pSeg){
  assert( pIter->pIndex->pConfig->eDetail==FTS5_DETAIL_NONE );
  pIter->base.iRowid = pSeg->iRowid;
  pIter->base.nData = pSeg->nPos;
}
/* …（同文件无关代码省略）… */
static void fts5IterSetOutputs_Nocolset(Fts5Iter *pIter, Fts5SegIter *pSeg){
  pIter->base.iRowid = pSeg->iRowid;
  pIter->base.nData = pSeg->nPos;

  assert( pIter->pIndex->pConfig->eDetail!=FTS5_DETAIL_NONE );
  assert( pIter->pColset==0 );

  if( pSeg->iLeafOffset+pSeg->nPos<=pSeg->pLeaf->szLeaf ){
    /* All data is stored on the current page. Populate the output 
    ** variables to point into the body of the page object. */
    pIter->base.pData = &pSeg->pLeaf->p[pSeg->iLeafOffset];
  }else{
    /* The data is distributed over two or more pages. Copy it into the
    ** Fts5Iter.poslist buffer and then set the output pointer to point
    ** to this buffer.  */
    fts5BufferZero(&pIter->poslist);
    fts5SegiterPoslist(pIter->pIndex, pSeg, 0, &pIter->poslist);
    pIter->base.pData = pIter->poslist.p;
  }
}
/* …（同文件无关代码省略）… */
static void fts5IterSetOutputs_ZeroColset(Fts5Iter *pIter, Fts5SegIter *pSeg){
  UNUSED_PARAM(pSeg);
  pIter->base.nData = 0;
  pIter->base.bEof = 1;
}
/* …（同文件无关代码省略）… */
static void fts5IterSetOutputs_Col(Fts5Iter *pIter, Fts5SegIter *pSeg){
  fts5BufferZero(&pIter->poslist);
  fts5SegiterPoslist(pIter->pIndex, pSeg, pIter->pColset, &pIter->poslist);
  pIter->base.iRowid = pSeg->iRowid;
  pIter->base.pData = pIter->poslist.p;
  pIter->base.nData = pIter->poslist.n;
}
/* …（同文件无关代码省略）… */
static void fts5IterSetOutputs_Col100(Fts5Iter *pIter, Fts5SegIter *pSeg){

  assert( pIter->pIndex->pConfig->eDetail==FTS5_DETAIL_COLUMNS );
  assert( pIter->pColset );
  assert( pIter->poslist.nSpace>=pIter->pIndex->pConfig->nCol );

  if( pSeg->iLeafOffset+pSeg->nPos>pSeg->pLeaf->szLeaf 
   || pSeg->nPos>pIter->pIndex->pConfig->nCol
  ){
    fts5IterSetOutputs_Col(pIter, pSeg);
  }else{
    u8 *a = (u8*)&pSeg->pLeaf->p[pSeg->iLeafOffset];
    u8 *pEnd = (u8*)&a[pSeg->nPos]; 
    int iPrev = 0;
    int *aiCol = pIter->pColset->aiCol;
    int *aiColEnd = &aiCol[pIter->pColset->nCol];

    u8 *aOut = pIter->poslist.p;
    int iPrevOut = 0;

    pIter->base.iRowid = pSeg->iRowid;

    while( a<pEnd ){
      iPrev += (int)a++[0] - 2;
      while( *aiCol<iPrev ){
        aiCol++;
        if( aiCol==aiColEnd ) goto setoutputs_col_out;
      }
      if( *aiCol==iPrev ){
        *aOut++ = (u8)((iPrev - iPrevOut) + 2);
        iPrevOut = iPrev;
      }
    }

setoutputs_col_out:
    pIter->base.pData = pIter->poslist.p;
    pIter->base.nData = aOut - pIter->poslist.p;
  }
}
/* …（同文件无关代码省略）… */
static void fts5IterSetOutputs_Full(Fts5Iter *pIter, Fts5SegIter *pSeg){
  Fts5Colset *pColset = pIter->pColset;
  pIter->base.iRowid = pSeg->iRowid;

  assert( pIter->pIndex->pConfig->eDetail==FTS5_DETAIL_FULL );
  assert( pColset );

  if( pSeg->iLeafOffset+pSeg->nPos<=pSeg->pLeaf->szLeaf ){
    /* All data is stored on the current page. Populate the output 
    ** variables to point into the body of the page object. */
    const u8 *a = &pSeg->pLeaf->p[pSeg->iLeafOffset];
    int *pRc = &pIter->pIndex->rc;
    fts5BufferZero(&pIter->poslist);
    fts5IndexExtractColset(pRc, pColset, a, pSeg->nPos, pIter);
  }else{
    /* The data is distributed over two or more pages. Copy it into the
    ** Fts5Iter.poslist buffer and then set the output pointer to point
    ** to this buffer.  */
    fts5BufferZero(&pIter->poslist);
    fts5SegiterPoslist(pIter->pIndex, pSeg, pColset, &pIter->poslist);
    pIter->base.pData = pIter->poslist.p;
    pIter->base.nData = pIter->poslist.n;
  }
}
/* …（同文件无关代码省略）… */
static void fts5IterSetOutputCb(int *pRc, Fts5Iter *pIter){
  assert( pIter!=0 || (*pRc)!=SQLITE_OK );
  if( *pRc==SQLITE_OK ){
    Fts5Config *pConfig = pIter->pIndex->pConfig;
    if( pConfig->eDetail==FTS5_DETAIL_NONE ){
      pIter->xSetOutputs = fts5IterSetOutputs_None;
    }

    else if( pIter->pColset==0 ){
      pIter->xSetOutputs = fts5IterSetOutputs_Nocolset;
    }

    else if( pIter->pColset->nCol==0 ){
      pIter->xSetOutputs = fts5IterSetOutputs_ZeroColset;
    }

    else if( pConfig->eDetail==FTS5_DETAIL_FULL ){
      pIter->xSetOutputs = fts5IterSetOutputs_Full;
    }

    else{
      assert( pConfig->eDetail==FTS5_DETAIL_COLUMNS );
      if( pConfig->nCol<=100 ){
        pIter->xSetOutputs = fts5IterSetOutputs_Col100;
        sqlite3Fts5BufferSize(pRc, &pIter->poslist, pConfig->nCol);
      }else{
        pIter->xSetOutputs = fts5IterSetOutputs_Col;
      }
    }
  }
}
/* …（同文件无关代码省略）… */
static void fts5MultiIterFinishSetup(Fts5Index *p, Fts5Iter *pIter){
  int iIter;
  for(iIter=pIter->nSeg-1; iIter>0; iIter--){
    int iEq;
    if( (iEq = fts5MultiIterDoCompare(pIter, iIter)) ){
      Fts5SegIter *pSeg = &pIter->aSeg[iEq];
      if( p->rc==SQLITE_OK ) pSeg->xNext(p, pSeg, 0);
      fts5MultiIterAdvanced(p, pIter, iEq, iIter);
    }
  }
  fts5MultiIterSetEof(pIter);
  fts5AssertMultiIterSetup(p, pIter);

  if( (pIter->bSkipEmpty && fts5MultiIterIsEmpty(p, pIter))
   || fts5MultiIterIsDeleted(pIter)
  ){
    fts5MultiIterNext(p, pIter, 0, 0);
  }else if( pIter->base.bEof==0 ){
    Fts5SegIter *pSeg = &pIter->aSeg[pIter->aFirst[1].iFirst];
    pIter->xSetOutputs(pIter, pSeg);
  }
}
/* …（同文件无关代码省略）… */
static void fts5MultiIterNew(
  Fts5Index *p,                   /* FTS5 backend to iterate within */
  Fts5Structure *pStruct,         /* Structure of specific index */
  int flags,                      /* FTS5INDEX_QUERY_XXX flags */
  Fts5Colset *pColset,            /* Colset to filter on (or NULL) */
  const u8 *pTerm, int nTerm,     /* Term to seek to (or NULL/0) */
  int iLevel,                     /* Level to iterate (-1 for all) */
  int nSegment,                   /* Number of segments to merge (iLevel>=0) */
  Fts5Iter **ppOut                /* New object */
){
  int nSeg = 0;                   /* Number of segment-iters in use */
  int iIter = 0;                  /* */
  int iSeg;                       /* Used to iterate through segments */
  Fts5StructureLevel *pLvl;
  Fts5Iter *pNew;

  assert( (pTerm==0 && nTerm==0) || iLevel<0 );

  /* Allocate space for the new multi-seg-iterator. */
  if( p->rc==SQLITE_OK ){
    if( iLevel<0 ){
      assert( pStruct->nSegment==fts5StructureCountSegments(pStruct) );
      nSeg = pStruct->nSegment;
      nSeg += (p->pHash && 0==(flags & FTS5INDEX_QUERY_SKIPHASH));
    }else{
      nSeg = MIN(pStruct->aLevel[iLevel].nSeg, nSegment);
    }
  }
  *ppOut = pNew = fts5MultiIterAlloc(p, nSeg);
  if( pNew==0 ){
    assert( p->rc!=SQLITE_OK );
    goto fts5MultiIterNew_post_check;
  }
  pNew->bRev = (0!=(flags & FTS5INDEX_QUERY_DESC));
  pNew->bSkipEmpty = (0!=(flags & FTS5INDEX_QUERY_SKIPEMPTY));
  pNew->pColset = pColset;
  if( (flags & FTS5INDEX_QUERY_NOOUTPUT)==0 ){
    fts5IterSetOutputCb(&p->rc, pNew);
  }

  /* Initialize each of the component segment iterators. */
  if( p->rc==SQLITE_OK ){
    if( iLevel<0 ){
      Fts5StructureLevel *pEnd = &pStruct->aLevel[pStruct->nLevel];
      if( p->pHash && 0==(flags & FTS5INDEX_QUERY_SKIPHASH) ){
        /* Add a segment iterator for the current contents of the hash table. */
        Fts5SegIter *pIter = &pNew->aSeg[iIter++];
        fts5SegIterHashInit(p, pTerm, nTerm, flags, pIter);
      }
      for(pLvl=&pStruct->aLevel[0]; pLvl<pEnd; pLvl++){
        for(iSeg=pLvl->nSeg-1; iSeg>=0; iSeg--){
          Fts5StructureSegment *pSeg = &pLvl->aSeg[iSeg];
          Fts5SegIter *pIter = &pNew->aSeg[iIter++];
          if( pTerm==0 ){
            fts5SegIterInit(p, pSeg, pIter);
          }else{
            fts5SegIterSeekInit(p, pTerm, nTerm, flags, pSeg, pIter);
          }
        }
      }
    }else{
      pLvl = &pStruct->aLevel[iLevel];
      for(iSeg=nSeg-1; iSeg>=0; iSeg--){
        fts5SegIterInit(p, &pLvl->aSeg[iSeg], &pNew->aSeg[iIter++]);
      }
    }
    assert( iIter==nSeg );
  }

  /* If the above was successful, each component iterator now points 
  ** to the first entry in its segment. In this case initialize the 
  ** aFirst[] array. Or, if an error has occurred, free the iterator
  ** object and set the output variable to NULL.  */
  if( p->rc==SQLITE_OK ){
    fts5MultiIterFinishSetup(p, pNew);
  }else{
    fts5MultiIterFree(pNew);
    *ppOut = 0;
  }

fts5MultiIterNew_post_check:
  assert( (*ppOut)!=0 || p->rc!=SQLITE_OK );
  return;
}
/* …（同文件无关代码省略）… */
static int fts5MultiIterEof(Fts5Index *p, Fts5Iter *pIter){
  assert( pIter!=0 || p->rc!=SQLITE_OK );
  assert( p->rc!=SQLITE_OK
      || (pIter->aSeg[ pIter->aFirst[1].iFirst ].pLeaf==0)==pIter->base.bEof 
  );
  return (p->rc || pIter->base.bEof);
}
/* …（同文件无关代码省略）… */
static i64 fts5MultiIterRowid(Fts5Iter *pIter){
  assert( pIter->aSeg[ pIter->aFirst[1].iFirst ].pLeaf );
  return pIter->aSeg[ pIter->aFirst[1].iFirst ].iRowid;
}
/* …（同文件无关代码省略）… */
static void fts5MultiIterNextFrom(
  Fts5Index *p, 
  Fts5Iter *pIter, 
  i64 iMatch
){
  while( 1 ){
    i64 iRowid;
    fts5MultiIterNext(p, pIter, 1, iMatch);
    if( fts5MultiIterEof(p, pIter) ) break;
    iRowid = fts5MultiIterRowid(pIter);
    if( pIter->bRev==0 && iRowid>=iMatch ) break;
    if( pIter->bRev!=0 && iRowid<=iMatch ) break;
  }
}
/* …（同文件无关代码省略）… */
static void fts5SecureDeleteIdxEntry(
  Fts5Index *p,                   /* FTS5 backend object */
  int iSegid,                     /* Id of segment to delete entry for */
  int iPgno                       /* Page number within segment */
){
  if( iPgno!=1 ){
    assert( p->pConfig->iVersion==FTS5_CURRENT_VERSION_SECUREDELETE );
    if( p->pDeleteFromIdx==0 ){
      fts5IndexPrepareStmt(p, &p->pDeleteFromIdx, sqlite3_mprintf(
          "DELETE FROM '%q'.'%q_idx' WHERE (segid, (pgno/2)) = (?1, ?2)",
          p->pConfig->zDb, p->pConfig->zName
      ));
    }
    if( p->rc==SQLITE_OK ){
      sqlite3_bind_int(p->pDeleteFromIdx, 1, iSegid);
      sqlite3_bind_int(p->pDeleteFromIdx, 2, iPgno);
      sqlite3_step(p->pDeleteFromIdx);
      p->rc = sqlite3_reset(p->pDeleteFromIdx);
    }
  }
}
/* …（同文件无关代码省略）… */
static void fts5SecureUpdateIdxEntry(
  Fts5Index *p,                   /* FTS5 backend object */
  int iSegid,                     /* Id of segment */
  int iPgno,                      /* Page number within segment */
  const u8 *pKey, int nKey        /* New key */
){
  assert( iPgno!=1 );
  if( p->rc==SQLITE_OK && p->pIdxKeyUpdate==0 ){
    fts5IndexPrepareStmt(p, &p->pIdxKeyUpdate, sqlite3_mprintf(
        "UPDATE '%q'.'%q_idx' SET term=?1 WHERE (segid, (pgno/2)) = (?2, ?3)",
        p->pConfig->zDb, p->pConfig->zName
    ));
  }
  if( p->rc==SQLITE_OK ){
    sqlite3_bind_blob(p->pIdxKeyUpdate, 1, pKey, nKey, SQLITE_STATIC);
    sqlite3_bind_int(p->pIdxKeyUpdate, 2, iSegid);
    sqlite3_bind_int(p->pIdxKeyUpdate, 3, iPgno);
    sqlite3_step(p->pIdxKeyUpdate);
    p->rc = sqlite3_reset(p->pIdxKeyUpdate);
    sqlite3_bind_null(p->pIdxKeyUpdate, 1);
  }
}
/* …（同文件无关代码省略）… */
static void fts5SecureDeleteOverflow(
  Fts5Index *p,
  Fts5StructureSegment *pSeg,
  int iPgno,
  int *pbLastInDoclist
){
  const int bDetailNone = (p->pConfig->eDetail==FTS5_DETAIL_NONE);
  int pgno;
  Fts5Data *pLeaf = 0;
  assert( iPgno!=1 );

  *pbLastInDoclist = 1;
  for(pgno=iPgno; p->rc==SQLITE_OK && pgno<=pSeg->pgnoLast; pgno++){
    i64 iRowid = FTS5_SEGMENT_ROWID(pSeg->iSegid, pgno);
    int iNext = 0;
    u8 *aPg = 0;

    pLeaf = fts5LeafRead(p, iRowid);
    if( pLeaf==0 ) break;
    aPg = pLeaf->p;

    iNext = fts5GetU16(&aPg[0]);
    if( iNext!=0 ){
      *pbLastInDoclist = 0;
    }
    if( iNext==0 && pLeaf->szLeaf<pLeaf->nn ){
      fts5GetVarint32(&aPg[pLeaf->szLeaf], iNext);
    }

    if( iNext==0 ){
      /* The page contains no terms or rowids. Replace it with an empty
      ** page and move on to the right-hand peer.  */
      const u8 aEmpty[] = {0x00, 0x00, 0x00, 0x04}; 
      assert_nc( bDetailNone==0 || pLeaf->nn==4 );
      if( bDetailNone==0 ) fts5DataWrite(p, iRowid, aEmpty, sizeof(aEmpty));
      fts5DataRelease(pLeaf);
      pLeaf = 0;
    }else if( bDetailNone ){
      break;
    }else if( iNext>=pLeaf->szLeaf || pLeaf->nn<pLeaf->szLeaf || iNext<4 ){
      FTS5_CORRUPT_ROWID(p, iRowid);
      break;
    }else{
      int nShift = iNext - 4;
      int nPg;

      int nIdx = 0;
      u8 *aIdx = 0;

      /* Unless the current page footer is 0 bytes in size (in which case
      ** the new page footer will be as well), allocate and populate a 
      ** buffer containing the new page footer. Set stack variables aIdx 
      ** and nIdx accordingly.  */
      if( pLeaf->nn>pLeaf->szLeaf ){
        int iFirst = 0;
        int i1 = pLeaf->szLeaf;
        int i2 = 0;

        i1 += fts5GetVarint32(&aPg[i1], iFirst);
        if( iFirst<iNext ){
          FTS5_CORRUPT_ROWID(p, iRowid);
          break;
        }
        aIdx = sqlite3Fts5MallocZero(&p->rc, (pLeaf->nn-pLeaf->szLeaf)+2);
        if( aIdx==0 ) break;
        i2 = sqlite3Fts5PutVarint(aIdx, iFirst-nShift);
        if( i1<pLeaf->nn ){
          memcpy(&aIdx[i2], &aPg[i1], pLeaf->nn-i1);
          i2 += (pLeaf->nn-i1);
        }
        nIdx = i2;
      }

      /* Modify the contents of buffer aPg[]. Set nPg to the new size 
      ** in bytes. The new page is always smaller than the old.  */
      nPg = pLeaf->szLeaf - nShift;
      memmove(&aPg[4], &aPg[4+nShift], nPg-4);
      fts5PutU16(&aPg[2], nPg);
      if( fts5GetU16(&aPg[0]) ) fts5PutU16(&aPg[0], 4);
      if( nIdx>0 ){
        memcpy(&aPg[nPg], aIdx, nIdx);
        nPg += nIdx;
      }
      sqlite3_free(aIdx);

      /* Write the new page to disk and exit the loop */
      assert( nPg>4 || fts5GetU16(aPg)==0 );
      fts5DataWrite(p, iRowid, aPg, nPg);
      break;
    }
  }
  fts5DataRelease(pLeaf);
}
/* …（同文件无关代码省略）… */
static void fts5DoSecureDelete(
  Fts5Index *p,
  Fts5SegIter *pSeg
){
  const int bDetailNone = (p->pConfig->eDetail==FTS5_DETAIL_NONE);
  int iSegid = pSeg->pSeg->iSegid;
  u8 *aPg = pSeg->pLeaf->p;
  int nPg = pSeg->pLeaf->nn;
  int iPgIdx = pSeg->pLeaf->szLeaf;         /* Offset of page footer */

  u64 iDelta = 0;
  int iNextOff = 0;
  int iOff = 0;
  int nIdx = 0;
  u8 *aIdx = 0;
  int bLastInDoclist = 0;
  int iIdx = 0;
  int iStart = 0;
  int iDelKeyOff = 0;       /* Offset of deleted key, if any */

  nIdx = nPg-iPgIdx;
  aIdx = sqlite3Fts5MallocZero(&p->rc, ((i64)nIdx)+16);
  if( p->rc ) return;
  memcpy(aIdx, &aPg[iPgIdx], nIdx);

  /* At this point segment iterator pSeg points to the entry
  ** this function should remove from the b-tree segment. 
  **
  ** In detail=full or detail=column mode, pSeg->iLeafOffset is the 
  ** offset of the first byte in the position-list for the entry to 
  ** remove. Immediately before this comes two varints that will also
  ** need to be removed:
  **
  **     + the rowid or delta rowid value for the entry, and
  **     + the size of the position list in bytes.
  **
  ** Or, in detail=none mode, there is a single varint prior to 
  ** pSeg->iLeafOffset - the rowid or delta rowid value.
  **
  ** This block sets the following variables:
  **
  **   iStart:
  **     The offset of the first byte of the rowid or delta-rowid
  **     value for the doclist entry being removed.
  **
  **   iDelta:
  **     The value of the rowid or delta-rowid value for the doclist
  **     entry being removed.
  **
  **   iNextOff:
  **     The offset of the next entry following the position list
  **     for the one being removed. If the position list for this
  **     entry overflows onto the next leaf page, this value will be
  **     greater than pLeaf->szLeaf.
  */
  {
    int iSOP;                     /* Start-Of-Position-list */
    if( pSeg->iLeafPgno==pSeg->iTermLeafPgno ){
      iStart = pSeg->iTermLeafOffset;
    }else{
      iStart = fts5GetU16(&aPg[0]);
    }
    if( iStart>=iPgIdx || pSeg->iLeafOffset>iPgIdx ){
      /* The rowid of the entry being removed, and the start of its position
      ** list, must be within the leaf content, which ends at szLeaf (iPgIdx).
      ** The page footer follows. */
      FTS5_CORRUPT_IDX(p);
      sqlite3_free(aIdx);
      return;
    }

    iSOP = iStart + fts5GetVarint(&aPg[iStart], &iDelta);
    assert_nc( iSOP<=pSeg->iLeafOffset );

    if( bDetailNone ){
      int iEnd = pSeg->iLeafOffset - (pSeg->bDel ? 1 + pSeg->nPos : 0);
      if( iEnd>iPgIdx ){
        FTS5_CORRUPT_IDX(p);
        sqlite3_free(aIdx);
        return;
      }
      while( iSOP<iEnd ){
        if( aPg[iSOP]==0x00 ) iSOP++;
        if( aPg[iSOP]==0x00 ) iSOP++;
        iStart = iSOP;
        iSOP = iStart + fts5GetVarint(&aPg[iStart], &iDelta);
      }

      iNextOff = iSOP;
      if( iNextOff<pSeg->iEndofDoclist && aPg[iNextOff]==0x00 ) iNextOff++;
      if( iNextOff<pSeg->iEndofDoclist && aPg[iNextOff]==0x00 ) iNextOff++;

    }else{
      int nPos = 0;
      iSOP += fts5GetVarint32(&aPg[iSOP], nPos);
      while( iSOP<pSeg->iLeafOffset ){
        /* nPos is the size of a position-list in bytes, times 2, plus the
        ** delete flag.  The rowid of the next entry follows it, and must
        ** come before the position-list of the entry being removed. */
        if( nPos<0 || (nPos/2)>=pSeg->iLeafOffset-iSOP ){
          FTS5_CORRUPT_IDX(p);
          sqlite3_free(aIdx);
          return;
        }
        iStart = iSOP + (nPos/2);
        iSOP = iStart + fts5GetVarint(&aPg[iStart], &iDelta);
        iSOP += fts5GetVarint32(&aPg[iSOP], nPos);
      }
      if( iSOP!=pSeg->iLeafOffset || pSeg->nPos<0 ){
        FTS5_CORRUPT_IDX(p);
        sqlite3_free(aIdx);
        return;
      }
      /* The position list may extend past the end of this page, but take
      ** care that iSOP + nPos cannot overflow. */
      iNextOff = (pSeg->nPos>=iPgIdx-iSOP) ? iPgIdx : iSOP + pSeg->nPos;
    }
  }

  iOff = iStart;

  /* If the position-list for the entry being removed flows over past
  ** the end of this page, delete the portion of the position-list on the
  ** next page and beyond.
  **
  ** Set variable bLastInDoclist to true if this entry happens 
  ** to be the last rowid in the doclist for its term.  */
  if( iNextOff>=iPgIdx ){
    int pgno = pSeg->iLeafPgno+1;
    fts5SecureDeleteOverflow(p, pSeg->pSeg, pgno, &bLastInDoclist);
    iNextOff = iPgIdx;
  }

  if( pSeg->bDel==0 ){
    if( iNextOff!=iPgIdx ){
      /* Loop through the page-footer. If iNextOff (offset of the
      ** entry following the one we are removing) is equal to the 
      ** offset of a key on this page, then the entry is the last 
      ** in its doclist. */
      int iKeyOff = 0;
      for(iIdx=0; iIdx<nIdx; /* no-op */){
        u32 iVal = 0;
        iIdx += fts5GetVarint32(&aIdx[iIdx], iVal);
        iKeyOff += iVal;
        if( iKeyOff==iNextOff ){
          bLastInDoclist = 1;
        }
      }
    }

    /* If this is (a) the first rowid on a page and (b) is not followed by
    ** another position list on the same page, set the "first-rowid" field
    ** of the header to 0.  */
    if( fts5GetU16(&aPg[0])==iStart && (bLastInDoclist || iNextOff==iPgIdx) ){
      fts5PutU16(&aPg[0], 0);
    }
  }

  if( pSeg->bDel ){
    iOff += sqlite3Fts5PutVarint(&aPg[iOff], iDelta);
    aPg[iOff++] = bDetailNone ? 0x00 : 0x01;
  }else if( bLastInDoclist==0 ){
    if( iNextOff!=iPgIdx ){
      u64 iNextDelta = 0;
      iNextOff += fts5GetVarint(&aPg[iNextOff], &iNextDelta);
      if( iNextOff>iPgIdx ){
        FTS5_CORRUPT_IDX(p);
        sqlite3_free(aIdx);
        return;
      }
      iOff += sqlite3Fts5PutVarint(&aPg[iOff], iDelta + iNextDelta);
    }
  }else if( 
      pSeg->iLeafPgno==pSeg->iTermLeafPgno 
   && iStart==pSeg->iTermLeafOffset 
  ){
    /* The entry being removed was the only position list in its
    ** doclist. Therefore the term needs to be removed as well. */
    int iKey = 0;
    int iKeyOff = 0;

    /* Set iKeyOff to the offset of the term that will be removed - the
    ** last offset in the footer that is not greater than iStart. */
    for(iIdx=0; iIdx<nIdx; iKey++){
      u32 iVal = 0;
      iIdx += fts5GetVarint32(&aIdx[iIdx], iVal);
      if( (iKeyOff+iVal)>(u32)iStart ) break;
      iKeyOff += iVal;
    }
    if( iKey<1 || iKeyOff<4 || iKeyOff>=iStart ){
      /* There must be a term before the doclist.  Terms start after the
      ** 4-byte page header. */
      FTS5_CORRUPT_IDX(p);
      sqlite3_free(aIdx);
      return;
    }

    /* Set iDelKeyOff to the value of the footer entry to remove from 
    ** the page. */
    iDelKeyOff = iOff = iKeyOff;

    if( iNextOff!=iPgIdx ){
      /* This is the only position-list associated with the term, and there
      ** is another term following it on this page. So the subsequent term
      ** needs to be moved to replace the term associated with the entry
      ** being removed. */
      u64 nPrefix = 0;
      u64 nSuffix = 0;
      u64 nPrefix2 = 0;
      u64 nSuffix2 = 0;
      i64 nNeed;

      iDelKeyOff = iNextOff;
      iNextOff += fts5GetVarint(&aPg[iNextOff], &nPrefix2);
      iNextOff += fts5GetVarint(&aPg[iNextOff], &nSuffix2);

      if( iKey!=1 ){
        iKeyOff += fts5GetVarint(&aPg[iKeyOff], &nPrefix);
      }
      iKeyOff += fts5GetVarint(&aPg[iKeyOff], &nSuffix);

      nPrefix = MIN(nPrefix, nPrefix2);
      nSuffix = (nPrefix2 + nSuffix2) - nPrefix;

      /* Compare by subtraction, not addition, since the varints above can
      ** be any 64-bit value and a sum could wrap around.  After the first
      ** three tests nPrefix2 and nSuffix2 are small, so the sum in
      ** nSuffix cannot have wrapped and the last test is safe. */
      if( iNextOff>iPgIdx
       || nSuffix2>(u64)(iPgIdx-iNextOff)
       || nPrefix2>(u64)pSeg->term.n
       || (iKeyOff+nSuffix)>(u64)iPgIdx
      ){
        FTS5_CORRUPT_IDX(p);
      }else{
        u8 aTmp[10];
        if( iKey==1 && pSeg->iLeafPgno!=1 
         && nSuffix2>0
        ){
          /* The term being removed is the first on the page, so the %_idx
          ** key for the page is a prefix of it. Replace the key with a
          ** prefix of the new first term that is one byte longer than
          ** the prefix it shares with the removed term. */
          int nNew = (int)nPrefix2 + 1;
          u8 *aNew = (u8*)sqlite3Fts5MallocZero(&p->rc, nNew);
          if( aNew ){
            memcpy(aNew, pSeg->term.p, nNew-1);
            aNew[nNew-1] = aPg[iNextOff];
            fts5SecureUpdateIdxEntry(p, iSegid, pSeg->iLeafPgno, aNew, nNew);
            sqlite3_free(aNew);
          }
        }
        /* Make sure the replacement for the removed term fits in the space
        ** between iOff and the start of the next term's suffix. */
        nNeed = sqlite3Fts5PutVarint(aTmp, nSuffix)
              + (iKey!=1 ? sqlite3Fts5PutVarint(aTmp, nPrefix) : 0)
              + (nPrefix2>nPrefix ? (nPrefix2-nPrefix) : 0);
        if( (i64)iOff + nNeed > iNextOff ){
          FTS5_CORRUPT_IDX(p);
          sqlite3_free(aIdx);
          return;
        }
        if( iKey!=1 ){
          iOff += sqlite3Fts5PutVarint(&aPg[iOff], nPrefix);
        }
        iOff += sqlite3Fts5PutVarint(&aPg[iOff], nSuffix);
        if( nPrefix2>nPrefix ){
          memcpy(&aPg[iOff], &pSeg->term.p[nPrefix], nPrefix2-nPrefix);
          iOff += (nPrefix2-nPrefix);
        }
        memmove(&aPg[iOff], &aPg[iNextOff], nSuffix2);
        iOff += nSuffix2;
        iNextOff += nSuffix2;
      }
    }
  }else if( iStart==4 ){
    int iPgno;

    assert_nc( pSeg->iLeafPgno>pSeg->iTermLeafPgno );
    /* The entry being removed may be the only position list in
    ** its doclist. */
    for(iPgno=pSeg->iLeafPgno-1; iPgno>pSeg->iTermLeafPgno; iPgno-- ){
      Fts5Data *pPg = fts5LeafRead(p, FTS5_SEGMENT_ROWID(iSegid, iPgno));
      int bEmpty = (pPg && pPg->nn==4);
      fts5DataRelease(pPg);
      if( bEmpty==0 ) break;
    }

    if( iPgno==pSeg->iTermLeafPgno ){
      i64 iId = FTS5_SEGMENT_ROWID(iSegid, pSeg->iTermLeafPgno);
      Fts5Data *pTerm = fts5LeafRead(p, iId);
      if( pTerm && pTerm->szLeaf==pSeg->iTermLeafOffset ){
        u8 *aTermIdx = &pTerm->p[pTerm->szLeaf];
        int nTermIdx = pTerm->nn - pTerm->szLeaf;
        int iTermIdx = 0;
        i64 iTermOff = 0;

        /* The page holds a term, so it has a footer. */
        if( nTermIdx<=0 ){
          FTS5_CORRUPT_IDX(p);
          fts5DataRelease(pTerm);
          sqlite3_free(aIdx);
          return;
        }
        while( 1 ){
          u32 iVal = 0;
          int nByte = fts5GetVarint32(&aTermIdx[iTermIdx], iVal);
          iTermOff += iVal;
          if( (iTermIdx+nByte)>=nTermIdx ) break;
          iTermIdx += nByte;
        }
        nTermIdx = iTermIdx;

        if( iTermOff>pTerm->szLeaf || iTermOff<4 ){
          FTS5_CORRUPT_IDX(p);
        }else{
          memmove(&pTerm->p[iTermOff], &pTerm->p[pTerm->szLeaf], nTermIdx);
          fts5PutU16(&pTerm->p[2], iTermOff);
          fts5DataWrite(p, iId, pTerm->p, iTermOff+nTermIdx);
          if( nTermIdx==0 ){
            fts5SecureDeleteIdxEntry(p, iSegid, pSeg->iTermLeafPgno);
          }
        }
      }
      fts5DataRelease(pTerm);
    }
  }

  /* Assuming no error has occurred, this block does final edits to the
  ** leaf page before writing it back to disk. Input variables are:
  **
  **   nPg: Total initial size of leaf page.
  **   iPgIdx: Initial offset of page footer.
  **
  **   iOff: Offset to move data to
  **   iNextOff: Offset to move data from
  */
  if( p->rc==SQLITE_OK ){
    const int nMove = nPg - iNextOff;     /* Number of bytes to move */
    int nShift = iNextOff - iOff;         /* Distance to move them */
    int nNewPg = 0;
    int iPrevKeyOut = 0;
    i64 iKeyIn = 0;

    if( nShift<0 ){
      /* Removing an entry cannot make the page larger. */
      FTS5_CORRUPT_IDX(p);
      sqlite3_free(aIdx);
      return;
    }
    if( nMove>0 ){
      memmove(&aPg[iOff], &aPg[iNextOff], nMove);
    }
    iPgIdx -= nShift;
    nNewPg = iPgIdx;
    fts5PutU16(&aPg[2], iPgIdx);

    for(iIdx=0; iIdx<nIdx; /* no-op */){
      u32 iVal = 0;
      iIdx += fts5GetVarint32(&aIdx[iIdx], iVal);
      iKeyIn += iVal;
      if( iKeyIn>nPg ){
        FTS5_CORRUPT_IDX(p);
        break;
      }
      if( iKeyIn!=iDelKeyOff ){
        int iKeyOut = (int)(iKeyIn - (iKeyIn>iOff ? nShift : 0));
        if( iKeyOut<iPrevKeyOut ){
          /* Footer entries must be in ascending order. */
          FTS5_CORRUPT_IDX(p);
          break;
        }
        nNewPg += sqlite3Fts5PutVarint(&aPg[nNewPg], iKeyOut - iPrevKeyOut);
        iPrevKeyOut = iKeyOut;
      }
    }

    if( p->rc==SQLITE_OK && iPgIdx==nNewPg && nIdx>0 && pSeg->iLeafPgno!=1 ){
      fts5SecureDeleteIdxEntry(p, iSegid, pSeg->iLeafPgno);
    }

    assert_nc( nNewPg>4 || fts5GetU16(aPg)==0 );
    fts5DataWrite(p, FTS5_SEGMENT_ROWID(iSegid,pSeg->iLeafPgno), aPg, nNewPg);
  }
  sqlite3_free(aIdx);
}
/* …（同文件无关代码省略）… */
static int fts5FlushSecureDelete(
  Fts5Index *p,
  Fts5Structure *pStruct,
  const char *zTerm,
  int nTerm,
  i64 iRowid
){
  const int f = FTS5INDEX_QUERY_SKIPHASH;
  Fts5Iter *pIter = 0;            /* Used to find term instance */

  /* If the version number has not been set to SECUREDELETE, do so now. */
  if( p->pConfig->iVersion!=FTS5_CURRENT_VERSION_SECUREDELETE ){
    Fts5Config *pConfig = p->pConfig;
    sqlite3_stmt *pStmt = 0;
    fts5IndexPrepareStmt(p, &pStmt, sqlite3_mprintf(
          "REPLACE INTO %Q.'%q_config' VALUES ('version', %d)",
          pConfig->zDb, pConfig->zName, FTS5_CURRENT_VERSION_SECUREDELETE
    ));
    if( p->rc==SQLITE_OK ){
      int rc;
      sqlite3_step(pStmt);
      rc = sqlite3_finalize(pStmt);
      if( p->rc==SQLITE_OK ) p->rc = rc;
      pConfig->iCookie++;
      pConfig->iVersion = FTS5_CURRENT_VERSION_SECUREDELETE;
    }
  }

  fts5MultiIterNew(p, pStruct, f, 0, (const u8*)zTerm, nTerm, -1, 0, &pIter);
  if( fts5MultiIterEof(p, pIter)==0 ){
    i64 iThis = fts5MultiIterRowid(pIter);
    if( iThis<iRowid ){
      fts5MultiIterNextFrom(p, pIter, iRowid);
    }

    if( p->rc==SQLITE_OK 
     && fts5MultiIterEof(p, pIter)==0 
     && iRowid==fts5MultiIterRowid(pIter)
    ){
      Fts5SegIter *pSeg = &pIter->aSeg[pIter->aFirst[1].iFirst];
      fts5DoSecureDelete(p, pSeg);
    }
  }

  fts5MultiIterFree(pIter);
  return p->rc;
}
/* …（同文件无关代码省略）… */
                    continue;
                  }
                }
              }else if( (pDoclist[iOff] & 0x01) 
                && !fts5FlushSecureDelete(p, pStruct, zTerm, nTerm, iRowid)
              ){
                if( p->rc!=SQLITE_OK || pDoclist[iOff]==0x01 ){
                  iOff++;
                  continue;
                }
              }
            }
