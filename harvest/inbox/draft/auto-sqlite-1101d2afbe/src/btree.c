// AUTO-DRAFT from sqlite/sqlite PR #bbe31d3b55e953271106851216fd993e800797ed
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
  // <<< BUG ANCHOR
# define TRACE(X)  if(sqlite3BtreeTrace){printf X;fflush(stdout);}
/* …（同文件无关代码省略）… */
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
static int freeSpace(MemPage *pPage, int iStart, int iSize){
  int iPtr;                             /* Address of ptr to next freeblock */
  int iFreeBlk;                         /* Address of the next freeblock */
  u8 hdr;                               /* Page header size.  0 or 100 */
  int nFrag = 0;                        /* Reduction in fragmentation */
  int iOrigSize = iSize;                /* Original value of iSize */
  int x;                                /* Offset to cell content area */
  int iEnd = iStart + iSize;            /* First byte past the iStart buffer */
  unsigned char *data = pPage->aData;   /* Page content */
  u8 *pTmp;                             /* Temporary ptr into data[] */

  assert( pPage->pBt!=0 );
  assert( sqlite3PagerIswriteable(pPage->pDbPage) );
  assert( CORRUPT_DB || iStart>=pPage->hdrOffset+6+pPage->childPtrSize );
  assert( CORRUPT_DB || iEnd <= (int)pPage->pBt->usableSize );
  assert( sqlite3_mutex_held(pPage->pBt->mutex) );
  assert( iSize>=4 );   /* Minimum cell size is 4 */
  assert( CORRUPT_DB || iStart<=(int)pPage->pBt->usableSize-4 );

  /* The list of freeblocks must be in ascending order.  Find the
  ** spot on the list where iStart should be inserted.
  */
  hdr = pPage->hdrOffset;
  iPtr = hdr + 1;
  if( data[iPtr+1]==0 && data[iPtr]==0 ){
    iFreeBlk = 0;  /* Shortcut for the case when the freelist is empty */
  }else{
    while( (iFreeBlk = get2byte(&data[iPtr]))<iStart ){
      if( iFreeBlk<=iPtr ){
        if( iFreeBlk==0 ) break; /* TH3: corrupt082.100 */
        return SQLITE_CORRUPT_PAGE(pPage);
      }
      iPtr = iFreeBlk;
    }
    if( iFreeBlk>(int)pPage->pBt->usableSize-4 ){ /* TH3: corrupt081.100 */
      return SQLITE_CORRUPT_PAGE(pPage);
    }
    assert( iFreeBlk>iPtr || iFreeBlk==0 || CORRUPT_DB );
 
    /* At this point:
    **    iFreeBlk:   First freeblock after iStart, or zero if none
    **    iPtr:       The address of a pointer to iFreeBlk
    **
    ** Check to see if iFreeBlk should be coalesced onto the end of iStart.
    */
    if( iFreeBlk && iEnd+3>=iFreeBlk ){
      nFrag = iFreeBlk - iEnd;
      if( iEnd>iFreeBlk ) return SQLITE_CORRUPT_PAGE(pPage);
      iEnd = iFreeBlk + get2byte(&data[iFreeBlk+2]);
      if( iEnd > (int)pPage->pBt->usableSize ){
        return SQLITE_CORRUPT_PAGE(pPage);
      }
      iSize = iEnd - iStart;
      iFreeBlk = get2byte(&data[iFreeBlk]);
    }
 
    /* If iPtr is another freeblock (that is, if iPtr is not the freelist
    ** pointer in the page header) then check to see if iStart should be
    ** coalesced onto the end of iPtr.
    */
    if( iPtr>hdr+1 ){
      int iPtrEnd = iPtr + get2byte(&data[iPtr+2]);
      if( iPtrEnd+3>=iStart ){
        if( iPtrEnd>iStart ) return SQLITE_CORRUPT_PAGE(pPage);
        nFrag += iStart - iPtrEnd;
        iSize = iEnd - iPtr;
        iStart = iPtr;
      }
    }
    if( nFrag>data[hdr+7] ) return SQLITE_CORRUPT_PAGE(pPage);
    data[hdr+7] -= (u8)nFrag;
  }
  pTmp = &data[hdr+5];
  x = get2byte(pTmp);
  if( pPage->pBt->btsFlags & BTS_FAST_SECURE ){
    /* Overwrite deleted information with zeros when the secure_delete
    ** option is enabled */
    memset(&data[iStart], 0, iSize);
  }
  if( iStart<=x ){
    /* The new freeblock is at the beginning of the cell content area,
    ** so just extend the cell content area rather than create another
    ** freelist entry */
    if( iStart<x ) return SQLITE_CORRUPT_PAGE(pPage);
    if( iPtr!=hdr+1 ) return SQLITE_CORRUPT_PAGE(pPage);
    put2byte(&data[hdr+1], iFreeBlk);
    put2byte(&data[hdr+5], iEnd);
  }else{
    /* Insert the new freeblock into the freelist */
    put2byte(&data[iPtr], iStart);
    put2byte(&data[iStart], iFreeBlk);
    assert( iSize>=0 && iSize<=0xffff );
    put2byte(&data[iStart+2], (u16)iSize);
  }
  pPage->nFree += iOrigSize;
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
    if( rc!=SQLITE_OK ){
      releasePage(*ppPage);
      *ppPage = 0;
    }
    TRACE(("ALLOCATE: %u from end of file\n", *pPgno));
  }
/* …（同文件无关代码省略）… */
typedef struct CellArray CellArray;
/* …（同文件无关代码省略）… */
static int pageFreeArray(
  MemPage *pPg,                   /* Page to edit */
  int iFirst,                     /* First cell to delete */
  int nCell,                      /* Cells to delete */
  CellArray *pCArray              /* Array of cells */
){
  u8 * const aData = pPg->aData;
  u8 * const pEnd = &aData[pPg->pBt->usableSize];
  u8 * const pStart = &aData[pPg->hdrOffset + 8 + pPg->childPtrSize];
  int nRet = 0;
  int i, j;
  int iEnd = iFirst + nCell;
  int nFree = 0;
  int aOfst[10];
  int aAfter[10];

  for(i=iFirst; i<iEnd; i++){
    u8 *pCell = pCArray->apCell[i];
    if( SQLITE_WITHIN(pCell, pStart, pEnd) ){
      int sz;
      int iAfter;
      int iOfst;
      /* No need to use cachedCellSize() here.  The sizes of all cells that
      ** are to be freed have already been computing while deciding which
      ** cells need freeing */
      sz = pCArray->szCell[i];  assert( sz>0 );
      iOfst = (u16)(pCell - aData);
      iAfter = iOfst+sz;
      for(j=0; j<nFree; j++){
        if( aOfst[j]==iAfter ){
          aOfst[j] = iOfst;
          break;
        }else if( aAfter[j]==iOfst ){
          aAfter[j] = iAfter;
          break;
        }
      }
      if( j>=nFree ){
        if( nFree>=(int)(sizeof(aOfst)/sizeof(aOfst[0])) ){
          for(j=0; j<nFree; j++){
            freeSpace(pPg, aOfst[j], aAfter[j]-aOfst[j]);
          }
          nFree = 0;
        }
        aOfst[nFree] = iOfst;
        aAfter[nFree] = iAfter;
        if( &aData[iAfter]>pEnd ) return 0;
        nFree++;
      }
      nRet++;
    }
  }
  for(j=0; j<nFree; j++){
    freeSpace(pPg, aOfst[j], aAfter[j]-aOfst[j]);
  }
  return nRet;
}
/* …（同文件无关代码省略）… */
  if( iOld<iNew ){
    int nShift = pageFreeArray(pPg, iOld, iNew-iOld, pCArray);
    if( NEVER(nShift>nCell) ) return SQLITE_CORRUPT_BKPT;
    memmove(pPg->aCellIdx, &pPg->aCellIdx[nShift*2], nCell*2);
    nCell -= nShift;
  }
  if( iNewEnd < iOldEnd ){
    int nTail = pageFreeArray(pPg, iNewEnd, iOldEnd - iNewEnd, pCArray);
