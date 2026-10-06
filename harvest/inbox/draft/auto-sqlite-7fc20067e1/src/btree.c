// AUTO-DRAFT from sqlite/sqlite PR #466e0851d9159ca355faa1ba63eb8cf6d11884e6
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define get2byteNotZero(X)  (((((int)get2byte(X))-1)&0xffff)+1)
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
  #define ptrmapPutOvflPtr(x, y, z, rc)
/* …（同文件无关代码省略）… */
#define findCell(P,I) \
  ((P)->aData + ((P)->maskPage & get2byteAligned(&(P)->aCellIdx[2*(I)])))
/* …（同文件无关代码省略）… */
static u16 cellSize(MemPage *pPage, int iCell){
  return pPage->xCellSize(pPage, findCell(pPage, iCell));
}
/* …（同文件无关代码省略）… */
static int defragmentPage(MemPage *pPage, int nMaxFrag){
  int i;                     /* Loop counter */
  int pc;                    /* Address of the i-th cell */
  int hdr;                   /* Offset to the page header */
  int size;                  /* Size of a cell */
  int usableSize;            /* Number of usable bytes on a page */
  int cellOffset;            /* Offset to the cell pointer array */
  int cbrk;                  /* Offset to the cell content area */
  int nCell;                 /* Number of cells on the page */
  unsigned char *data;       /* The page data */
  unsigned char *temp;       /* Temp area for cell content */
  unsigned char *src;        /* Source of content */
  int iCellFirst;            /* First allowable cell index */
  int iCellLast;             /* Last possible cell index */
  int iCellStart;            /* First cell offset in input */

  assert( sqlite3PagerIswriteable(pPage->pDbPage) );
  assert( pPage->pBt!=0 );
  assert( pPage->pBt->usableSize <= SQLITE_MAX_PAGE_SIZE );
  assert( pPage->nOverflow==0 );
  assert( sqlite3_mutex_held(pPage->pBt->mutex) );
  data = pPage->aData;
  hdr = pPage->hdrOffset;
  cellOffset = pPage->cellOffset;
  nCell = pPage->nCell;
  assert( nCell==get2byte(&data[hdr+3]) || CORRUPT_DB );
  iCellFirst = cellOffset + 2*nCell;
  usableSize = pPage->pBt->usableSize;

  /* This block handles pages with two or fewer free blocks and nMaxFrag
  ** or fewer fragmented bytes. In this case it is faster to move the
  ** two (or one) blocks of cells using memmove() and add the required
  ** offsets to each pointer in the cell-pointer array than it is to
  ** reconstruct the entire page.  */
  if( (int)data[hdr+7]<=nMaxFrag ){
    int iFree = get2byte(&data[hdr+1]);
    if( iFree>usableSize-4 ) return SQLITE_CORRUPT_PAGE(pPage);
    if( iFree ){
      int iFree2 = get2byte(&data[iFree]);
      if( iFree2>usableSize-4 ) return SQLITE_CORRUPT_PAGE(pPage);
      if( 0==iFree2 || (data[iFree2]==0 && data[iFree2+1]==0) ){
        u8 *pEnd = &data[cellOffset + nCell*2];
        u8 *pAddr;
        int sz2 = 0;
        int sz = get2byte(&data[iFree+2]);
        int top = get2byte(&data[hdr+5]);
        if( top>=iFree ){
          return SQLITE_CORRUPT_PAGE(pPage);
        }
        if( iFree2 ){
          if( iFree+sz>iFree2 ) return SQLITE_CORRUPT_PAGE(pPage);
          sz2 = get2byte(&data[iFree2+2]);
          if( iFree2+sz2 > usableSize ) return SQLITE_CORRUPT_PAGE(pPage);
          memmove(&data[iFree+sz+sz2], &data[iFree+sz], iFree2-(iFree+sz));
          sz += sz2;
        }else if( iFree+sz>usableSize ){
          return SQLITE_CORRUPT_PAGE(pPage);
        }

        cbrk = top+sz;
        assert( cbrk+(iFree-top) <= usableSize );
        memmove(&data[cbrk], &data[top], iFree-top);
        for(pAddr=&data[cellOffset]; pAddr<pEnd; pAddr+=2){
          pc = get2byte(pAddr);
          if( pc<iFree ){ put2byte(pAddr, pc+sz); }
          else if( pc<iFree2 ){ put2byte(pAddr, pc+sz2); }
        }
        goto defragment_out;
      }
    }
  }

  cbrk = usableSize;
  iCellLast = usableSize - 4;
  iCellStart = get2byte(&data[hdr+5]);
  if( nCell>0 ){
    temp = sqlite3PagerTempSpace(pPage->pBt->pPager);
    memcpy(temp, data, usableSize);
    src = temp;
    for(i=0; i<nCell; i++){
      u8 *pAddr;     /* The i-th cell pointer */
      pAddr = &data[cellOffset + i*2];
      pc = get2byte(pAddr);
      testcase( pc==iCellFirst );
      testcase( pc==iCellLast );
      /* These conditions have already been verified in btreeInitPage()
      ** if PRAGMA cell_size_check=ON.
      */
      if( pc>iCellLast ){
        return SQLITE_CORRUPT_PAGE(pPage);
      }
      assert( pc>=0 && pc<=iCellLast );
      size = pPage->xCellSize(pPage, &src[pc]);
      cbrk -= size;
      if( cbrk<iCellStart || pc+size>usableSize ){
        return SQLITE_CORRUPT_PAGE(pPage);
      }
      assert( cbrk+size<=usableSize && cbrk>=iCellStart );
      testcase( cbrk+size==usableSize );
      testcase( pc+size==usableSize );
      put2byte(pAddr, cbrk);
      memcpy(&data[cbrk], &src[pc], size);
    }
  }
  data[hdr+7] = 0;

defragment_out:
  assert( pPage->nFree>=0 );
  if( data[hdr+7]+cbrk-iCellFirst!=pPage->nFree ){
    return SQLITE_CORRUPT_PAGE(pPage);
  }
  assert( cbrk>=iCellFirst );
  put2byte(&data[hdr+5], cbrk);
  data[hdr+1] = 0;
  data[hdr+2] = 0;
  memset(&data[iCellFirst], 0, cbrk-iCellFirst);
  assert( sqlite3PagerIswriteable(pPage->pDbPage) );
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
static u8 *pageFindSlot(MemPage *pPg, int nByte, int *pRc){
  const int hdr = pPg->hdrOffset;            /* Offset to page header */
  u8 * const aData = pPg->aData;             /* Page data */
  int iAddr = hdr + 1;                       /* Address of ptr to pc */
  u8 *pTmp = &aData[iAddr];                  /* Temporary ptr into aData[] */
  int pc = get2byte(pTmp);                   /* Address of a free slot */
  int x;                                     /* Excess size of the slot */
  int maxPC = pPg->pBt->usableSize - nByte;  /* Max address for a usable slot */
  int size;                                  /* Size of the free slot */

  assert( pc>0 );
  while( pc<=maxPC ){
    /* EVIDENCE-OF: R-22710-53328 The third and fourth bytes of each
    ** freeblock form a big-endian integer which is the size of the freeblock
    ** in bytes, including the 4-byte header. */
    pTmp = &aData[pc+2];
    size = get2byte(pTmp);
    if( (x = size - nByte)>=0 ){
      testcase( x==4 );
      testcase( x==3 );
      if( x<4 ){
        /* EVIDENCE-OF: R-11498-58022 In a well-formed b-tree page, the total
        ** number of bytes in fragments may not exceed 60. */
        if( aData[hdr+7]>57 ) return 0;

        /* Remove the slot from the free-list. Update the number of
        ** fragmented bytes within the page. */
        memcpy(&aData[iAddr], &aData[pc], 2);
        aData[hdr+7] += (u8)x;
        return &aData[pc];
      }else if( x+pc > maxPC ){
        /* This slot extends off the end of the usable part of the page */
        *pRc = SQLITE_CORRUPT_PAGE(pPg);
        return 0;
      }else{
        /* The slot remains on the free-list. Reduce its size to account
        ** for the portion used by the new allocation. */
        put2byte(&aData[pc+2], x);
      }
      return &aData[pc + x];
    }
    iAddr = pc;
    pTmp = &aData[pc];
    pc = get2byte(pTmp);
    if( pc<=iAddr ){
      if( pc ){
        /* The next slot in the chain comes before the current slot */
        *pRc = SQLITE_CORRUPT_PAGE(pPg);
      }
      return 0;
    }
  }
  if( pc>maxPC+nByte-4 ){
    /* The free slot chain extends off the end of the page */
    *pRc = SQLITE_CORRUPT_PAGE(pPg);
  }
  return 0;
}
/* …（同文件无关代码省略）… */
static SQLITE_INLINE int allocateSpace(MemPage *pPage, int nByte, int *pIdx){
  const int hdr = pPage->hdrOffset;    /* Local cache of pPage->hdrOffset */
  u8 * const data = pPage->aData;      /* Local cache of pPage->aData */
  int top;                             /* First byte of cell content area */
  int rc = SQLITE_OK;                  /* Integer return code */
  u8 *pTmp;                            /* Temp ptr into data[] */
  int gap;        /* First byte of gap between cell pointers and cell content */
 
  assert( sqlite3PagerIswriteable(pPage->pDbPage) );
  assert( pPage->pBt );
  assert( sqlite3_mutex_held(pPage->pBt->mutex) );
  assert( nByte>=0 );  /* Minimum cell size is 4 */
  assert( pPage->nFree>=nByte );
  assert( pPage->nOverflow==0 );
  assert( nByte < (int)(pPage->pBt->usableSize-8) );

  assert( pPage->cellOffset == hdr + 12 - 4*pPage->leaf );
  gap = pPage->cellOffset + 2*pPage->nCell;
  assert( gap<=65536 );
  /* EVIDENCE-OF: R-29356-02391 If the database uses a 65536-byte page size
  ** and the reserved space is zero (the usual value for reserved space)
  ** then the cell content offset of an empty page wants to be 65536.
  ** However, that integer is too large to be stored in a 2-byte unsigned
  ** integer, so a value of 0 is used in its place. */
  pTmp = &data[hdr+5];
  top = get2byte(pTmp);
  if( gap>top ){
    if( top==0 && pPage->pBt->usableSize==65536 ){
      top = 65536;
    }else{
      return SQLITE_CORRUPT_PAGE(pPage);
    }
  }else if( top>(int)pPage->pBt->usableSize ){
    return SQLITE_CORRUPT_PAGE(pPage);
  }

  /* If there is enough space between gap and top for one more cell pointer,
  ** and if the freelist is not empty, then search the
  ** freelist looking for a slot big enough to satisfy the request.
  */
  testcase( gap+2==top );
  testcase( gap+1==top );
  testcase( gap==top );
  if( (data[hdr+2] || data[hdr+1]) && gap+2<=top ){
    u8 *pSpace = pageFindSlot(pPage, nByte, &rc);
    if( pSpace ){
      int g2;
      assert( pSpace+nByte<=data+pPage->pBt->usableSize );
      *pIdx = g2 = (int)(pSpace-data);
      if( g2<=gap ){
        return SQLITE_CORRUPT_PAGE(pPage);
      }else{
        return SQLITE_OK;
      }
    }else if( rc ){
      return rc;
    }
  }

  /* The request could not be fulfilled using a freelist slot.  Check
  ** to see if defragmentation is necessary.
  */
  testcase( gap+2+nByte==top );
  if( gap+2+nByte>top ){
    assert( pPage->nCell>0 || CORRUPT_DB );
    assert( pPage->nFree>=0 );
    rc = defragmentPage(pPage, MIN(4, pPage->nFree - (2+nByte)));
    if( rc ) return rc;
    top = get2byteNotZero(&data[hdr+5]);
    assert( gap+2+nByte<=top );
  }


  /* Allocate memory from the gap in between the cell pointer array
  ** and the cell content area.  The btreeComputeFreeSpace() call has already
  ** validated the freelist.  Given that the freelist is valid, there
  ** is no way that the allocation can extend off the end of the page.
  ** The assert() below verifies the previous sentence.
  */
  top -= nByte;
  put2byte(&data[hdr+5], top);
  assert( top+nByte <= (int)pPage->pBt->usableSize );
  *pIdx = top;
  return SQLITE_OK;
}
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
static void dropCell(MemPage *pPage, int idx, int sz, int *pRC){
  u32 pc;         /* Offset to cell content of cell being deleted */
  u8 *data;       /* pPage->aData */
  u8 *ptr;        /* Used to move bytes around within data[] */
  int rc;         /* The return code */
  int hdr;        /* Beginning of the header.  0 most pages.  100 page 1 */

  if( *pRC ) return;
  assert( idx>=0 );
  assert( idx<pPage->nCell );
  assert( CORRUPT_DB || sz==cellSize(pPage, idx) );
  assert( sqlite3PagerIswriteable(pPage->pDbPage) );
  assert( sqlite3_mutex_held(pPage->pBt->mutex) );
  assert( pPage->nFree>=0 );
  data = pPage->aData;
  ptr = &pPage->aCellIdx[2*idx];
  assert( pPage->pBt->usableSize > (u32)(ptr-data) );
  pc = get2byte(ptr);
  hdr = pPage->hdrOffset;
  testcase( pc==(u32)get2byte(&data[hdr+5]) );
  testcase( pc+sz==pPage->pBt->usableSize );
  if( pc+sz > pPage->pBt->usableSize ){
    *pRC = SQLITE_CORRUPT_BKPT;
    return;
  }
  rc = freeSpace(pPage, pc, sz);
  if( rc ){
    *pRC = rc;
    return;
  }
  pPage->nCell--;
  if( pPage->nCell==0 ){
    memset(&data[hdr+1], 0, 4);
    data[hdr+7] = 0;
    put2byte(&data[hdr+5], pPage->pBt->usableSize);
    pPage->nFree = pPage->pBt->usableSize - pPage->hdrOffset
                       - pPage->childPtrSize - 8;
  }else{
    memmove(ptr, ptr+2, 2*(pPage->nCell - idx));
    put2byte(&data[hdr+3], pPage->nCell);
    pPage->nFree += 2;
  }
}
/* …（同文件无关代码省略）… */
static int insertCell(
  MemPage *pPage,   /* Page into which we are copying */
  int i,            /* New cell becomes the i-th cell of the page */
  u8 *pCell,        /* Content of the new cell */
  int sz,           /* Bytes of content in pCell */
  u8 *pTemp,        /* Temp storage space for pCell, if needed */
  Pgno iChild       /* If non-zero, replace first 4 bytes with this value */
){
  int idx = 0;      /* Where to write new cell content in data[] */
  int j;            /* Loop counter */
  u8 *data;         /* The content of the whole page */
  u8 *pIns;         /* The point in pPage->aCellIdx[] where no cell inserted */

  assert( i>=0 && i<=pPage->nCell+pPage->nOverflow );
  assert( MX_CELL(pPage->pBt)<=10921 );
  assert( pPage->nCell<=MX_CELL(pPage->pBt) || CORRUPT_DB );
  assert( pPage->nOverflow<=ArraySize(pPage->apOvfl) );
  assert( ArraySize(pPage->apOvfl)==ArraySize(pPage->aiOvfl) );
  assert( sqlite3_mutex_held(pPage->pBt->mutex) );
  assert( sz==pPage->xCellSize(pPage, pCell) || CORRUPT_DB );
  assert( pPage->nFree>=0 );
  assert( iChild>0 );
  if( pPage->nOverflow || sz+2>pPage->nFree ){
    if( pTemp ){
      memcpy(pTemp, pCell, sz);
      pCell = pTemp;
    }
    put4byte(pCell, iChild);
    j = pPage->nOverflow++;
    /* Comparison against ArraySize-1 since we hold back one extra slot
    ** as a contingency.  In other words, never need more than 3 overflow
    ** slots but 4 are allocated, just to be safe. */
    assert( j < ArraySize(pPage->apOvfl)-1 );
    pPage->apOvfl[j] = pCell;
    pPage->aiOvfl[j] = (u16)i;

    /* When multiple overflows occur, they are always sequential and in
    ** sorted order.  This invariants arise because multiple overflows can
    ** only occur when inserting divider cells into the parent page during
    ** balancing, and the dividers are adjacent and sorted.
    */
    assert( j==0 || pPage->aiOvfl[j-1]<(u16)i ); /* Overflows in sorted order */
    assert( j==0 || i==pPage->aiOvfl[j-1]+1 );   /* Overflows are sequential */
  }else{
    int rc = sqlite3PagerWrite(pPage->pDbPage);
    if( NEVER(rc!=SQLITE_OK) ){
      return rc;
    }
    assert( sqlite3PagerIswriteable(pPage->pDbPage) );
    data = pPage->aData;
    assert( &data[pPage->cellOffset]==pPage->aCellIdx );
    rc = allocateSpace(pPage, sz, &idx);
    if( rc ){ return rc; }
    /* The allocateSpace() routine guarantees the following properties
    ** if it returns successfully */
    assert( idx >= 0 );
    assert( idx >= pPage->cellOffset+2*pPage->nCell+2 || CORRUPT_DB );
    assert( idx+sz <= (int)pPage->pBt->usableSize );
    pPage->nFree -= (u16)(2 + sz);
    /* In a corrupt database where an entry in the cell index section of
    ** a btree page has a value of 3 or less, the pCell value might point
    ** as many as 4 bytes in front of the start of the aData buffer for
    ** the source page.  Make sure this does not cause problems by not
    ** reading the first 4 bytes */
    memcpy(&data[idx+4], pCell+4, sz-4);
    put4byte(&data[idx], iChild);
    pIns = pPage->aCellIdx + i*2;
    memmove(pIns+2, pIns, 2*(pPage->nCell - i));
    put2byte(pIns, idx);
    pPage->nCell++;
    /* increment the cell count */
    if( (++data[pPage->hdrOffset+4])==0 ) data[pPage->hdrOffset+3]++;
    assert( get2byte(&data[pPage->hdrOffset+3])==pPage->nCell || CORRUPT_DB );
#ifndef SQLITE_OMIT_AUTOVACUUM
    if( pPage->pBt->autoVacuum ){
      int rc2 = SQLITE_OK;
      /* The cell may contain a pointer to an overflow page. If so, write
      ** the entry for the overflow page into the pointer map.
      */
      ptrmapPutOvflPtr(pPage, pPage, pCell, &rc2);
      if( rc2 ) return rc2;
    }
#endif
  }
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
    assert( pTmp!=0 );
    rc = sqlite3PagerWrite(pLeaf->pDbPage);
    if( rc==SQLITE_OK ){
      rc = insertCell(pPage, iCellIdx, pCell-4, nCell+4, pTmp, n);
    }
    dropCell(pLeaf, pLeaf->nCell-1, nCell, &rc);
    if( rc ) return rc;
