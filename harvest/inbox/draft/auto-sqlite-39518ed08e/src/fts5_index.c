// AUTO-DRAFT from sqlite/sqlite PR #814fd8ef4d42d6567f1613a66f556c8194764d06
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
#define FTS5_DLIDX_ROWID(segid, height, pgno) fts5_dri(segid, 1, height, pgno)
/* …（同文件无关代码省略）… */
#define FTS5_DATA_PADDING 20
/* …（同文件无关代码省略）… */
typedef struct Fts5Data Fts5Data;
/* …（同文件无关代码省略）… */
typedef struct Fts5DlidxWriter Fts5DlidxWriter;
typedef struct Fts5Iter Fts5Iter;
typedef struct Fts5PageWriter Fts5PageWriter;
typedef struct Fts5SegIter Fts5SegIter;
/* …（同文件无关代码省略）… */
typedef struct Fts5SegWriter Fts5SegWriter;
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
  i64 iRowid;                     /* Current rowid */
  int nPos;                       /* Number of bytes in current position list */
  u8 bDel;                        /* True if the delete flag is set */
};

static int fts5IndexCorruptRowid(Fts5Index *pIdx, i64 iRowid){
/* …（同文件无关代码省略）… */
#define FTS5_CORRUPT_ROWID(pIdx, iRowid) fts5IndexCorruptRowid(pIdx, iRowid)
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
#define FTS5_SEGITER_REVERSE 0x02
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
          SQLITE_PREPARE_PERSISTENT|SQLITE_PREPARE_NO_VTAB,
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
  sqlite3_bind_blob(p->pWriter, 2, pData, nData, SQLITE_STATIC);
  sqlite3_step(p->pWriter);
  p->rc = sqlite3_reset(p->pWriter);
  sqlite3_bind_null(p->pWriter, 2);
}
/* …（同文件无关代码省略）… */
      assert_nc( i2!=0 );
      pRes->bTermEq = 1;
      if( p1->iRowid==p2->iRowid ){
        return i2;
      }
      res = ((p1->iRowid > p2->iRowid)==pIter->bRev) ? -1 : +1;
/* …（同文件无关代码省略）… */
static void fts5ChunkIterate(
  Fts5Index *p,                   /* Index object */
  Fts5SegIter *pSeg,              /* Poslist of this iterator */
  void *pCtx,                     /* Context pointer for xChunk callback */
  void (*xChunk)(Fts5Index*, void*, const u8*, int)
){
  int nRem = pSeg->nPos;          /* Number of bytes still to come */
  Fts5Data *pData = 0;
  u8 *pChunk = &pSeg->pLeaf->p[pSeg->iLeafOffset];
  int nChunk = MIN(nRem, pSeg->pLeaf->szLeaf - pSeg->iLeafOffset);
  int pgno = pSeg->iLeafPgno;
  int pgnoSave = 0;

  /* This function does not work with detail=none databases. */
  assert( p->pConfig->eDetail!=FTS5_DETAIL_NONE );

  if( (pSeg->flags & FTS5_SEGITER_REVERSE)==0 ){
    pgnoSave = pgno+1;
  }

  while( 1 ){
    xChunk(p, pCtx, pChunk, nChunk);
    nRem -= nChunk;
    fts5DataRelease(pData);
    if( nRem<=0 ){
      break;
    }else if( pSeg->pSeg==0 ){
      FTS5_CORRUPT_IDX(p);
      return;
    }else{
      pgno++;
      pData = fts5LeafRead(p, FTS5_SEGMENT_ROWID(pSeg->pSeg->iSegid, pgno));
      if( pData==0 ) break;
      pChunk = &pData->p[4];
      nChunk = MIN(nRem, pData->szLeaf - 4);
      if( pgno==pgnoSave ){
        assert( pSeg->pNextLeaf==0 );
        pSeg->pNextLeaf = pData;
        pData = 0;
      }
    }
  }
}
/* …（同文件无关代码省略）… */
static i64 fts5MultiIterRowid(Fts5Iter *pIter){
  assert( pIter->aSeg[ pIter->aFirst[1].iFirst ].pLeaf );
  return pIter->aSeg[ pIter->aFirst[1].iFirst ].iRowid;
}
/* …（同文件无关代码省略）… */
static const u8 *fts5MultiIterTerm(Fts5Iter *pIter, int *pn){
  Fts5SegIter *p = &pIter->aSeg[ pIter->aFirst[1].iFirst ];
  *pn = p->term.n;
  return p->term.p;
}
/* …（同文件无关代码省略）… */
static int fts5WriteDlidxGrow(
  Fts5Index *p,
  Fts5SegWriter *pWriter,
  int nLvl
){
  if( p->rc==SQLITE_OK && nLvl>=pWriter->nDlidx ){
    Fts5DlidxWriter *aDlidx = (Fts5DlidxWriter*)sqlite3_realloc64(
        pWriter->aDlidx, sizeof(Fts5DlidxWriter) * nLvl
    );
    if( aDlidx==0 ){
      p->rc = SQLITE_NOMEM;
    }else{
      size_t nByte = sizeof(Fts5DlidxWriter) * (nLvl - pWriter->nDlidx);
      memset(&aDlidx[pWriter->nDlidx], 0, nByte);
      pWriter->aDlidx = aDlidx;
      pWriter->nDlidx = nLvl;
    }
  }
  return p->rc;
}
/* …（同文件无关代码省略）… */
static void fts5WriteBtreeNoTerm(
  Fts5Index *p,                   /* FTS5 backend object */
  Fts5SegWriter *pWriter          /* Writer object */
){
  /* If there were no rowids on the leaf page either and the doclist-index
  ** has already been started, append an 0x00 byte to it.  */
  if( pWriter->bFirstRowidInPage && pWriter->aDlidx[0].buf.n>0 ){
    Fts5DlidxWriter *pDlidx = &pWriter->aDlidx[0];
    assert( pDlidx->bPrevValid );
    sqlite3Fts5BufferAppendVarint(&p->rc, &pDlidx->buf, 0);
  }

  /* Increment the "number of sequential leaves without a term" counter. */
  pWriter->nEmpty++;
}
/* …（同文件无关代码省略）… */
static i64 fts5DlidxExtractFirstRowid(Fts5Buffer *pBuf){
  i64 iRowid;
  int iOff;

  iOff = 1 + fts5GetVarint(&pBuf->p[1], (u64*)&iRowid);
  fts5GetVarint(&pBuf->p[iOff], (u64*)&iRowid);
  return iRowid;
}
/* …（同文件无关代码省略）… */
static void fts5WriteDlidxAppend(
  Fts5Index *p, 
  Fts5SegWriter *pWriter, 
  i64 iRowid
){
  int i;
  int bDone = 0;

  for(i=0; p->rc==SQLITE_OK && bDone==0; i++){
    i64 iVal;
    Fts5DlidxWriter *pDlidx = &pWriter->aDlidx[i];

    if( pDlidx->buf.n>=p->pConfig->pgsz ){
      /* The current doclist-index page is full. Write it to disk and push
      ** a copy of iRowid (which will become the first rowid on the next
      ** doclist-index leaf page) up into the next level of the b-tree 
      ** hierarchy. If the node being flushed is currently the root node,
      ** also push its first rowid upwards. */
      pDlidx->buf.p[0] = 0x01;    /* Not the root node */
      fts5DataWrite(p, 
          FTS5_DLIDX_ROWID(pWriter->iSegid, i, pDlidx->pgno),
          pDlidx->buf.p, pDlidx->buf.n
      );
      fts5WriteDlidxGrow(p, pWriter, i+2);
      pDlidx = &pWriter->aDlidx[i];
      if( p->rc==SQLITE_OK && pDlidx[1].buf.n==0 ){
        i64 iFirst = fts5DlidxExtractFirstRowid(&pDlidx->buf);

        /* This was the root node. Push its first rowid up to the new root. */
        pDlidx[1].pgno = pDlidx->pgno;
        sqlite3Fts5BufferAppendVarint(&p->rc, &pDlidx[1].buf, 0);
        sqlite3Fts5BufferAppendVarint(&p->rc, &pDlidx[1].buf, pDlidx->pgno);
        sqlite3Fts5BufferAppendVarint(&p->rc, &pDlidx[1].buf, iFirst);
        pDlidx[1].bPrevValid = 1;
        pDlidx[1].iPrev = iFirst;
      }

      sqlite3Fts5BufferZero(&pDlidx->buf);
      pDlidx->bPrevValid = 0;
      pDlidx->pgno++;
    }else{
      bDone = 1;
    }

    if( pDlidx->bPrevValid ){
      iVal = (u64)iRowid - (u64)pDlidx->iPrev;
    }else{
      i64 iPgno = (i==0 ? pWriter->writer.pgno : pDlidx[-1].pgno);
      assert( pDlidx->buf.n==0 );
      sqlite3Fts5BufferAppendVarint(&p->rc, &pDlidx->buf, !bDone);
      sqlite3Fts5BufferAppendVarint(&p->rc, &pDlidx->buf, iPgno);
      iVal = iRowid;
    }

    sqlite3Fts5BufferAppendVarint(&p->rc, &pDlidx->buf, iVal);
    pDlidx->bPrevValid = 1;
    pDlidx->iPrev = iRowid;
  }
}
/* …（同文件无关代码省略）… */
static void fts5WriteFlushLeaf(Fts5Index *p, Fts5SegWriter *pWriter){
  static const u8 zero[] = { 0x00, 0x00, 0x00, 0x00 };
  Fts5PageWriter *pPage = &pWriter->writer;
  i64 iRowid;

  assert( (pPage->pgidx.n==0)==(pWriter->bFirstTermInPage) );

  /* Set the szLeaf header field. */
  assert( 0==fts5GetU16(&pPage->buf.p[2]) );
  fts5PutU16(&pPage->buf.p[2], (u16)pPage->buf.n);

  if( pWriter->bFirstTermInPage ){
    /* No term was written to this page. */
    assert( pPage->pgidx.n==0 );
    fts5WriteBtreeNoTerm(p, pWriter);
  }else{
    /* Append the pgidx to the page buffer. Set the szLeaf header field. */
    fts5BufferAppendBlob(&p->rc, &pPage->buf, pPage->pgidx.n, pPage->pgidx.p);
  }

  /* Write the page out to disk */
  iRowid = FTS5_SEGMENT_ROWID(pWriter->iSegid, pPage->pgno);
  fts5DataWrite(p, iRowid, pPage->buf.p, pPage->buf.n);

  /* Initialize the next page. */
  fts5BufferZero(&pPage->buf);
  fts5BufferZero(&pPage->pgidx);
  fts5BufferAppendBlob(&p->rc, &pPage->buf, 4, zero);
  pPage->iPrevPgidx = 0;
  pPage->pgno++;

  /* Increase the leaves written counter */
  pWriter->nLeafWritten++;

  /* The new leaf holds no terms or rowids */
  pWriter->bFirstTermInPage = 1;
  pWriter->bFirstRowidInPage = 1;
}
/* …（同文件无关代码省略）… */
static void fts5WriteAppendRowid(
  Fts5Index *p, 
  Fts5SegWriter *pWriter,
  i64 iRowid
){
  if( p->rc==SQLITE_OK ){
    Fts5PageWriter *pPage = &pWriter->writer;

    if( (pPage->buf.n + pPage->pgidx.n)>=p->pConfig->pgsz ){
      fts5WriteFlushLeaf(p, pWriter);
    }

    /* If this is to be the first rowid written to the page, set the 
    ** rowid-pointer in the page-header. Also append a value to the dlidx
    ** buffer, in case a doclist-index is required.  */
    if( pWriter->bFirstRowidInPage ){
      fts5PutU16(pPage->buf.p, (u16)pPage->buf.n);
      fts5WriteDlidxAppend(p, pWriter, iRowid);
    }

    /* Write the rowid. */
    if( pWriter->bFirstRowidInDoclist || pWriter->bFirstRowidInPage ){
      fts5BufferAppendVarint(&p->rc, &pPage->buf, iRowid);
    }else{
      assert_nc( p->rc || iRowid>pWriter->iPrevRowid );
      fts5BufferAppendVarint(&p->rc, &pPage->buf, 
          (u64)iRowid - (u64)pWriter->iPrevRowid
      );
    }
    pWriter->iPrevRowid = iRowid;
    pWriter->bFirstRowidInDoclist = 0;
    pWriter->bFirstRowidInPage = 0;
  }
}
/* …（同文件无关代码省略）… */
static void fts5WriteAppendPoslistData(
  Fts5Index *p, 
  Fts5SegWriter *pWriter, 
  const u8 *aData, 
  int nData
){
  Fts5PageWriter *pPage = &pWriter->writer;
  const u8 *a = aData;
  int n = nData;
  
  assert( p->pConfig->pgsz>0 || p->rc!=SQLITE_OK );
  while( p->rc==SQLITE_OK 
     && (pPage->buf.n + pPage->pgidx.n + n)>=p->pConfig->pgsz 
  ){
    int nReq = p->pConfig->pgsz - pPage->buf.n - pPage->pgidx.n;
    int nCopy = 0;
    while( nCopy<nReq ){
      i64 dummy;
      nCopy += fts5GetVarint(&a[nCopy], (u64*)&dummy);
    }
    fts5BufferAppendBlob(&p->rc, &pPage->buf, nCopy, a);
    a += nCopy;
    n -= nCopy;
    fts5WriteFlushLeaf(p, pWriter);
  }
  if( n>0 ){
    fts5BufferAppendBlob(&p->rc, &pPage->buf, n, a);
  }
}
/* …（同文件无关代码省略）… */
static void fts5MergeChunkCallback(
  Fts5Index *p, 
  void *pCtx, 
  const u8 *pChunk, int nChunk
){
  Fts5SegWriter *pWriter = (Fts5SegWriter*)pCtx;
  fts5WriteAppendPoslistData(p, pWriter, pChunk, nChunk);
}
/* …（同文件无关代码省略）… */
    int nPos;                     /* position-list size field value */
    int nTerm;
    const u8 *pTerm;

    pTerm = fts5MultiIterTerm(pIter, &nTerm);
    if( nTerm!=term.n || fts5Memcmp(pTerm, term.p, nTerm) ){
/* …（同文件无关代码省略）… */
    }

    /* Check for key annihilation. */
    if( pSegIter->nPos==0 && (bOldest || pSegIter->bDel==0) ) continue;

    if( p->rc==SQLITE_OK && bTermWritten==0 ){
      /* This is a new term. Append a term to the output segment. */
/* …（同文件无关代码省略）… */
    fts5WriteAppendRowid(p, &writer, fts5MultiIterRowid(pIter));

    if( eDetail==FTS5_DETAIL_NONE ){
      if( pSegIter->bDel ){
        fts5BufferAppendVarint(&p->rc, &writer.writer.buf, 0);
        if( pSegIter->nPos>0 ){
          fts5BufferAppendVarint(&p->rc, &writer.writer.buf, 0);
        }
      }
    }else{
      /* Append the position-list data to the output */
      nPos = pSegIter->nPos*2 + pSegIter->bDel;
      fts5BufferAppendVarint(&p->rc, &writer.writer.buf, nPos);
      fts5ChunkIterate(p, pSegIter, (void*)&writer, fts5MergeChunkCallback);
    }
/* …（同文件无关代码省略）… */
    assert_nc( iSOP<=pSeg->iLeafOffset );

    if( bDetailNone ){
      while( iSOP<pSeg->iLeafOffset ){
        if( aPg[iSOP]==0x00 ) iSOP++;
        if( aPg[iSOP]==0x00 ) iSOP++;
        iStart = iSOP;
/* …（同文件无关代码省略）… */

  if( pSeg->bDel ){
    iOff += sqlite3Fts5PutVarint(&aPg[iOff], iDelta);
    aPg[iOff++] = 0x01;
  }else if( bLastInDoclist==0 ){
    if( iNextOff!=iPgIdx ){
      u64 iNextDelta = 0;
