// AUTO-DRAFT from sqlite/sqlite PR #3d26d25a76f3d32afc73f24cd3c4ffb77925679c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
int corruptPageError(int lineno, MemPage *p){
  char *zMsg;
  sqlite3BeginBenignMalloc();
  zMsg = sqlite3_mprintf("database corruption page %u of %s",
             p->pgno, sqlite3PagerFilename(p->pBt->pPager, 0)
  );
  sqlite3EndBenignMalloc();
  if( zMsg ){
    sqlite3ReportError(SQLITE_CORRUPT, lineno, zMsg);
  }
  sqlite3_free(zMsg);
  return SQLITE_CORRUPT_BKPT;
}
# define SQLITE_CORRUPT_PAGE(pMemPage) corruptPageError(__LINE__, pMemPage)
/* …（同文件无关代码省略）… */
static int cursorHoldsMutex(BtCursor *p){
  return sqlite3_mutex_held(p->pBt->mutex);
}
/* …（同文件无关代码省略）… */
static int cursorOwnsBtShared(BtCursor *p){
  assert( cursorHoldsMutex(p) );
  return (p->pBtree->db==p->pBt->db);
}
/* …（同文件无关代码省略）… */
void sqlite3BtreeClearCursor(BtCursor *pCur){
  assert( cursorHoldsMutex(pCur) );
  sqlite3_free(pCur->pKey);
  pCur->pKey = 0;
  pCur->eState = CURSOR_INVALID;
}
/* …（同文件无关代码省略）… */
static int btreeMoveto(
  BtCursor *pCur,     /* Cursor open on the btree to be searched */
  const void *pKey,   /* Packed key if the btree is an index */
  i64 nKey,           /* Integer key for tables.  Size of pKey for indices */
  int bias,           /* Bias search to the high end */
  int *pRes           /* Write search results here */
){
  int rc;                    /* Status code */
  UnpackedRecord *pIdxKey;   /* Unpacked index key */

  if( pKey ){
    KeyInfo *pKeyInfo = pCur->pKeyInfo;
    assert( nKey==(i64)(int)nKey );
    pIdxKey = sqlite3VdbeAllocUnpackedRecord(pKeyInfo);
    if( pIdxKey==0 ) return SQLITE_NOMEM_BKPT;
    sqlite3VdbeRecordUnpack((int)nKey, pKey, pIdxKey);
    if( pIdxKey->nField==0 || pIdxKey->nField>pKeyInfo->nAllField ){
      rc = SQLITE_CORRUPT_BKPT;
    }else{
      rc = sqlite3BtreeIndexMoveto(pCur, pIdxKey, pRes);
    }
    sqlite3DbFree(pCur->pKeyInfo->db, pIdxKey);
  }else{
    pIdxKey = 0;
    rc = sqlite3BtreeTableMoveto(pCur, nKey, bias, pRes);
  }
  return rc;
}
/* …（同文件无关代码省略）… */
static int btreeRestoreCursorPosition(BtCursor *pCur){
  int rc;
  int skipNext = 0;
  assert( cursorOwnsBtShared(pCur) );
  assert( pCur->eState>=CURSOR_REQUIRESEEK );
  if( pCur->eState==CURSOR_FAULT ){
    return pCur->skipNext;
  }
  pCur->eState = CURSOR_INVALID;
  if( sqlite3FaultSim(410) ){
    rc = SQLITE_IOERR;
  }else{
    rc = btreeMoveto(pCur, pCur->pKey, pCur->nKey, 0, &skipNext);
  }
  if( rc==SQLITE_OK ){
    sqlite3_free(pCur->pKey);
    pCur->pKey = 0;
    assert( pCur->eState==CURSOR_VALID || pCur->eState==CURSOR_INVALID );
    if( skipNext ) pCur->skipNext = skipNext;
    if( pCur->skipNext && pCur->eState==CURSOR_VALID ){
      pCur->eState = CURSOR_SKIPNEXT;
    }
  }
  return rc;
}
/* …（同文件无关代码省略）… */
  #define ptrmapGet(w,x,y,z) SQLITE_OK
/* …（同文件无关代码省略）… */
#define findCell(P,I) \
  ((P)->aData + ((P)->maskPage & get2byteAligned(&(P)->aCellIdx[2*(I)])))
#define findCellPastPtr(P,I) \
  ((P)->aDataOfst + ((P)->maskPage & get2byteAligned(&(P)->aCellIdx[2*(I)])))
/* …（同文件无关代码省略）… */
static void btreeParseCell(
  MemPage *pPage,         /* Page containing the cell */
  int iCell,              /* The cell index.  First cell is 0 */
  CellInfo *pInfo         /* Fill in this structure */
){
  pPage->xParseCell(pPage, findCell(pPage, iCell), pInfo);
}
/* …（同文件无关代码省略）… */
static int btreeInitPage(MemPage *pPage){
  u8 *data;          /* Equal to pPage->aData */
  BtShared *pBt;        /* The main btree structure */

  assert( pPage->pBt!=0 );
  assert( pPage->pBt->db!=0 );
  assert( sqlite3_mutex_held(pPage->pBt->mutex) );
  assert( pPage->pgno==sqlite3PagerPagenumber(pPage->pDbPage) );
  assert( pPage == sqlite3PagerGetExtra(pPage->pDbPage) );
  assert( pPage->aData == sqlite3PagerGetData(pPage->pDbPage) );
  assert( pPage->isInit==0 );

  pBt = pPage->pBt;
  data = pPage->aData + pPage->hdrOffset;
  /* EVIDENCE-OF: R-28594-02890 The one-byte flag at offset 0 indicating
  ** the b-tree page type. */
  if( decodeFlags(pPage, data[0]) ){
    return SQLITE_CORRUPT_PAGE(pPage);
  }
  assert( pBt->pageSize>=512 && pBt->pageSize<=65536 );
  pPage->maskPage = (u16)(pBt->pageSize - 1);
  pPage->nOverflow = 0;
  pPage->cellOffset = (u16)(pPage->hdrOffset + 8 + pPage->childPtrSize);
  pPage->aCellIdx = data + pPage->childPtrSize + 8;
  pPage->aDataEnd = pPage->aData + pBt->pageSize;
  pPage->aDataOfst = pPage->aData + pPage->childPtrSize;
  /* EVIDENCE-OF: R-37002-32774 The two-byte integer at offset 3 gives the
  ** number of cells on the page. */
  pPage->nCell = get2byte(&data[3]);
  if( pPage->nCell>MX_CELL(pBt) ){
    /* To many cells for a single page.  The page must be corrupt */
    return SQLITE_CORRUPT_PAGE(pPage);
  }
  testcase( pPage->nCell==MX_CELL(pBt) );
  /* EVIDENCE-OF: R-24089-57979 If a page contains no cells (which is only
  ** possible for a root page of a table that contains no rows) then the
  ** offset to the cell content area will equal the page size minus the
  ** bytes of reserved space. */
  assert( pPage->nCell>0
       || get2byteNotZero(&data[5])==(int)pBt->usableSize
       || CORRUPT_DB );
  pPage->nFree = -1;  /* Indicate that this value is yet uncomputed */
  pPage->isInit = 1;
  if( pBt->db->flags & SQLITE_CellSizeCk ){
    return btreeCellSizeCheck(pPage);
  }
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
static MemPage *btreePageFromDbPage(DbPage *pDbPage, Pgno pgno, BtShared *pBt){
  MemPage *pPage = (MemPage*)sqlite3PagerGetExtra(pDbPage);
  if( pgno!=pPage->pgno ){
    pPage->aData = sqlite3PagerGetData(pDbPage);
    pPage->pDbPage = pDbPage;
    pPage->pBt = pBt;
    pPage->pgno = pgno;
    pPage->hdrOffset = pgno==1 ? 100 : 0;
  }
  assert( pPage->aData==sqlite3PagerGetData(pDbPage) );
  return pPage;
}
/* …（同文件无关代码省略）… */
static int btreeGetPage(
  BtShared *pBt,       /* The btree */
  Pgno pgno,           /* Number of the page to fetch */
  MemPage **ppPage,    /* Return the page in this parameter */
  int flags            /* PAGER_GET_NOCONTENT or PAGER_GET_READONLY */
){
  int rc;
  DbPage *pDbPage;

  assert( flags==0 || flags==PAGER_GET_NOCONTENT || flags==PAGER_GET_READONLY );
  assert( sqlite3_mutex_held(pBt->mutex) );
  rc = sqlite3PagerGet(pBt->pPager, pgno, (DbPage**)&pDbPage, flags);
  if( rc ) return rc;
  *ppPage = btreePageFromDbPage(pDbPage, pgno, pBt);
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
static Pgno btreePagecount(BtShared *pBt){
  return pBt->nPage;
}
/* …（同文件无关代码省略）… */
static int getAndInitPage(
  BtShared *pBt,                  /* The database file */
  Pgno pgno,                      /* Number of the page to get */
  MemPage **ppPage,               /* Write the page pointer here */
  int bReadOnly                   /* True for a read-only page */
){
  int rc;
  DbPage *pDbPage;
  MemPage *pPage;
  assert( sqlite3_mutex_held(pBt->mutex) );

  if( pgno>btreePagecount(pBt) ){
    *ppPage = 0;
    return SQLITE_CORRUPT_BKPT;
  }
  rc = sqlite3PagerGet(pBt->pPager, pgno, (DbPage**)&pDbPage, bReadOnly);
  if( rc ){
    *ppPage = 0;
    return rc;
  }
  pPage = (MemPage*)sqlite3PagerGetExtra(pDbPage);
  if( pPage->isInit==0 ){
    btreePageFromDbPage(pDbPage, pgno, pBt);
    rc = btreeInitPage(pPage);
    if( rc!=SQLITE_OK ){
      releasePage(pPage);
      *ppPage = 0;
      return rc;
    }
  }
  assert( pPage->pgno==pgno || CORRUPT_DB );
  assert( pPage->aData==sqlite3PagerGetData(pDbPage) );
  *ppPage = pPage;
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
static void releasePageNotNull(MemPage *pPage){
  assert( pPage->aData );
  assert( pPage->pBt );
  assert( pPage->pDbPage!=0 );
  assert( sqlite3PagerGetExtra(pPage->pDbPage) == (void*)pPage );
  assert( sqlite3PagerGetData(pPage->pDbPage)==pPage->aData );
  assert( sqlite3_mutex_held(pPage->pBt->mutex) );
  sqlite3PagerUnrefNotNull(pPage->pDbPage);
}
static void releasePage(MemPage *pPage){
  if( pPage ) releasePageNotNull(pPage);
}
/* …（同文件无关代码省略）… */
  #define assertCellInfo(x)
/* …（同文件无关代码省略）… */
static SQLITE_NOINLINE void getCellInfo(BtCursor *pCur){
  if( pCur->info.nSize==0 ){
    pCur->curFlags |= BTCF_ValidNKey;
    btreeParseCell(pCur->pPage,pCur->ix,&pCur->info);
  }else{
    assertCellInfo(pCur);
  }
}
/* …（同文件无关代码省略）… */
static int getOverflowPage(
  BtShared *pBt,               /* The database file */
  Pgno ovfl,                   /* Current overflow page number */
  MemPage **ppPage,            /* OUT: MemPage handle (may be NULL) */
  Pgno *pPgnoNext              /* OUT: Next overflow page number */
){
  Pgno next = 0;
  MemPage *pPage = 0;
  int rc = SQLITE_OK;

  assert( sqlite3_mutex_held(pBt->mutex) );
  assert(pPgnoNext);

#ifndef SQLITE_OMIT_AUTOVACUUM
  /* Try to find the next page in the overflow list using the
  ** autovacuum pointer-map pages. Guess that the next page in
  ** the overflow list is page number (ovfl+1). If that guess turns
  ** out to be wrong, fall back to loading the data of page
  ** number ovfl to determine the next page number.
  */
  if( pBt->autoVacuum ){
    Pgno pgno;
    Pgno iGuess = ovfl+1;
    u8 eType;

    while( PTRMAP_ISPAGE(pBt, iGuess) || iGuess==PENDING_BYTE_PAGE(pBt) ){
      iGuess++;
    }

    if( iGuess<=btreePagecount(pBt) ){
      rc = ptrmapGet(pBt, iGuess, &eType, &pgno);
      if( rc==SQLITE_OK && eType==PTRMAP_OVERFLOW2 && pgno==ovfl ){
        next = iGuess;
        rc = SQLITE_DONE;
      }
    }
  }
#endif

  assert( next==0 || rc==SQLITE_DONE );
  if( rc==SQLITE_OK ){
    rc = btreeGetPage(pBt, ovfl, &pPage, (ppPage==0) ? PAGER_GET_READONLY : 0);
    assert( rc==SQLITE_OK || pPage==0 );
    if( rc==SQLITE_OK ){
      next = get4byte(pPage->aData);
    }
  }

  *pPgnoNext = next;
  if( ppPage ){
    *ppPage = pPage;
  }else{
    releasePage(pPage);
  }
  return (rc==SQLITE_DONE ? SQLITE_OK : rc);
}
/* …（同文件无关代码省略）… */
static int copyPayload(
  void *pPayload,           /* Pointer to page data */
  void *pBuf,               /* Pointer to buffer */
  int nByte,                /* Number of bytes to copy */
  int eOp,                  /* 0 -> copy from page, 1 -> copy to page */
  DbPage *pDbPage           /* Page containing pPayload */
){
  if( eOp ){
    /* Copy data from buffer to page (a write operation) */
    int rc = sqlite3PagerWrite(pDbPage);
    if( rc!=SQLITE_OK ){
      return rc;
    }
    memcpy(pPayload, pBuf, nByte);
  }else{
    /* Copy data from page to buffer (a read operation) */
    memcpy(pBuf, pPayload, nByte);
  }
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
static int accessPayload(
  BtCursor *pCur,      /* Cursor pointing to entry to read from */
  u32 offset,          /* Begin reading this far into payload */
  u32 amt,             /* Read this many bytes */
  unsigned char *pBuf, /* Write the bytes into this buffer */
  int eOp              /* zero to read. non-zero to write. */
){
  unsigned char *aPayload;
  int rc = SQLITE_OK;
  int iIdx = 0;
  MemPage *pPage = pCur->pPage;               /* Btree page of current entry */
  BtShared *pBt = pCur->pBt;                  /* Btree this cursor belongs to */
#ifdef SQLITE_DIRECT_OVERFLOW_READ
  unsigned char * const pBufStart = pBuf;     /* Start of original out buffer */
#endif

  assert( pPage );
  assert( eOp==0 || eOp==1 );
  assert( pCur->eState==CURSOR_VALID );
  if( pCur->ix>=pPage->nCell ){
    return SQLITE_CORRUPT_PAGE(pPage);
  }
  assert( cursorHoldsMutex(pCur) );

  getCellInfo(pCur);
  aPayload = pCur->info.pPayload;
  assert( (u64)offset+(u64)amt <= (u64)pCur->info.nPayload );

  assert( aPayload > pPage->aData );
  if( (uptr)(aPayload - pPage->aData) > (pBt->usableSize - pCur->info.nLocal) ){
    /* Trying to read or write past the end of the data is an error.  The
    ** conditional above is really:
    **    &aPayload[pCur->info.nLocal] > &pPage->aData[pBt->usableSize]
    ** but is recast into its current form to avoid integer overflow problems
    */
    return SQLITE_CORRUPT_PAGE(pPage);
  }

  /* Check if data must be read/written to/from the btree page itself. */
  if( offset<pCur->info.nLocal ){
    int a = amt;
    if( a+offset>pCur->info.nLocal ){
      a = pCur->info.nLocal - offset;
    }
    rc = copyPayload(&aPayload[offset], pBuf, a, eOp, pPage->pDbPage);
    offset = 0;
    pBuf += a;
    amt -= a;
  }else{
    offset -= pCur->info.nLocal;
  }


  if( rc==SQLITE_OK && amt>0 ){
    const u32 ovflSize = pBt->usableSize - 4;  /* Bytes content per ovfl page */
    Pgno nextPage;

    nextPage = get4byte(&aPayload[pCur->info.nLocal]);

    /* If the BtCursor.aOverflow[] has not been allocated, allocate it now.
    **
    ** The aOverflow[] array is sized at one entry for each overflow page
    ** in the overflow chain. The page number of the first overflow page is
    ** stored in aOverflow[0], etc. A value of 0 in the aOverflow[] array
    ** means "not yet known" (the cache is lazily populated).
    */
    if( (pCur->curFlags & BTCF_ValidOvfl)==0 ){
      i64 nOvfl = pCur->info.nPayload;
      testcase( nOvfl - pCur->info.nLocal + ovflSize - 1 > 0xffffffffU );
      nOvfl = (nOvfl - pCur->info.nLocal + ovflSize-1)/ovflSize;
      if( pCur->aOverflow==0
       || nOvfl*(int)sizeof(Pgno) > sqlite3MallocSize(pCur->aOverflow)
      ){
        Pgno *aNew;
        if( sqlite3FaultSim(413) ){
          aNew = 0;
        }else{
          aNew = (Pgno*)sqlite3Realloc(pCur->aOverflow, nOvfl*2*sizeof(Pgno));
        }
        if( aNew==0 ){
          return SQLITE_NOMEM_BKPT;
        }else{
          pCur->aOverflow = aNew;
        }
      }
      memset(pCur->aOverflow, 0, nOvfl*sizeof(Pgno));
      pCur->curFlags |= BTCF_ValidOvfl;
    }else{
      /* Sanity check the validity of the overflow page cache */
      assert( pCur->aOverflow[0]==nextPage
           || pCur->aOverflow[0]==0
           || CORRUPT_DB );
      assert( pCur->aOverflow[0]!=0 || pCur->aOverflow[offset/ovflSize]==0 );

      /* If the overflow page-list cache has been allocated and the
      ** entry for the first required overflow page is valid, skip
      ** directly to it.
      */
      if( pCur->aOverflow[offset/ovflSize] ){
        iIdx = (offset/ovflSize);
        nextPage = pCur->aOverflow[iIdx];
        offset = (offset%ovflSize);
      }
    }

    assert( rc==SQLITE_OK && amt>0 );
    while( nextPage ){
      /* If required, populate the overflow page-list cache. */
      if( nextPage > pBt->nPage ) return SQLITE_CORRUPT_BKPT;
      assert( pCur->aOverflow[iIdx]==0
              || pCur->aOverflow[iIdx]==nextPage
              || CORRUPT_DB );
      pCur->aOverflow[iIdx] = nextPage;

      if( offset>=ovflSize ){
        /* The only reason to read this page is to obtain the page
        ** number for the next page in the overflow chain. The page
        ** data is not required. So first try to lookup the overflow
        ** page-list cache, if any, then fall back to the getOverflowPage()
        ** function.
        */
        assert( pCur->curFlags & BTCF_ValidOvfl );
        assert( pCur->pBtree->db==pBt->db );
        if( pCur->aOverflow[iIdx+1] ){
          nextPage = pCur->aOverflow[iIdx+1];
        }else{
          rc = getOverflowPage(pBt, nextPage, 0, &nextPage);
        }
        offset -= ovflSize;
      }else{
        /* Need to read this page properly. It contains some of the
        ** range of data that is being read (eOp==0) or written (eOp!=0).
        */
        int a = amt;
        if( a + offset > ovflSize ){
          a = ovflSize - offset;
        }

#ifdef SQLITE_DIRECT_OVERFLOW_READ
        /* If all the following are true:
        **
        **   1) this is a read operation, and
        **   2) data is required from the start of this overflow page, and
        **   3) there are no dirty pages in the page-cache
        **   4) the database is file-backed, and
        **   5) the page is not in the WAL file
        **   6) at least 4 bytes have already been read into the output buffer
        **
        ** then data can be read directly from the database file into the
        ** output buffer, bypassing the page-cache altogether. This speeds
        ** up loading large records that span many overflow pages.
        */
        if( eOp==0                                             /* (1) */
         && offset==0                                          /* (2) */
         && sqlite3PagerDirectReadOk(pBt->pPager, nextPage)    /* (3,4,5) */
         && &pBuf[-4]>=pBufStart                               /* (6) */
        ){
          sqlite3_file *fd = sqlite3PagerFile(pBt->pPager);
          u8 aSave[4];
          u8 *aWrite = &pBuf[-4];
          assert( aWrite>=pBufStart );                         /* due to (6) */
          memcpy(aSave, aWrite, 4);
          rc = sqlite3OsRead(fd, aWrite, a+4, (i64)pBt->pageSize*(nextPage-1));
          nextPage = get4byte(aWrite);
          memcpy(aWrite, aSave, 4);
        }else
#endif

        {
          DbPage *pDbPage;
          rc = sqlite3PagerGet(pBt->pPager, nextPage, &pDbPage,
              (eOp==0 ? PAGER_GET_READONLY : 0)
          );
          if( rc==SQLITE_OK ){
            if( eOp!=0
             && (sqlite3PagerPageRefcount(pDbPage)!=1
                 || NEVER(((MemPage*)sqlite3PagerGetExtra(pDbPage))->isInit))
             && sqlite3FaultSim(411)==SQLITE_OK
            ){
              sqlite3PagerUnref(pDbPage);
              return SQLITE_CORRUPT_PAGE(pPage);
            }
            aPayload = sqlite3PagerGetData(pDbPage);
            nextPage = get4byte(aPayload);
            rc = copyPayload(&aPayload[offset+4], pBuf, a, eOp, pDbPage);
            sqlite3PagerUnref(pDbPage);
            offset = 0;
          }
        }
        amt -= a;
        if( amt==0 ) return rc;
        pBuf += a;
      }
      if( rc ) break;
      iIdx++;
    }
  }

  if( rc==SQLITE_OK && amt>0 ){
    /* Overflow chain ends prematurely */
    return SQLITE_CORRUPT_PAGE(pPage);
  }
  return rc;
}
/* …（同文件无关代码省略）… */
static SQLITE_NOINLINE int accessPayloadChecked(
  BtCursor *pCur,
  u32 offset,
  u32 amt,
  void *pBuf
){
  int rc;
  if ( pCur->eState==CURSOR_INVALID ){
    return SQLITE_ABORT;
  }
  assert( cursorOwnsBtShared(pCur) );
  rc = btreeRestoreCursorPosition(pCur);
  return rc ? rc : accessPayload(pCur, offset, amt, pBuf, 0);
}
int sqlite3BtreePayloadChecked(BtCursor *pCur, u32 offset, u32 amt, void *pBuf){
  if( pCur->eState==CURSOR_VALID ){
    assert( cursorOwnsBtShared(pCur) );
    return accessPayload(pCur, offset, amt, pBuf, 0);
  }else{
    return accessPayloadChecked(pCur, offset, amt, pBuf);
  }
}
#endif /* SQLITE_OMIT_INCRBLOB */

/* …（同文件无关代码省略）… */
static int moveToChild(BtCursor *pCur, u32 newPgno){
  int rc;
  assert( cursorOwnsBtShared(pCur) );
  assert( pCur->eState==CURSOR_VALID );
  assert( pCur->iPage<BTCURSOR_MAX_DEPTH );
  assert( pCur->iPage>=0 );
  if( pCur->iPage>=(BTCURSOR_MAX_DEPTH-1) ){
    return SQLITE_CORRUPT_BKPT;
  }
  pCur->info.nSize = 0;
  pCur->curFlags &= ~(BTCF_ValidNKey|BTCF_ValidOvfl);
  pCur->aiIdx[pCur->iPage] = pCur->ix;
  pCur->apPage[pCur->iPage] = pCur->pPage;
  pCur->ix = 0;
  pCur->iPage++;
  rc = getAndInitPage(pCur->pBt, newPgno, &pCur->pPage, pCur->curPagerFlags);
  assert( pCur->pPage!=0 || rc!=SQLITE_OK );
  if( rc==SQLITE_OK
   && (pCur->pPage->nCell<1 || pCur->pPage->intKey!=pCur->curIntKey)
  ){
    releasePage(pCur->pPage);
    rc = SQLITE_CORRUPT_PGNO(newPgno);
  }
  if( rc ){
    pCur->pPage = pCur->apPage[--pCur->iPage];
  }
  return rc;
}
/* …（同文件无关代码省略）… */
static int moveToRoot(BtCursor *pCur){
  MemPage *pRoot;
  int rc = SQLITE_OK;

  assert( cursorOwnsBtShared(pCur) );
  assert( CURSOR_INVALID < CURSOR_REQUIRESEEK );
  assert( CURSOR_VALID   < CURSOR_REQUIRESEEK );
  assert( CURSOR_FAULT   > CURSOR_REQUIRESEEK );
  assert( pCur->eState < CURSOR_REQUIRESEEK || pCur->iPage<0 );
  assert( pCur->pgnoRoot>0 || pCur->iPage<0 );

  if( pCur->iPage>=0 ){
    if( pCur->iPage ){
      releasePageNotNull(pCur->pPage);
      while( --pCur->iPage ){
        releasePageNotNull(pCur->apPage[pCur->iPage]);
      }
      pRoot = pCur->pPage = pCur->apPage[0];
      goto skip_init;
    }
  }else if( pCur->pgnoRoot==0 ){
    pCur->eState = CURSOR_INVALID;
    return SQLITE_EMPTY;
  }else{
    assert( pCur->iPage==(-1) );
    if( pCur->eState>=CURSOR_REQUIRESEEK ){
      if( pCur->eState==CURSOR_FAULT ){
        assert( pCur->skipNext!=SQLITE_OK );
        return pCur->skipNext;
      }
      sqlite3BtreeClearCursor(pCur);
    }
    rc = getAndInitPage(pCur->pBt, pCur->pgnoRoot, &pCur->pPage,
                        pCur->curPagerFlags);
    if( rc!=SQLITE_OK ){
      pCur->eState = CURSOR_INVALID;
      return rc;
    }
    pCur->iPage = 0;
    pCur->curIntKey = pCur->pPage->intKey;
  }
  pRoot = pCur->pPage;
  assert( pRoot->pgno==pCur->pgnoRoot || CORRUPT_DB );

  /* If pCur->pKeyInfo is not NULL, then the caller that opened this cursor
  ** expected to open it on an index b-tree. Otherwise, if pKeyInfo is
  ** NULL, the caller expects a table b-tree. If this is not the case,
  ** return an SQLITE_CORRUPT error.
  **
  ** Earlier versions of SQLite assumed that this test could not fail
  ** if the root page was already loaded when this function was called (i.e.
  ** if pCur->iPage>=0). But this is not so if the database is corrupted
  ** in such a way that page pRoot is linked into a second b-tree table
  ** (or the freelist).  */
  assert( pRoot->intKey==1 || pRoot->intKey==0 );
  if( pRoot->isInit==0 || (pCur->pKeyInfo==0)!=pRoot->intKey ){
    return SQLITE_CORRUPT_PAGE(pCur->pPage);
  }

skip_init: 
  pCur->ix = 0;
  pCur->info.nSize = 0;
  pCur->curFlags &= ~(BTCF_AtLast|BTCF_ValidNKey|BTCF_ValidOvfl);

  if( pRoot->nCell>0 ){
    pCur->eState = CURSOR_VALID;
  }else if( !pRoot->leaf ){
    Pgno subpage;
    if( pRoot->pgno!=1 ) return SQLITE_CORRUPT_BKPT;
    subpage = get4byte(&pRoot->aData[pRoot->hdrOffset+8]);
    pCur->eState = CURSOR_VALID;
    rc = moveToChild(pCur, subpage);
  }else{
    pCur->eState = CURSOR_INVALID;
    rc = SQLITE_EMPTY;
  }
  return rc;
}
/* …（同文件无关代码省略）… */
static int moveToLeftmost(BtCursor *pCur){
  Pgno pgno;
  int rc = SQLITE_OK;
  MemPage *pPage;

  assert( cursorOwnsBtShared(pCur) );
  assert( pCur->eState==CURSOR_VALID );
  while( rc==SQLITE_OK && !(pPage = pCur->pPage)->leaf ){
    assert( pCur->ix<pPage->nCell );
    pgno = get4byte(findCell(pPage, pCur->ix));
    rc = moveToChild(pCur, pgno);
  }
  return rc;
}
/* …（同文件无关代码省略）… */
static int cursorIsAtLastEntry(BtCursor *pCur){
  int ii;
  for(ii=0; ii<pCur->iPage; ii++){
    if( pCur->aiIdx[ii]!=pCur->apPage[ii]->nCell ) return 0;
  }
  return pCur->ix==pCur->pPage->nCell-1 && pCur->pPage->leaf!=0;
}
/* …（同文件无关代码省略）… */
int sqlite3BtreeTableMoveto(
  BtCursor *pCur,          /* The cursor to be moved */
  i64 intKey,              /* The table key */
  int biasRight,           /* If true, bias the search to the high end */
  int *pRes                /* Write search results here */
){
  int rc;

  assert( cursorOwnsBtShared(pCur) );
  assert( sqlite3_mutex_held(pCur->pBtree->db->mutex) );
  assert( pRes );
  assert( pCur->pKeyInfo==0 );
  assert( pCur->eState!=CURSOR_VALID || pCur->curIntKey!=0 );

  /* If the cursor is already positioned at the point we are trying
  ** to move to, then just return without doing any work */
  if( pCur->eState==CURSOR_VALID && (pCur->curFlags & BTCF_ValidNKey)!=0 ){
    if( pCur->info.nKey==intKey ){
      *pRes = 0;
      return SQLITE_OK;
    }
    if( pCur->info.nKey<intKey ){
      if( (pCur->curFlags & BTCF_AtLast)!=0 ){
        assert( cursorIsAtLastEntry(pCur) || CORRUPT_DB );
        *pRes = -1;
        return SQLITE_OK;
      }
      /* If the requested key is one more than the previous key, then
      ** try to get there using sqlite3BtreeNext() rather than a full
      ** binary search.  This is an optimization only.  The correct answer
      ** is still obtained without this case, only a little more slowly. */
      if( pCur->info.nKey+1==intKey ){
        *pRes = 0;
        rc = sqlite3BtreeNext(pCur, 0);
        if( rc==SQLITE_OK ){
          getCellInfo(pCur);
          if( pCur->info.nKey==intKey ){
            return SQLITE_OK;
          }
        }else if( rc!=SQLITE_DONE ){
          return rc;
        }
      }
    }
  }

#ifdef SQLITE_DEBUG
  pCur->pBtree->nSeek++;   /* Performance measurement during testing */
#endif

  rc = moveToRoot(pCur);
  if( rc ){
    if( rc==SQLITE_EMPTY ){
      assert( pCur->pgnoRoot==0 || pCur->pPage->nCell==0 );
      *pRes = -1;
      return SQLITE_OK;
    }
    return rc;
  }
  assert( pCur->pPage );
  assert( pCur->pPage->isInit );
  assert( pCur->eState==CURSOR_VALID );
  assert( pCur->pPage->nCell > 0 );
  assert( pCur->iPage==0 || pCur->apPage[0]->intKey==pCur->curIntKey );
  assert( pCur->curIntKey );

  for(;;){
    int lwr, upr, idx, c;
    Pgno chldPg;
    MemPage *pPage = pCur->pPage;
    u8 *pCell;                          /* Pointer to current cell in pPage */

    /* pPage->nCell must be greater than zero. If this is the root-page
    ** the cursor would have been INVALID above and this for(;;) loop
    ** not run. If this is not the root-page, then the moveToChild() routine
    ** would have already detected db corruption. Similarly, pPage must
    ** be the right kind (index or table) of b-tree page. Otherwise
    ** a moveToChild() or moveToRoot() call would have detected corruption.  */
    assert( pPage->nCell>0 );
    assert( pPage->intKey );
    lwr = 0;
    upr = pPage->nCell-1;
    assert( biasRight==0 || biasRight==1 );
    idx = upr>>(1-biasRight); /* idx = biasRight ? upr : (lwr+upr)/2; */
    for(;;){
      i64 nCellKey;
      pCell = findCellPastPtr(pPage, idx);
      if( pPage->intKeyLeaf ){
        while( 0x80 <= *(pCell++) ){
          if( pCell>=pPage->aDataEnd ){
            return SQLITE_CORRUPT_PAGE(pPage);
          }
        }
      }
      nCellKey = sqlite3VarintValue(pCell);
      if( nCellKey<intKey ){
        lwr = idx+1;
        if( lwr>upr ){ c = -1; break; }
      }else if( nCellKey>intKey ){
        upr = idx-1;
        if( lwr>upr ){ c = +1; break; }
      }else{
        assert( nCellKey==intKey );
        pCur->ix = (u16)idx;
        if( !pPage->leaf ){
          lwr = idx;
          goto moveto_table_next_layer;
        }else{
          pCur->curFlags |= BTCF_ValidNKey;
          pCur->info.nKey = nCellKey;
          pCur->info.nSize = 0;
          *pRes = 0;
          return SQLITE_OK;
        }
      }
      assert( lwr+upr>=0 );
      idx = (lwr+upr)>>1;  /* idx = (lwr+upr)/2; */
    }
    assert( lwr==upr+1 || !pPage->leaf );
    assert( pPage->isInit );
    if( pPage->leaf ){
      assert( pCur->ix<pCur->pPage->nCell );
      pCur->ix = (u16)idx;
      *pRes = c;
      rc = SQLITE_OK;
      goto moveto_table_finish;
    }
moveto_table_next_layer:
    if( lwr>=pPage->nCell ){
      chldPg = get4byte(&pPage->aData[pPage->hdrOffset+8]);
    }else{
      chldPg = get4byte(findCell(pPage, lwr));
    }
    pCur->ix = (u16)lwr;
    rc = moveToChild(pCur, chldPg);
    if( rc ) break;
  }
moveto_table_finish:
  pCur->info.nSize = 0;
  assert( (pCur->curFlags & BTCF_ValidOvfl)==0 );
  return rc;
}
/* …（同文件无关代码省略）… */
static int indexCellCompare(
  MemPage *pPage,
  int idx,
  UnpackedRecord *pIdxKey,
  RecordCompare xRecordCompare
){
  int c;
  int nCell;  /* Size of the pCell cell in bytes */
  u8 *pCell = findCellPastPtr(pPage, idx);

  nCell = pCell[0];
  if( nCell<=pPage->max1bytePayload ){
    /* This branch runs if the record-size field of the cell is a
    ** single byte varint and the record fits entirely on the main
    ** b-tree page.  */
    if( pCell + nCell >= pPage->aDataEnd ) return 99;
    c = xRecordCompare(nCell, (void*)&pCell[1], pIdxKey);
  }else if( !(pCell[1] & 0x80)
    && (nCell = ((nCell&0x7f)<<7) + pCell[1])<=pPage->maxLocal
  ){
    /* The record-size field is a 2 byte varint and the record
    ** fits entirely on the main b-tree page.  */
    if( pCell + nCell >= pPage->aDataEnd ) return 99;
    c = xRecordCompare(nCell, (void*)&pCell[2], pIdxKey);
  }else{
    /* If the record extends into overflow pages, do not attempt
    ** the optimization. */
    c = 99;
  }
  return c;
}
/* …（同文件无关代码省略）… */
static int cursorOnLastPage(BtCursor *pCur){
  int i;
  assert( pCur->eState==CURSOR_VALID );
  for(i=0; i<pCur->iPage; i++){
    MemPage *pPage = pCur->apPage[i];
    if( pCur->aiIdx[i]<pPage->nCell ) return 0;
  }
  return 1;
}
/* …（同文件无关代码省略）… */
int sqlite3BtreeIndexMoveto(
  BtCursor *pCur,          /* The cursor to be moved */
  UnpackedRecord *pIdxKey, /* Unpacked index key */
  int *pRes                /* Write search results here */
){
  int rc;
  RecordCompare xRecordCompare;

  assert( cursorOwnsBtShared(pCur) );
  assert( sqlite3_mutex_held(pCur->pBtree->db->mutex) );
  assert( pRes );
  assert( pCur->pKeyInfo!=0 );

#ifdef SQLITE_DEBUG
  pCur->pBtree->nSeek++;   /* Performance measurement during testing */
#endif

  xRecordCompare = sqlite3VdbeFindCompare(pIdxKey);
  pIdxKey->errCode = 0;
  assert( pIdxKey->default_rc==1
       || pIdxKey->default_rc==0
       || pIdxKey->default_rc==-1
  );


  /* Check to see if we can skip a lot of work.  Two cases:
  **
  **    (1) If the cursor is already pointing to the very last cell
  **        in the table and the pIdxKey search key is greater than or
  **        equal to that last cell, then no movement is required.
  **
  **    (2) If the cursor is on the last page of the table and the first
  **        cell on that last page is less than or equal to the pIdxKey
  **        search key, then we can start the search on the current page
  **        without needing to go back to root.
  */
  if( pCur->eState==CURSOR_VALID
   && pCur->pPage->leaf
   && cursorOnLastPage(pCur)
  ){
    int c;
    if( pCur->ix==pCur->pPage->nCell-1
     && (c = indexCellCompare(pCur->pPage,pCur->ix,pIdxKey,xRecordCompare))<=0
     && pIdxKey->errCode==SQLITE_OK
    ){
      *pRes = c;
      return SQLITE_OK;  /* Cursor already pointing at the correct spot */
    }
    if( pCur->iPage>0
     && indexCellCompare(pCur->pPage, 0, pIdxKey, xRecordCompare)<=0
     && pIdxKey->errCode==SQLITE_OK
    ){
      pCur->curFlags &= ~(BTCF_ValidOvfl|BTCF_AtLast);
      if( !pCur->pPage->isInit ){
        return SQLITE_CORRUPT_BKPT;
      }
      goto bypass_moveto_root;  /* Start search on the current page */
    }
    pIdxKey->errCode = SQLITE_OK;
  }

  rc = moveToRoot(pCur);
  if( rc ){
    if( rc==SQLITE_EMPTY ){
      assert( pCur->pgnoRoot==0 || pCur->pPage->nCell==0 );
      *pRes = -1;
      return SQLITE_OK;
    }
    return rc;
  }

bypass_moveto_root:
  assert( pCur->pPage );
  assert( pCur->pPage->isInit );
  assert( pCur->eState==CURSOR_VALID );
  assert( pCur->pPage->nCell > 0 );
  assert( pCur->curIntKey==0 );
  assert( pIdxKey!=0 );
  for(;;){
    int lwr, upr, idx, c;
    Pgno chldPg;
    MemPage *pPage = pCur->pPage;
    u8 *pCell;                          /* Pointer to current cell in pPage */

    /* pPage->nCell must be greater than zero. If this is the root-page
    ** the cursor would have been INVALID above and this for(;;) loop
    ** not run. If this is not the root-page, then the moveToChild() routine
    ** would have already detected db corruption. Similarly, pPage must
    ** be the right kind (index or table) of b-tree page. Otherwise
    ** a moveToChild() or moveToRoot() call would have detected corruption.  */
    assert( pPage->nCell>0 );
    assert( pPage->intKey==0 );
    lwr = 0;
    upr = pPage->nCell-1;
    idx = upr>>1; /* idx = (lwr+upr)/2; */
    for(;;){
      int nCell;  /* Size of the pCell cell in bytes */
      pCell = findCellPastPtr(pPage, idx);

      /* The maximum supported page-size is 65536 bytes. This means that
      ** the maximum number of record bytes stored on an index B-Tree
      ** page is less than 16384 bytes and may be stored as a 2-byte
      ** varint. This information is used to attempt to avoid parsing
      ** the entire cell by checking for the cases where the record is
      ** stored entirely within the b-tree page by inspecting the first
      ** 2 bytes of the cell.
      */
      nCell = pCell[0];
      if( nCell<=pPage->max1bytePayload ){
        /* This branch runs if the record-size field of the cell is a
        ** single byte varint and the record fits entirely on the main
        ** b-tree page.  */
        if( pCell + nCell >= pPage->aDataEnd ){
          rc = SQLITE_CORRUPT_PAGE(pPage);
          goto moveto_index_finish;
        }
        c = xRecordCompare(nCell, (void*)&pCell[1], pIdxKey);
      }else if( !(pCell[1] & 0x80)
        && (nCell = ((nCell&0x7f)<<7) + pCell[1])<=pPage->maxLocal
        && pCell + nCell < pPage->aDataEnd
      ){
        /* The record-size field is a 2 byte varint and the record
        ** fits entirely on the main b-tree page.  */
        c = xRecordCompare(nCell, (void*)&pCell[2], pIdxKey);
      }else{
        /* The record flows over onto one or more overflow pages. In
        ** this case the whole cell needs to be parsed, a buffer allocated
        ** and accessPayload() used to retrieve the record into the
        ** buffer before VdbeRecordCompare() can be called.
        **
        ** If the record is corrupt, the xRecordCompare routine may read
        ** up to two varints past the end of the buffer. An extra 18
        ** bytes of padding is allocated at the end of the buffer in
        ** case this happens.  */
        void *pCellKey;
        u8 * const pCellBody = pCell - pPage->childPtrSize;
        const int nOverrun = 18;  /* Size of the overrun padding */
        pPage->xParseCell(pPage, pCellBody, &pCur->info);
        nCell = (int)pCur->info.nKey;
        testcase( nCell<0 );   /* True if key size is 2^32 or more */
        testcase( nCell==0 );  /* Invalid key size:  0x80 0x80 0x00 */
        testcase( nCell==1 );  /* Invalid key size:  0x80 0x80 0x01 */
        testcase( nCell==2 );  /* Minimum legal index key size */
        if( nCell<2 || nCell/pCur->pBt->usableSize>pCur->pBt->nPage ){
          rc = SQLITE_CORRUPT_PAGE(pPage);
          goto moveto_index_finish;
        }
        pCellKey = sqlite3Malloc( (u64)nCell+(u64)nOverrun );
        if( pCellKey==0 ){
          rc = SQLITE_NOMEM_BKPT;
          goto moveto_index_finish;
        }
        pCur->ix = (u16)idx;
        rc = accessPayload(pCur, 0, nCell, (unsigned char*)pCellKey, 0);
        memset(((u8*)pCellKey)+nCell,0,nOverrun); /* Fix uninit warnings */
        pCur->curFlags &= ~BTCF_ValidOvfl;
        if( rc ){
          sqlite3_free(pCellKey);
          goto moveto_index_finish;
        }
        c = sqlite3VdbeRecordCompare(nCell, pCellKey, pIdxKey);
        sqlite3_free(pCellKey);
      }
      assert(
          (pIdxKey->errCode!=SQLITE_CORRUPT || c==0)
       && (pIdxKey->errCode!=SQLITE_NOMEM || pCur->pBtree->db->mallocFailed)
      );
      if( c<0 ){
        lwr = idx+1;
      }else if( c>0 ){
        upr = idx-1;
      }else{
        assert( c==0 );
        *pRes = 0;
        rc = SQLITE_OK;
        pCur->ix = (u16)idx;
        if( pIdxKey->errCode ) rc = SQLITE_CORRUPT_BKPT;
        goto moveto_index_finish;
      }
      if( lwr>upr ) break;
      assert( lwr+upr>=0 );
      idx = (lwr+upr)>>1;  /* idx = (lwr+upr)/2 */
    }
    assert( lwr==upr+1 || (pPage->intKey && !pPage->leaf) );
    assert( pPage->isInit );
    if( pPage->leaf ){
      assert( pCur->ix<pCur->pPage->nCell || CORRUPT_DB );
      pCur->ix = (u16)idx;
      *pRes = c;
      rc = SQLITE_OK;
      goto moveto_index_finish;
    }
    if( lwr>=pPage->nCell ){
      chldPg = get4byte(&pPage->aData[pPage->hdrOffset+8]);
    }else{
      chldPg = get4byte(findCell(pPage, lwr));
    }

    /* This block is similar to an in-lined version of:
    **
    **    pCur->ix = (u16)lwr;
    **    rc = moveToChild(pCur, chldPg);
    **    if( rc ) break;
    */
    pCur->info.nSize = 0;
    pCur->curFlags &= ~(BTCF_ValidNKey|BTCF_ValidOvfl);
    if( pCur->iPage>=(BTCURSOR_MAX_DEPTH-1) ){
      return SQLITE_CORRUPT_BKPT;
    }
    pCur->aiIdx[pCur->iPage] = (u16)lwr;
    pCur->apPage[pCur->iPage] = pCur->pPage;
    pCur->ix = 0;
    pCur->iPage++;
    rc = getAndInitPage(pCur->pBt, chldPg, &pCur->pPage, pCur->curPagerFlags);
    if( rc==SQLITE_OK
     && (pCur->pPage->nCell<1 || pCur->pPage->intKey!=pCur->curIntKey)
    ){
      releasePage(pCur->pPage);
      rc = SQLITE_CORRUPT_PGNO(chldPg);
    }
    if( rc ){
      pCur->pPage = pCur->apPage[--pCur->iPage];
      break;
    }
    /*
    ***** End of in-lined moveToChild() call */
 }
moveto_index_finish:
  pCur->info.nSize = 0;
  assert( (pCur->curFlags & BTCF_ValidOvfl)==0 );
  return rc;
}
/* …（同文件无关代码省略）… */
static SQLITE_NOINLINE int btreeNext(BtCursor *pCur){
  int rc;
  int idx;
  MemPage *pPage;

  assert( cursorOwnsBtShared(pCur) );
  if( pCur->eState!=CURSOR_VALID ){
    assert( (pCur->curFlags & BTCF_ValidOvfl)==0 );
    rc = restoreCursorPosition(pCur);
    if( rc!=SQLITE_OK ){
      return rc;
    }
    if( CURSOR_INVALID==pCur->eState ){
      return SQLITE_DONE;
    }
    if( pCur->eState==CURSOR_SKIPNEXT ){
      pCur->eState = CURSOR_VALID;
      if( pCur->skipNext>0 ) return SQLITE_OK;
    }
  }

  pPage = pCur->pPage;
  idx = ++pCur->ix;
  if( sqlite3FaultSim(412) ) pPage->isInit = 0;
  if( !pPage->isInit ){
    return SQLITE_CORRUPT_BKPT;
  }

  if( idx>=pPage->nCell ){
    if( !pPage->leaf ){
      rc = moveToChild(pCur, get4byte(&pPage->aData[pPage->hdrOffset+8]));
      if( rc ) return rc;
      return moveToLeftmost(pCur);
    }
    do{
      if( pCur->iPage==0 ){
        pCur->eState = CURSOR_INVALID;
        return SQLITE_DONE;
      }
      moveToParent(pCur);
      pPage = pCur->pPage;
    }while( pCur->ix>=pPage->nCell );
    if( pPage->intKey ){
      return sqlite3BtreeNext(pCur, 0);
    }else{
      return SQLITE_OK;
    }
  }
  if( pPage->leaf ){
    return SQLITE_OK;
  }else{
    return moveToLeftmost(pCur);
  }
}
int sqlite3BtreeNext(BtCursor *pCur, int flags){
  MemPage *pPage;
  UNUSED_PARAMETER( flags );  /* Used in COMDB2 but not native SQLite */
  assert( cursorOwnsBtShared(pCur) );
  assert( flags==0 || flags==1 );
  pCur->info.nSize = 0;
  pCur->curFlags &= ~(BTCF_ValidNKey|BTCF_ValidOvfl);
  if( pCur->eState!=CURSOR_VALID ) return btreeNext(pCur);
  pPage = pCur->pPage;
  if( (++pCur->ix)>=pPage->nCell ){
    pCur->ix--;
    return btreeNext(pCur);
  }
  if( pPage->leaf ){
    return SQLITE_OK;
  }else{
    return moveToLeftmost(pCur);
  }
}
