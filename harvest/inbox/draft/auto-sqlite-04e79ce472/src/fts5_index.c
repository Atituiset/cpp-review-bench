// AUTO-DRAFT from sqlite/sqlite PR #03c38c992ae0f01fd88957a1b55c254bc0d0f9bc
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

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

  sqlite3_stmt *pDataVersion;
  i64 iStructVersion;             /* data_version when pStruct read */
  Fts5Structure *pStruct;         /* Current db structure (or NULL) */
};
/* …（同文件无关代码省略）… */
#define ASSERT_SZLEAF_OK(x) assert( \
    (x)->szLeaf==(x)->nn || (x)->szLeaf==fts5GetU16(&(x)->p[2]) \
)
/* …（同文件无关代码省略）… */
static u16 fts5GetU16(const u8 *aIn){
  return ((u16)aIn[0] << 8) + aIn[1];
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
          fts5SegIterLoadNPos(p, pIter);
        }
        break;
      }
    }
  }
