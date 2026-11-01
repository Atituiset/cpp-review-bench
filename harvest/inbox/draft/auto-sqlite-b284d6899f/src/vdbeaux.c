// AUTO-DRAFT from sqlite/sqlite PR #9e2a3083e165cf30d7d3b7d3ca4b268ac7cc3d22
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>

static void freeEphemeralFunction(sqlite3 *db, FuncDef *pDef){
  assert( db!=0 );
  if( (pDef->funcFlags & SQLITE_FUNC_EPHEM)!=0 ){
    sqlite3DbNNFreeNN(db, pDef);
  }
}
/* …（同文件无关代码省略）… */
}
static SQLITE_NOINLINE void freeP4FuncCtx(sqlite3 *db, sqlite3_context *p){
  assert( db!=0 );
  freeEphemeralFunction(db, p->pFunc);
  sqlite3DbNNFreeNN(db, p);
}
