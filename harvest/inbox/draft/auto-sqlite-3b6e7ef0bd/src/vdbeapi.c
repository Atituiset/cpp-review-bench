// AUTO-DRAFT from sqlite/sqlite PR #3bc2ad344b6b4121bfd9f3cc73d66c5cb72ce7fe
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
  int i;
  int rc = SQLITE_OK;
  Vdbe *p = (Vdbe*)pStmt;
#if SQLITE_THREADSAFE
  sqlite3_mutex *mutex;
#endif
/* …（同文件无关代码省略）… */
#if SQLITE_THREADSAFE
  mutex = p->db->mutex;
#endif
  sqlite3_mutex_enter(mutex);
  for(i=0; i<p->nVar; i++){
    sqlite3VdbeMemRelease(&p->aVar[i]);
    p->aVar[i].flags = MEM_Null;
  }
  assert( (p->prepFlags & SQLITE_PREPARE_SAVESQL)!=0 || p->expmask==0 );
  if( p->expmask ){
    p->expired = 1;
  }
  sqlite3_mutex_leave(mutex);
