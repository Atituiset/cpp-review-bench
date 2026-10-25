// AUTO-DRAFT from sqlite/sqlite PR #31b1a10a9c623fa1078e0a95a8d711e1ae2d0f84
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
typedef struct PCache1 PCache1;
/* …（同文件无关代码省略）… */
typedef struct PGroup PGroup;
/* …（同文件无关代码省略）… */
** object is positioned immediately after the page buffer to serve as an
** overrun area, so that overreads are harmless.
**
** Variables isBulkLocal and isAnchor were once type "u8". That works,
** but causes a 2-byte gap in the structure for most architectures (since
** pointers must be either 4 or 8-byte aligned). As this structure is located
/* …（同文件无关代码省略）… */
** read part of this structure before the corruption is detected. This
** can cause a valgrind error if the uninitialized gap is accessed. Using u16
** ensures there is no such gap, and therefore no bytes of uninitialized
** memory in the structure.
**
** The pLruNext and pLruPrev pointers form a double-linked circular list
** of all pages that are unpinned.  The PGroup.lru element (which should be
** the only element on the list with PgHdr1.isAnchor set to 1) forms the
** beginning and the end of the list.
*/
struct PgHdr1 {
  sqlite3_pcache_page page; /* Base class. Must be first. pBuf & pExtra */
  unsigned int iKey;        /* Key value (page number) */
  u16 isBulkLocal;          /* This page from bulk local storage */
  u16 isAnchor;             /* This is the PGroup.lru element */
  PgHdr1 *pNext;            /* Next in hash table chain */
  PCache1 *pCache;          /* Cache that currently owns this page */
  PgHdr1 *pLruNext;         /* Next in circular LRU list of unpinned pages */
  PgHdr1 *pLruPrev;         /* Previous in LRU list of unpinned pages */
                            /* NB: pLruPrev is only valid if pLruNext!=0 */
};

/*
** A page is pinned if it is not on the LRU list.  To be "pinned" means
** that the page is in active use and must not be deallocated.
/* …（同文件无关代码省略）… */
#define PAGE_IS_UNPINNED(p)  ((p)->pLruNext!=0)
/* …（同文件无关代码省略）… */
# define pcache1EnterMutex(X)  assert((X)->mutex==0)
/* …（同文件无关代码省略）… */
# define PCACHE1_MIGHT_USE_GROUP_MUTEX 0
/* …（同文件无关代码省略）… */
**
** The PGroup mutex must be held when this function is called.
*/
static PgHdr1 *pcache1PinPage(PgHdr1 *pPage){
  assert( pPage!=0 );
  assert( PAGE_IS_UNPINNED(pPage) );
  assert( pPage->pLruNext );
/* …（同文件无关代码省略）… */
  assert( pPage->isAnchor==0 );
  assert( pPage->pCache->pGroup->lru.isAnchor==1 );
  pPage->pCache->nRecyclable--;
  return pPage;
}


/* …（同文件无关代码省略）… */
** usually not needed, and by avoiding the stack initialization required
** for these steps, the main pcache1Fetch() procedure can run faster.
*/
static SQLITE_NOINLINE PgHdr1 *pcache1FetchStage2(
  PCache1 *pCache,
  unsigned int iKey,
  int createFlag
/* …（同文件无关代码省略）… */
    if( iKey>pCache->iMaxKey ){
      pCache->iMaxKey = iKey;
    }
  }
  return pPage;
}

/*
/* …（同文件无关代码省略）… */
** the common case where pGroup->mutex is NULL.  The pcache1Fetch() wrapper
** invokes the appropriate routine.
*/
static PgHdr1 *pcache1FetchNoMutex(
  sqlite3_pcache *p,
  unsigned int iKey,
  int createFlag
/* …（同文件无关代码省略）… */
    if( PAGE_IS_UNPINNED(pPage) ){
      return pcache1PinPage(pPage);
    }else{
      return pPage;
    }
  }else if( createFlag ){
    /* Steps 3, 4, and 5 implemented by this subroutine */
/* …（同文件无关代码省略）… */
  }
}
#if PCACHE1_MIGHT_USE_GROUP_MUTEX
static PgHdr1 *pcache1FetchWithMutex(
  sqlite3_pcache *p,
  unsigned int iKey,
  int createFlag
){
  PCache1 *pCache = (PCache1 *)p;
  PgHdr1 *pPage;

  pcache1EnterMutex(pCache->pGroup);
  pPage = pcache1FetchNoMutex(p, iKey, createFlag);
/* …（同文件无关代码省略）… */
  PCache1 *pCache = (PCache1 *)p;
#endif

  assert( offsetof(PgHdr1,page)==0 );
  assert( pCache->bPurgeable || createFlag!=1 );
  assert( pCache->bPurgeable || pCache->nMin==0 );
  assert( pCache->bPurgeable==0 || pCache->nMin==10 );
  assert( pCache->nMin==0 || pCache->bPurgeable );
  assert( pCache->nHash>0 );
#if PCACHE1_MIGHT_USE_GROUP_MUTEX
  if( pCache->pGroup->mutex ){
    return (sqlite3_pcache_page*)pcache1FetchWithMutex(p, iKey, createFlag);
  }else
#endif
  {
    return (sqlite3_pcache_page*)pcache1FetchNoMutex(p, iKey, createFlag);
  }
}

/* …（同文件无关代码省略）… */
  int reuseUnlikely
){
  PCache1 *pCache = (PCache1 *)p;
  PgHdr1 *pPage = (PgHdr1 *)pPg;
  PGroup *pGroup = pCache->pGroup;

  assert( pPage->pCache==pCache );
/* …（同文件无关代码省略）… */
  unsigned int iNew
){
  PCache1 *pCache = (PCache1 *)p;
  PgHdr1 *pPage = (PgHdr1 *)pPg;
  PgHdr1 **pp;
  unsigned int hOld, hNew;
  assert( pPage->iKey==iOld );
/* …（同文件无关代码省略）… */

  pcache1EnterMutex(pCache->pGroup);

  assert( pcache1FetchNoMutex(p, iOld, 0)==pPage ); /* pPg really is iOld */
  assert( pCache->nHash>0 && (pCache->nHash & (pCache->nHash-1))==0 );
  hOld = iOld & (pCache->nHash-1);
  pp = &pCache->apHash[hOld];
