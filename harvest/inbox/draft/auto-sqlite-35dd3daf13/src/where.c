// AUTO-DRAFT from sqlite/sqlite PR #d8d097aebb6b7641c76ff982d97038a138b0c132
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>

  return pWInfo->eDistinct;
}

/*
** Return the number of ORDER BY terms that are satisfied by the
** WHERE clause.  A return of 0 means that the output must be
/* …（同文件无关代码省略）… */
static SQLITE_NOINLINE void whereCheckIfBloomFilterIsUseful(
  const WhereInfo *pWInfo
){
  int i;
  LogEst nSearch = 0;

  assert( pWInfo->nLevel>=2 );
  assert( OptimizationEnabled(pWInfo->pParse->db, SQLITE_BloomFilter) );
  for(i=0; i<pWInfo->nLevel; i++){
    WhereLoop *pLoop = pWInfo->a[i].pWLoop;
    const unsigned int reqFlags = (WHERE_SELFCULL|WHERE_COLUMN_EQ);
    SrcItem *pItem = &pWInfo->pTabList->a[pLoop->iTab];
    Table *pTab = pItem->pSTab;
    if( (pTab->tabFlags & TF_HasStat1)==0 ) break;
    pTab->tabFlags |= TF_MaybeReanalyze;
    if( i>=1
     && (pLoop->wsFlags & reqFlags)==reqFlags
     && sqlite3WhereLoopBloomable(pLoop)
     && nSearch > pTab->nRowLogEst
    ){
      testcase( pItem->fg.jointype & JT_LEFT );
      pLoop->wsFlags |= WHERE_BLOOMFILTER;
      pLoop->wsFlags &= ~WHERE_IDX_ONLY;
      WHERETRACE(0xffffffff, (
         "-> use Bloom-filter on loop %c because there are ~%.1e "
         "lookups into %s which has only ~%.1e rows\n",
         pLoop->cId, (double)sqlite3LogEstToInt(nSearch), pTab->zName,
         (double)sqlite3LogEstToInt(pTab->nRowLogEst)));
    }
    nSearch += pLoop->nOut;
  }
}
/* …（同文件无关代码省略）… */
  */
  if( pWInfo->nLevel>=2
   && OptimizationEnabled(db, SQLITE_BloomFilter)
  ){
    whereCheckIfBloomFilterIsUseful(pWInfo);
  }
