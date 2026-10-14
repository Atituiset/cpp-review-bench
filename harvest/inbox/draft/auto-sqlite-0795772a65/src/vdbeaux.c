// AUTO-DRAFT from sqlite/sqlite PR #0fbd83dc99401dc6eb2630bc783efb698dbe9fd5
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>

  p->prepFlags = prepFlags;
  if( (prepFlags & SQLITE_PREPARE_SAVESQL)==0 ){
    p->expmask = 0;
  }
  assert( p->zSql==0 );
  p->zSql = sqlite3DbStrNDup(p->db, z, n);
