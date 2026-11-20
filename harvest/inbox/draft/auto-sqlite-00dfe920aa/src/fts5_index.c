// AUTO-DRAFT from sqlite/sqlite PR #67897e97219d9cc6dccbcb89fcecd709faa4bc86
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct Fts5SegIter Fts5SegIter;
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
  sqlite3_stmt *pIdxKeyUpdate;    /* UPDATE %_idx SET term WHERE (segid,pgno) */

  sqlite3_stmt *pDataVersion;
  i64 iStructVersion;             /* data_version when pStruct read */
  Fts5Structure *pStruct;         /* Current db structure (or NULL) */
};
/* …（同文件无关代码省略）… */
static int fts5IndexCorruptIter(Fts5Index *pIdx, Fts5SegIter *pIter){
  pIdx->rc = FTS5_CORRUPT;
  sqlite3Fts5ConfigErrmsg(pIdx->pConfig, 
      "fts5: corruption on page %d, segment %d, table \"%s\"", 
      pIter->iLeafPgno, pIter->pSeg->iSegid, pIdx->pConfig->zName
  );
  return SQLITE_CORRUPT_VTAB;
}
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
static u16 fts5GetU16(const u8 *aIn){
  return ((u16)aIn[0] << 8) + aIn[1];
} 
/* …（同文件无关代码省略）… */
        bNewTerm = 1;
      }
      assert_nc( iOff<pLeaf->szLeaf );
      if( iOff>pLeaf->szLeaf ){
        FTS5_CORRUPT_ITER(p, pIter);
        return;
      }
/* …（同文件无关代码省略）… */
    }else{
      iStart = fts5GetU16(&aPg[0]);
    }
    if( iStart>nPg ){
      FTS5_CORRUPT_IDX(p);
      sqlite3_free(aIdx);
      return;
/* …（同文件无关代码省略）… */

    if( bDetailNone ){
      int iEnd = pSeg->iLeafOffset - (pSeg->bDel ? 1 + pSeg->nPos : 0);
      while( iSOP<iEnd ){
        if( aPg[iSOP]==0x00 ) iSOP++;
        if( aPg[iSOP]==0x00 ) iSOP++;
/* …（同文件无关代码省略）… */
      int nPos = 0;
      iSOP += fts5GetVarint32(&aPg[iSOP], nPos);
      while( iSOP<pSeg->iLeafOffset ){
        iStart = iSOP + (nPos/2);
        iSOP = iStart + fts5GetVarint(&aPg[iStart], &iDelta);
        iSOP += fts5GetVarint32(&aPg[iSOP], nPos);
      }
      assert_nc( iSOP==pSeg->iLeafOffset );
      iNextOff = iSOP + pSeg->nPos;
    }
  }

/* …（同文件无关代码省略）… */
    if( iNextOff!=iPgIdx ){
      u64 iNextDelta = 0;
      iNextOff += fts5GetVarint(&aPg[iNextOff], &iNextDelta);
      iOff += sqlite3Fts5PutVarint(&aPg[iOff], iDelta + iNextDelta);
    }
  }else if( 
/* …（同文件无关代码省略）… */
      if( (iKeyOff+iVal)>(u32)iStart ) break;
      iKeyOff += iVal;
    }
    assert_nc( iKey>=1 );

    /* Set iDelKeyOff to the value of the footer entry to remove from 
    ** the page. */
/* …（同文件无关代码省略）… */
      u64 nSuffix = 0;
      u64 nPrefix2 = 0;
      u64 nSuffix2 = 0;

      iDelKeyOff = iNextOff;
      iNextOff += fts5GetVarint(&aPg[iNextOff], &nPrefix2);
/* …（同文件无关代码省略）… */
      nPrefix = MIN(nPrefix, nPrefix2);
      nSuffix = (nPrefix2 + nSuffix2) - nPrefix;

      if( (iKeyOff+nSuffix)>(u64)iPgIdx || (iNextOff+nSuffix2)>(u64)iPgIdx ){
        FTS5_CORRUPT_IDX(p);
      }else{
        if( iKey==1 && pSeg->iLeafPgno!=1 
         && nPrefix2<=(u64)pSeg->term.n && nSuffix2>0
        ){
          /* The term being removed is the first on the page, so the %_idx
          ** key for the page is a prefix of it. Replace the key with a
/* …（同文件无关代码省略）… */
            sqlite3_free(aNew);
          }
        }
        if( iKey!=1 ){
          iOff += sqlite3Fts5PutVarint(&aPg[iOff], nPrefix);
        }
        iOff += sqlite3Fts5PutVarint(&aPg[iOff], nSuffix);
        if( nPrefix2>(u64)pSeg->term.n ){
          FTS5_CORRUPT_IDX(p);
        }else if( nPrefix2>nPrefix ){
          memcpy(&aPg[iOff], &pSeg->term.p[nPrefix], nPrefix2-nPrefix);
          iOff += (nPrefix2-nPrefix);
        }
/* …（同文件无关代码省略）… */
        int iTermIdx = 0;
        i64 iTermOff = 0;

        while( 1 ){
          u32 iVal = 0;
          int nByte = fts5GetVarint32(&aTermIdx[iTermIdx], iVal);
/* …（同文件无关代码省略）… */
        }
        nTermIdx = iTermIdx;

        if( iTermOff>pTerm->szLeaf ){
          FTS5_CORRUPT_IDX(p);
        }else{
          memmove(&pTerm->p[iTermOff], &pTerm->p[pTerm->szLeaf], nTermIdx);
/* …（同文件无关代码省略）… */
    int iPrevKeyOut = 0;
    i64 iKeyIn = 0;

    if( nMove>0 ){
      memmove(&aPg[iOff], &aPg[iNextOff], nMove);
    }
/* …（同文件无关代码省略）… */
        break;
      }
      if( iKeyIn!=iDelKeyOff ){
        int iKeyOut = (iKeyIn - (iKeyIn>iOff ? nShift : 0));
        nNewPg += sqlite3Fts5PutVarint(&aPg[nNewPg], iKeyOut - iPrevKeyOut);
 
