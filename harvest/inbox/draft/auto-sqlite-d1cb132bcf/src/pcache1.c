// AUTO-DRAFT from sqlite/sqlite PR #4460306eb112cda8b85a70e6e297934a8e477cec
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
typedef struct PCache1 PCache1;
/* …（同文件无关代码省略）… */
typedef struct PgFreeslot PgFreeslot;
typedef struct PGroup PGroup;
/* …（同文件无关代码省略）… */
static SQLITE_WSD struct PCacheGlobal {
  PGroup grp;                    /* The global PGroup for mode (2) */

  /* Variables related to SQLITE_CONFIG_PAGECACHE settings.  The
  ** szSlot, nSlot, pStart, pEnd, nReserve, and isInit values are all
  ** fixed at sqlite3_initialize() time and do not require mutex protection.
  ** The nFreeSlot and pFree values do require mutex protection.
  */
  int isInit;                    /* True if initialized */
  int separateCache;             /* Use a new PGroup for each PCache */
  int nInitPage;                 /* Initial bulk allocation size */
  int szSlot;                    /* Size of each free slot */
  int nSlot;                     /* The number of pcache slots */
  int nReserve;                  /* Try to keep nFreeSlot above this */
  void *pStart, *pEnd;           /* Bounds of global page cache memory */
  /* Above requires no mutex.  Use mutex below for variable that follow. */
  sqlite3_mutex *mutex;          /* Mutex for accessing the following: */
  PgFreeslot *pFree;             /* Free page blocks */
  int nFreeSlot;                 /* Number of unused pcache slots */
  int bUnderPressure;            /* True if low on PAGECACHE memory */
} pcache1_g;
/* …（同文件无关代码省略）… */
#define pcache1 (GLOBAL(struct PCacheGlobal, pcache1_g))
/* …（同文件无关代码省略）… */
static int pcache1UnderMemoryPressure(PCache1 *pCache){
  if( pcache1.nSlot && (pCache->szPage+pCache->szExtra)<=pcache1.szSlot ){
    return AtomicLoad(&pcache1.bUnderPressure);
  }else{
    return sqlite3HeapNearlyFull();
  }
}
/* …（同文件无关代码省略）… */
  /* Step 4. Try to recycle a page. */
  if( pCache->bPurgeable
   && !pGroup->lru.pLruPrev->isAnchor
   && ((pCache->nPage+1>=pCache->nMax) || pcache1UnderMemoryPressure(pCache))
  ){
    PCache1 *pOther;
    pPage = pGroup->lru.pLruPrev;
