// AUTO-DRAFT from sqlite/sqlite PR #09a55130084c903bb0d1b6a2a8a672660b4bce37
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct WalIndexHdr WalIndexHdr;
typedef struct WalIterator WalIterator;
typedef struct WalCkptInfo WalCkptInfo;
/* …（同文件无关代码省略）… */
#define WALINDEX_HDR_SIZE    (sizeof(WalIndexHdr)*2+sizeof(WalCkptInfo))
/* …（同文件无关代码省略）… */
struct Wal {
  sqlite3_vfs *pVfs;         /* The VFS used to create pDbFd */
  sqlite3_file *pDbFd;       /* File handle for the database file */
  sqlite3_file *pWalFd;      /* File handle for WAL file */
  u32 iCallback;             /* Value to pass to log callback (or 0) */
  i64 mxWalSize;             /* Truncate WAL to this size upon reset */
  int nWiData;               /* Size of array apWiData */
  int szFirstBlock;          /* Size of first block written to WAL file */
  volatile u32 **apWiData;   /* Pointer to wal-index content in memory */
  u32 szPage;                /* Database page size */
  i16 readLock;              /* Which read lock is being held.  -1 for none */
  u8 syncFlags;              /* Flags to use to sync header writes */
  u8 exclusiveMode;          /* Non-zero if connection is in exclusive mode */
  u8 writeLock;              /* True if in a write transaction */
  u8 ckptLock;               /* True if holding a checkpoint lock */
  u8 readOnly;               /* WAL_RDWR, WAL_RDONLY, or WAL_SHM_RDONLY */
  u8 truncateOnCommit;       /* True to truncate WAL file on commit */
  u8 syncHeader;             /* Fsync the WAL header if true */
  u8 padToSectorBoundary;    /* Pad transactions out to the next sector */
  u8 bShmUnreliable;         /* SHM content is read-only and unreliable */
  WalIndexHdr hdr;           /* Wal-index header for current transaction */
  u32 minFrame;              /* Ignore wal frames before this one */
  u32 iReCksum;              /* On commit, recalculate checksums from here */
  const char *zWalName;      /* Name of WAL file */
  u32 nCkpt;                 /* Checkpoint sequence counter in the wal-header */
#ifdef SQLITE_USE_SEH
  u32 lockMask;              /* Mask of locks held */
  void *pFree;               /* Pointer to sqlite3_free() if exception thrown */
  u32 *pWiValue;             /* Value to write into apWiData[iWiPg] */
  int iWiPg;                 /* Write pWiValue into apWiData[iWiPg] */
  int iSysErrno;             /* System error code following exception */
#endif
#ifdef SQLITE_DEBUG
  int nSehTry;               /* Number of nested SEH_TRY{} blocks */
  u8 lockError;              /* True if a locking error has occurred */
#endif
#ifdef SQLITE_ENABLE_SNAPSHOT
  WalIndexHdr *pSnapshot;    /* Start transaction here if not NULL */
  int bGetSnapshot;          /* Transaction opened for sqlite3_get_snapshot() */
#endif
#ifdef SQLITE_ENABLE_SETLK_TIMEOUT
  sqlite3 *db;
#endif
};
/* …（同文件无关代码省略）… */
#define WAL_HEAPMEMORY_MODE 2
/* …（同文件无关代码省略）… */
#define WAL_SHM_RDONLY  2    /* The SHM file is readonly */
/* …（同文件无关代码省略）… */
typedef u16 ht_slot;
/* …（同文件无关代码省略）… */
  struct WalSegment {
    int iNext;                    /* Next slot in aIndex[] not yet returned */
    ht_slot *aIndex;              /* i0, i1, i2... such that aPgno[iN] ascend */
    u32 *aPgno;                   /* Array of page numbers. */
    int nEntry;                   /* Nr. of entries in aPgno[] and aIndex[] */
    int iZero;                    /* Frame number associated with aPgno[0] */
  } aSegment[FLEXARRAY];          /* One for every 32KB page in the wal-index */
/* …（同文件无关代码省略）… */
#define SZ_WALITERATOR(N)  \
     (offsetof(WalIterator,aSegment)+(N)*sizeof(struct WalSegment))
/* …（同文件无关代码省略）… */
#define HASHTABLE_NPAGE      4096                 /* Must be power of 2 */
/* …（同文件无关代码省略）… */
#define HASHTABLE_NSLOT      (HASHTABLE_NPAGE*2)  /* Must be a power of 2 */
/* …（同文件无关代码省略）… */
#define HASHTABLE_NPAGE_ONE  (HASHTABLE_NPAGE - (WALINDEX_HDR_SIZE/sizeof(u32)))
/* …（同文件无关代码省略）… */
#define WALINDEX_PGSZ   (                                         \
    sizeof(ht_slot)*HASHTABLE_NSLOT + HASHTABLE_NPAGE*sizeof(u32) \
)
/* …（同文件无关代码省略）… */
# define SEH_INJECT_FAULT sehInjectFault(pWal) 
/* …（同文件无关代码省略）… */
static void sehInjectFault(Wal *pWal){
  int res;
  assert( pWal->nSehTry>0 );

  res = sqlite3FaultSim(650);
  if( res!=0 ){
    ULONG_PTR aArg[3];
    aArg[0] = 0;
    aArg[1] = 0;
    aArg[2] = (ULONG_PTR)res;
    RaiseException(EXCEPTION_IN_PAGE_ERROR, 0, 3, (const ULONG_PTR*)aArg);
  }
}
/* …（同文件无关代码省略）… */
#define SEH_FREE_ON_ERROR(X,Y) \
  assert( (X==0 || Y==0) && pWal->pFree==X ); pWal->pFree = Y
/* …（同文件无关代码省略）… */
static SQLITE_NOINLINE int walIndexPageRealloc(
  Wal *pWal,               /* The WAL context */
  int iPage,               /* The page we seek */
  volatile u32 **ppPage    /* Write the page pointer here */
){
  int rc = SQLITE_OK;

  /* Enlarge the pWal->apWiData[] array if required */
  if( pWal->nWiData<=iPage ){
    sqlite3_int64 nByte = sizeof(u32*)*(1+(i64)iPage);
    volatile u32 **apNew;
    apNew = (volatile u32 **)sqlite3Realloc((void *)pWal->apWiData, nByte);
    if( !apNew ){
      *ppPage = 0;
      return SQLITE_NOMEM_BKPT;
    }
    memset((void*)&apNew[pWal->nWiData], 0,
           sizeof(u32*)*(iPage+1-pWal->nWiData));
    pWal->apWiData = apNew;
    pWal->nWiData = iPage+1;
  }

  /* Request a pointer to the required page from the VFS */
  assert( pWal->apWiData[iPage]==0 );
  if( pWal->exclusiveMode==WAL_HEAPMEMORY_MODE ){
    pWal->apWiData[iPage] = (u32 volatile *)sqlite3MallocZero(WALINDEX_PGSZ);
    if( !pWal->apWiData[iPage] ) rc = SQLITE_NOMEM_BKPT;
  }else{
    rc = sqlite3OsShmMap(pWal->pDbFd, iPage, WALINDEX_PGSZ,
        pWal->writeLock, (void volatile **)&pWal->apWiData[iPage]
    );
    assert( pWal->apWiData[iPage]!=0
         || rc!=SQLITE_OK
         || (pWal->writeLock==0 && iPage==0) );
    testcase( pWal->apWiData[iPage]==0 && rc==SQLITE_OK );
    if( rc==SQLITE_OK ){
      if( iPage>0 && sqlite3FaultSim(600) ) rc = SQLITE_NOMEM;
    }else if( (rc&0xff)==SQLITE_READONLY ){
      pWal->readOnly |= WAL_SHM_RDONLY;
      if( rc==SQLITE_READONLY ){
        rc = SQLITE_OK;
      }
    }
  }

  *ppPage = pWal->apWiData[iPage];
  assert( iPage==0 || *ppPage || rc!=SQLITE_OK );
  return rc;
}
static int walIndexPage(
  Wal *pWal,               /* The WAL context */
  int iPage,               /* The page we seek */
  volatile u32 **ppPage    /* Write the page pointer here */
){
  SEH_INJECT_FAULT;
  if( pWal->nWiData<=iPage || (*ppPage = pWal->apWiData[iPage])==0 ){
    return walIndexPageRealloc(pWal, iPage, ppPage);
  }
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
typedef struct WalHashLoc WalHashLoc;
/* …（同文件无关代码省略）… */
static int walHashGet(
  Wal *pWal,                      /* WAL handle */
  int iHash,                      /* Find the iHash'th table */
  WalHashLoc *pLoc                /* OUT: Hash table location */
){
  int rc;                         /* Return code */

  rc = walIndexPage(pWal, iHash, &pLoc->aPgno);
  assert( rc==SQLITE_OK || iHash>0 );

  if( pLoc->aPgno ){
    pLoc->aHash = (volatile ht_slot *)&pLoc->aPgno[HASHTABLE_NPAGE];
    if( iHash==0 ){
      pLoc->aPgno = &pLoc->aPgno[WALINDEX_HDR_SIZE/sizeof(u32)];
      pLoc->iZero = 0;
    }else{
      pLoc->iZero = HASHTABLE_NPAGE_ONE + (iHash-1)*HASHTABLE_NPAGE;
    }
  }else if( NEVER(rc==SQLITE_OK) ){
    rc = SQLITE_ERROR;
  }
  return rc;
}
/* …（同文件无关代码省略）… */
static int walFramePage(u32 iFrame){
  int iHash = (iFrame+HASHTABLE_NPAGE-HASHTABLE_NPAGE_ONE-1) / HASHTABLE_NPAGE;
  assert( (iHash==0 || iFrame>HASHTABLE_NPAGE_ONE)
       && (iHash>=1 || iFrame<=HASHTABLE_NPAGE_ONE)
       && (iHash<=1 || iFrame>(HASHTABLE_NPAGE_ONE+HASHTABLE_NPAGE))
       && (iHash>=2 || iFrame<=HASHTABLE_NPAGE_ONE+HASHTABLE_NPAGE)
       && (iHash<=2 || iFrame>(HASHTABLE_NPAGE_ONE+2*HASHTABLE_NPAGE))
  );
  assert( iHash>=0 );
  return iHash;
}
/* …（同文件无关代码省略）… */
static void walMerge(
  const u32 *aContent,            /* Pages in wal - keys for the sort */
  ht_slot *aLeft,                 /* IN: Left hand input list */
  int nLeft,                      /* IN: Elements in array *paLeft */
  ht_slot **paRight,              /* IN/OUT: Right hand input list */
  int *pnRight,                   /* IN/OUT: Elements in *paRight */
  ht_slot *aTmp                   /* Temporary buffer */
){
  int iLeft = 0;                  /* Current index in aLeft */
  int iRight = 0;                 /* Current index in aRight */
  int iOut = 0;                   /* Current index in output buffer */
  int nRight = *pnRight;
  ht_slot *aRight = *paRight;

  assert( nLeft>0 && nRight>0 );
  while( iRight<nRight || iLeft<nLeft ){
    ht_slot logpage;
    Pgno dbpage;

    if( (iLeft<nLeft)
     && (iRight>=nRight || aContent[aLeft[iLeft]]<aContent[aRight[iRight]])
    ){
      logpage = aLeft[iLeft++];
    }else{
      logpage = aRight[iRight++];
    }
    dbpage = aContent[logpage];

    aTmp[iOut++] = logpage;
    if( iLeft<nLeft && aContent[aLeft[iLeft]]==dbpage ) iLeft++;

    assert( iLeft>=nLeft || aContent[aLeft[iLeft]]>dbpage );
    assert( iRight>=nRight || aContent[aRight[iRight]]>dbpage );
  }

  *paRight = aLeft;
  *pnRight = iOut;
  memcpy(aLeft, aTmp, sizeof(aTmp[0])*iOut);
}
/* …（同文件无关代码省略）… */
static void walMergesort(
  const u32 *aContent,            /* Pages in wal */
  ht_slot *aBuffer,               /* Buffer of at least *pnList items to use */
  ht_slot *aList,                 /* IN/OUT: List to sort */
  int *pnList                     /* IN/OUT: Number of elements in aList[] */
){
  struct Sublist {
    int nList;                    /* Number of elements in aList */
    ht_slot *aList;               /* Pointer to sub-list content */
  };

  const int nList = *pnList;      /* Size of input list */
  int nMerge = 0;                 /* Number of elements in list aMerge */
  ht_slot *aMerge = 0;            /* List to be merged */
  int iList;                      /* Index into input list */
  u32 iSub = 0;                   /* Index into aSub array */
  struct Sublist aSub[13];        /* Array of sub-lists */

  memset(aSub, 0, sizeof(aSub));
  assert( nList<=HASHTABLE_NPAGE && nList>0 );
  assert( HASHTABLE_NPAGE==(1<<(ArraySize(aSub)-1)) );

  for(iList=0; iList<nList; iList++){
    nMerge = 1;
    aMerge = &aList[iList];
    for(iSub=0; iList & (1<<iSub); iSub++){
      struct Sublist *p;
      assert( iSub<ArraySize(aSub) );
      p = &aSub[iSub];
      assert( p->aList && p->nList<=(1<<iSub) );
      assert( p->aList==&aList[iList&~((2<<iSub)-1)] );
      walMerge(aContent, p->aList, p->nList, &aMerge, &nMerge, aBuffer);
    }
    aSub[iSub].aList = aMerge;
    aSub[iSub].nList = nMerge;
  }

  for(iSub++; iSub<ArraySize(aSub); iSub++){
    if( nList & (1<<iSub) ){
      struct Sublist *p;
      assert( iSub<ArraySize(aSub) );
      p = &aSub[iSub];
      assert( p->nList<=(1<<iSub) );
      assert( p->aList==&aList[nList&~((2<<iSub)-1)] );
      walMerge(aContent, p->aList, p->nList, &aMerge, &nMerge, aBuffer);
    }
  }
  assert( aMerge==aList );
  *pnList = nMerge;

#ifdef SQLITE_DEBUG
  {
    int i;
    for(i=1; i<*pnList; i++){
      assert( aContent[aList[i]] > aContent[aList[i-1]] );
    }
  }
#endif
}
/* …（同文件无关代码省略）… */
** WalIterator object when it has finished with it.
*/
static int walIteratorInit(Wal *pWal, u32 nBackfill, WalIterator **pp){
  WalIterator *p;                 /* Return value */
  int nSegment;                   /* Number of segments to merge */
  u32 iLast;                      /* Last frame in log */
/* …（同文件无关代码省略）… */
  int rc = SQLITE_OK;             /* Return Code */

  /* This routine only runs while holding the checkpoint lock. And
  ** it only runs if there is actually content in the log (mxFrame>0).
  */
  assert( pWal->ckptLock && pWal->hdr.mxFrame>0 );
  iLast = pWal->hdr.mxFrame;

  /* Allocate space for the WalIterator object. */
  nSegment = walFramePage(iLast) + 1;
  nByte = SZ_WALITERATOR(nSegment)
        + iLast*sizeof(ht_slot);
  p = (WalIterator *)sqlite3_malloc64(nByte
      + sizeof(ht_slot) * (iLast>HASHTABLE_NPAGE?HASHTABLE_NPAGE:iLast)
  );
/* …（同文件无关代码省略）… */
  p->nSegment = nSegment;
  aTmp = (ht_slot*)&(((u8*)p)[nByte]);
  SEH_FREE_ON_ERROR(0, p);
  for(i=walFramePage(nBackfill+1); rc==SQLITE_OK && i<nSegment; i++){
    WalHashLoc sLoc;

    rc = walHashGet(pWal, i, &sLoc);
/* …（同文件无关代码省略）… */
      int nEntry;                 /* Number of entries in this segment */
      ht_slot *aIndex;            /* Sorted index for this segment */

      if( (i+1)==nSegment ){
        nEntry = (int)(iLast - sLoc.iZero);
      }else{
        nEntry = (int)((u32*)sLoc.aHash - (u32*)sLoc.aPgno);
      }
      aIndex = &((ht_slot *)&p->aSegment[p->nSegment])[sLoc.iZero];
      sLoc.iZero++;

      for(j=0; j<nEntry; j++){
        aIndex[j] = (ht_slot)j;
      }
      walMergesort((u32 *)sLoc.aPgno, aTmp, aIndex, &nEntry);
      p->aSegment[i].iZero = sLoc.iZero;
      p->aSegment[i].nEntry = nEntry;
      p->aSegment[i].aIndex = aIndex;
      p->aSegment[i].aPgno = (u32 *)sLoc.aPgno;
    }
  }
  if( rc!=SQLITE_OK ){
