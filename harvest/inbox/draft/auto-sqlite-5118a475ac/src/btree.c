// AUTO-DRAFT from sqlite/sqlite PR #3d0c703e3d18b05099d90163189bca51e6820526
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
  #define ptrmapGet(w,x,y,z) SQLITE_OK
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

#ifndef SQLITE_OMIT_INTEGRITY_CHECK
/*
** Record an OOM error during integrity_check
*/
static void checkOom(IntegrityCk *pCheck){
  pCheck->rc = SQLITE_NOMEM;
  pCheck->mxErr = 0;  /* Causes integrity_check processing to stop */
  if( pCheck->nErr==0 ) pCheck->nErr++;
}

/*
/* …（同文件无关代码省略）… */
static void checkProgress(IntegrityCk *pCheck){
  sqlite3 *db = pCheck->db;
  if( AtomicLoad(&db->u1.isInterrupted) ){
    pCheck->rc = SQLITE_INTERRUPT;
    pCheck->nErr++;
    pCheck->mxErr = 0;
  }
#ifndef SQLITE_OMIT_PROGRESS_CALLBACK
  if( db->xProgress ){
    assert( db->nProgressOps>0 );
    pCheck->nStep++;
    if( (pCheck->nStep % db->nProgressOps)==0
     && db->xProgress(db->pProgressArg)
    ){
      pCheck->rc = SQLITE_INTERRUPT;
      pCheck->nErr++;
      pCheck->mxErr = 0;
    }
  }
#endif
}
/* …（同文件无关代码省略）… */
static void checkAppendMsg(
  IntegrityCk *pCheck,
  const char *zFormat,
  ...
){
  va_list ap;
  checkProgress(pCheck);
  if( !pCheck->mxErr ) return;
  pCheck->mxErr--;
  pCheck->nErr++;
  va_start(ap, zFormat);
  if( pCheck->errMsg.nChar ){
    sqlite3_str_append(&pCheck->errMsg, "\n", 1);
  }
  if( pCheck->zPfx ){
    sqlite3_str_appendf(&pCheck->errMsg, pCheck->zPfx,
                        pCheck->v0, pCheck->v1, pCheck->v2);
  }
  sqlite3_str_vappendf(&pCheck->errMsg, zFormat, ap);
  va_end(ap);
  if( pCheck->errMsg.accError==SQLITE_NOMEM ){
    checkOom(pCheck);
  }
}
#endif /* SQLITE_OMIT_INTEGRITY_CHECK */
/* …（同文件无关代码省略）… */

  rc = ptrmapGet(pCheck->pBt, iChild, &ePtrmapType, &iPtrmapParent);
  if( rc!=SQLITE_OK ){
    if( rc==SQLITE_NOMEM || rc==SQLITE_IOERR_NOMEM ) checkOom(pCheck);
    checkAppendMsg(pCheck, "Failed to read ptrmap key=%u", iChild);
    return;
  }
/* …（同文件无关代码省略）… */
  pCheck->v1 = iPage;
  pCheck->nAbove++;
  if( (rc = btreeGetPage(pBt, iPage, &pPage, 0))!=0 ){
    checkAppendMsg(pCheck,
       "unable to get the page. error code=%d", rc);
    if( rc==SQLITE_IOERR_NOMEM ) pCheck->rc = SQLITE_NOMEM;
    goto end_of_check;
  }
  if( pCheck->nAbove > BTCURSOR_MAX_DEPTH ){
/* …（同文件无关代码省略）… */

  sCheck.aPgRef = sqlite3MallocZero((sCheck.nCkPage / 8)+ 1);
  if( !sCheck.aPgRef ){
    checkOom(&sCheck);
    goto integrity_ck_cleanup;
  }
  sCheck.heap = (u32*)sqlite3Malloc( pBt->pageSize*2 );
#ifdef SQLITE_DEBUG
  sCheck.mxHeap = pBt->pageSize/2 - 1;
#endif
  if( sCheck.heap==0 ){
    checkOom(&sCheck);
    goto integrity_ck_cleanup;
  }
