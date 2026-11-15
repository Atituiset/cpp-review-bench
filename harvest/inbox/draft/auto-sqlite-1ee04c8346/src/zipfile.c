// AUTO-DRAFT from sqlite/sqlite PR #e742a915065f86d0f271f84dc54663a7508bd12c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdio.h>
  // <<< BUG ANCHOR
typedef sqlite3_int64 i64;
/* …（同文件无关代码省略）… */
typedef struct ZipfileCDS ZipfileCDS;
/* …（同文件无关代码省略）… */
typedef struct ZipfileEntry ZipfileEntry;
/* …（同文件无关代码省略）… */
typedef struct ZipfileCsr ZipfileCsr;
/* …（同文件无关代码省略）… */
typedef struct ZipfileTab ZipfileTab;
/* …（同文件无关代码省略）… */
static void zipfileEntryFree(ZipfileEntry *p){
  if( p ){
    sqlite3_free(p->cds.zFile);
    sqlite3_free(p);
  }
}
/* …（同文件无关代码省略）… */
static void zipfileCleanupTransaction(ZipfileTab *pTab){
  ZipfileEntry *pEntry;
  ZipfileEntry *pNext;

  if( pTab->pWriteFd ){
    fclose(pTab->pWriteFd);
/* …（同文件无关代码省略）… */

  pCsr->bEof = 0;
  if( pCsr->pFile ){
    fclose(pCsr->pFile);
    pCsr->pFile = 0;
    zipfileEntryFree(pCsr->pCurrent);
    pCsr->pCurrent = 0;
  }

  for(p=pCsr->pFreeEntry; p; p=pNext){
    pNext = p->pNext;
/* …（同文件无关代码省略）… */
      pCsr->pCurrent = p;
    }
  }else{
    if( !pCsr->bNoop ){
      pCsr->pCurrent = pCsr->pCurrent->pNext;
    }
    if( pCsr->pCurrent==0 ){
/* …（同文件无关代码省略）… */
  int i                       /* Which column to return */
){
  ZipfileCsr *pCsr = (ZipfileCsr*)cur;
  ZipfileCDS *pCDS = &pCsr->pCurrent->cds;
  int rc = SQLITE_OK;
  switch( i ){
    case 0:   /* name */
      sqlite3_result_text(ctx, pCDS->zFile, -1, SQLITE_TRANSIENT);
/* …（同文件无关代码省略）… */
static ZipfileCsr *zipfileFindCursor(ZipfileTab *pTab, i64 iId){
  ZipfileCsr *pCsr;
  for(pCsr=pTab->pCsrList; pCsr; pCsr=pCsr->pCsrNext){
    if( iId==pCsr->iId ) break;
  }
  return pCsr;
}
/* …（同文件无关代码省略）… */
  assert( argc>0 );

  pCsr = zipfileFindCursor(pTab, sqlite3_value_int64(argv[0]));
  if( pCsr ){
    ZipfileCDS *p = &pCsr->pCurrent->cds;
    char *zRes = sqlite3_mprintf("{"
        "\"version-made-by\" : %u, "
