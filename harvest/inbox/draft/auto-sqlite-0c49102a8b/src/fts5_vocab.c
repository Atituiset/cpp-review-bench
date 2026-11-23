// AUTO-DRAFT from sqlite/sqlite PR #248b8a33f4fbacaecba2990a23ecb0a170ec342e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <string.h>
  // <<< BUG ANCHOR
  Fts5Global *pGlobal;            /* FTS5 global object for this database */
  int eType;                      /* FTS5_VOCAB_COL, ROW or INSTANCE */
  unsigned bBusy;                 /* True if busy */
};

struct Fts5VocabCursor {
/* …（同文件无关代码省略）… */
#define FTS5_VOCAB_COLUSED_MASK 0xFF


/*
** Translate a string containing an fts5vocab table type to an 
** FTS5_VOCAB_XXX constant. If successful, set *peType to the output
/* …（同文件无关代码省略）… */
      pRet->pGlobal = (Fts5Global*)pAux;
      pRet->eType = eType;
      pRet->db = db;
      pRet->zFts5Tbl = (char*)&pRet[1];
      pRet->zFts5Db = &pRet->zFts5Tbl[nTab];
      memcpy(pRet->zFts5Tbl, zTab, nTab);
/* …（同文件无关代码省略）… */
static int fts5VocabIsBinary(sqlite3_index_info *pInfo, int iCons){
  return 0==sqlite3_stricmp("binary", sqlite3_vtab_collation(pInfo, iCons));
}
/* …（同文件无关代码省略）… */
** identically, as are greater-than and greater-than-or-equal.
*/
static int fts5VocabBestIndexMethod(
  sqlite3_vtab *pUnused,
  sqlite3_index_info *pInfo
){
  int i;
  int iTermEq = -1;
  int iTermGe = -1;
  int iTermLe = -1;
  int idxNum = (int)pInfo->colUsed;
  int nArg = 0;

  UNUSED_PARAM(pUnused);

  assert( (pInfo->colUsed & FTS5_VOCAB_COLUSED_MASK)==pInfo->colUsed );

  for(i=0; i<pInfo->nConstraint; i++){
    struct sqlite3_index_constraint *p = &pInfo->aConstraint[i];
    if( p->usable==0 ) continue;
    if( p->iColumn==0 && fts5VocabIsBinary(pInfo, i) ){    /* term column */
      if( p->op==SQLITE_INDEX_CONSTRAINT_EQ ) iTermEq = i;
      if( p->op==SQLITE_INDEX_CONSTRAINT_LE ) iTermLe = i;
      if( p->op==SQLITE_INDEX_CONSTRAINT_LT ) iTermLe = i;
      if( p->op==SQLITE_INDEX_CONSTRAINT_GE ) iTermGe = i;
      if( p->op==SQLITE_INDEX_CONSTRAINT_GT ) iTermGe = i;
    }
  }

/* …（同文件无关代码省略）… */
  ** the "term" column (column 0). So if the user has requested this
  ** specifically - "ORDER BY term" or "ORDER BY term ASC" - set the
  ** sqlite3_index_info.orderByConsumed flag to tell the core the results
  ** are already in sorted order.  */
  if( pInfo->nOrderBy==1 
   && pInfo->aOrderBy[0].iColumn==0 
   && pInfo->aOrderBy[0].desc==0
  ){
