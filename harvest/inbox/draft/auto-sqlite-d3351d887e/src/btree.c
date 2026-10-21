// AUTO-DRAFT from sqlite/sqlite PR #5a1da0ae1bac995707f1a4dee41c7935f538a51c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

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
static void btreeParseCellPtrNoPayload(
  MemPage *pPage,         /* Page containing the cell */
  u8 *pCell,              /* Pointer to the cell text. */
  CellInfo *pInfo         /* Fill in this structure */
){
  assert( sqlite3_mutex_held(pPage->pBt->mutex) );
  assert( pPage->leaf==0 );
  assert( pPage->childPtrSize==4 );
#ifndef SQLITE_DEBUG
  UNUSED_PARAMETER(pPage);
#endif
  pInfo->nSize = 4 + sqlite3GetVarint(&pCell[4], (u64*)&pInfo->nKey);
  pInfo->nPayload = 0;
  pInfo->nLocal = 0;
  pInfo->pPayload = 0;
  return;
}
static void btreeParseCellPtr(
  MemPage *pPage,         /* Page containing the cell */
  u8 *pCell,              /* Pointer to the cell text. */
  CellInfo *pInfo         /* Fill in this structure */
){
  u8 *pIter;              /* For scanning through pCell */
  u64 nPayload;           /* Number of bytes of cell payload */
  u64 iKey;               /* Extracted Key value */

  assert( sqlite3_mutex_held(pPage->pBt->mutex) );
  assert( pPage->leaf==0 || pPage->leaf==1 );
  assert( pPage->intKeyLeaf );
  assert( pPage->childPtrSize==0 );
  pIter = pCell;

  /* The next block of code is equivalent to:
  **
  **     pIter += getVarint32(pIter, nPayload);
  **
  ** The code is inlined to avoid a function call.
  */
  nPayload = *pIter;
  if( nPayload>=0x80 ){
    u8 *pEnd = &pIter[8];
    nPayload &= 0x7f;
    do{
      nPayload = (nPayload<<7) | (*++pIter & 0x7f);
    }while( (*pIter)>=0x80 && pIter<pEnd );
    nPayload &= 0xffffffff;
  }
  pIter++;

  /* The next block of code is equivalent to:
  **
  **     pIter += sqlite3GetVarint(pIter, (u64*)&pInfo->nKey);
  **
  ** The code is inlined and the loop is unrolled for performance.
  ** This routine is a high-runner.
  */
  iKey = *pIter;
  if( iKey>=0x80 ){
    u8 x;
    iKey = (iKey<<7) ^ (x = *++pIter);
    if( x>=0x80 ){
      iKey = (iKey<<7) ^ (x = *++pIter);
      if( x>=0x80 ){
        iKey = (iKey<<7) ^ 0x10204000 ^ (x = *++pIter);
        if( x>=0x80 ){
          iKey = (iKey<<7) ^ 0x4000 ^ (x = *++pIter);
          if( x>=0x80 ){
            iKey = (iKey<<7) ^ 0x4000 ^ (x = *++pIter);
            if( x>=0x80 ){
              iKey = (iKey<<7) ^ 0x4000 ^ (x = *++pIter);
              if( x>=0x80 ){
                iKey = (iKey<<7) ^ 0x4000 ^ (x = *++pIter);
                if( x>=0x80 ){
                  iKey = (iKey<<8) ^ 0x8000 ^ (*++pIter);
                }
              }
            }
          }
        }
      }else{
        iKey ^= 0x204000;
      }
    }else{
      iKey ^= 0x4000;
    }
  }
  pIter++;

  pInfo->nKey = *(i64*)&iKey;
  pInfo->nPayload = (u32)nPayload;
  pInfo->pPayload = pIter;
  testcase( nPayload==pPage->maxLocal );
  testcase( nPayload==(u32)pPage->maxLocal+1 );
  assert( pPage->maxLocal <= BT_MAX_LOCAL );
  if( nPayload<=pPage->maxLocal ){
    /* This is the (easy) common case where the entire payload fits
    ** on the local page.  No overflow is required.
    */
    pInfo->nSize = (u16)nPayload + (u16)(pIter - pCell);
    if( pInfo->nSize<4 ) pInfo->nSize = 4;
    pInfo->nLocal = (u16)nPayload;
  }else{
    btreeParseCellAdjustSizeForOverflow(pPage, pCell, pInfo);
  }
}
static void btreeParseCellPtrIndex(
  MemPage *pPage,         /* Page containing the cell */
  u8 *pCell,              /* Pointer to the cell text. */
  CellInfo *pInfo         /* Fill in this structure */
){
  u8 *pIter;              /* For scanning through pCell */
  u32 nPayload;           /* Number of bytes of cell payload */

  assert( sqlite3_mutex_held(pPage->pBt->mutex) );
  assert( pPage->leaf==0 || pPage->leaf==1 );
  assert( pPage->intKeyLeaf==0 );
  pIter = pCell + pPage->childPtrSize;
  nPayload = *pIter;
  if( nPayload>=0x80 ){
    u8 *pEnd = &pIter[8];
    nPayload &= 0x7f;
    do{
      nPayload = (nPayload<<7) | (*++pIter & 0x7f);
    }while( *(pIter)>=0x80 && pIter<pEnd );
  }
  pIter++;
  pInfo->nKey = nPayload;
  pInfo->nPayload = nPayload;
  pInfo->pPayload = pIter;
  testcase( nPayload==pPage->maxLocal );
  testcase( nPayload==(u32)pPage->maxLocal+1 );
  assert( nPayload>=0 );
  assert( pPage->maxLocal <= BT_MAX_LOCAL );
  if( nPayload<=pPage->maxLocal ){
    /* This is the (easy) common case where the entire payload fits
    ** on the local page.  No overflow is required.
    */
    pInfo->nSize = (u16)nPayload + (u16)(pIter - pCell);
    if( pInfo->nSize<4 ) pInfo->nSize = 4;
    pInfo->nLocal = (u16)nPayload;
  }else{
    btreeParseCellAdjustSizeForOverflow(pPage, pCell, pInfo);
  }
}
/* …（同文件无关代码省略）… */
static u16 cellSizePtr(MemPage *pPage, u8 *pCell){
  u8 *pIter = pCell + 4;                   /* For looping over bytes of pCell */
  u8 *pEnd;                                /* End mark for a varint */
  u32 nSize;                               /* Size value to return */

#ifdef SQLITE_DEBUG
  /* The value returned by this function should always be the same as
  ** the (CellInfo.nSize) value found by doing a full parse of the
  ** c
