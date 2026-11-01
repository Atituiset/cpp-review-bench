// AUTO-DRAFT from sqlite/sqlite PR #98ddcd30c0e3c3f4f732ca4fc5692be0306a4224
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>

static int vdbeSafety(Vdbe *p){
  if( p->db==0 ){
    sqlite3_log(SQLITE_MISUSE, "API called with finalized prepared statement");
    return 1;
  }else{
    return 0;
  }
}
/* …（同文件无关代码省略）… */
static SQLITE_NOINLINE void invokeProfileCallback(sqlite3 *db, Vdbe *p){
  sqlite3_int64 iNow;
  sqlite3_int64 iElapse;
  assert( p->startTime>0 );
  assert( db->init.busy==0 );
  assert( p->zSql!=0 );
  sqlite3OsCurrentTimeInt64(db->pVfs, &iNow);
  iElapse = (iNow - p->startTime)*1000000;
#ifndef SQLITE_OMIT_DEPRECATED
  if( db->xProfile ){
    db->xProfile(db->pProfileArg, p->zSql, iElapse);
  }
#endif
  if( db->mTrace & SQLITE_TRACE_PROFILE ){
    db->trace.xV2(SQLITE_TRACE_PROFILE, db->pTraceArg, p, (void*)&iElapse);
  }
  p->startTime = 0;
}
/* …（同文件无关代码省略）… */
# define checkProfileCallback(DB,P) \
   if( ((P)->startTime)>0 ){ invokeProfileCallback(DB,P); }
/* …（同文件无关代码省略）… */
    Vdbe *v = (Vdbe*)pStmt;
    sqlite3 *db = v->db;
    if( vdbeSafety(v) ) return SQLITE_MISUSE_BKPT;
    sqlite3_mutex_enter(db->mutex);
    checkProfileCallback(db, v);
    assert( v->eVdbeState>=VDBE_READY_STATE );
