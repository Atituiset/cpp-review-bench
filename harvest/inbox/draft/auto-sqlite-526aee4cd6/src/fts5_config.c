// AUTO-DRAFT from sqlite/sqlite PR #a105f448ef55340802960e02c1df0ca2aa5e34ed
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
  va_start(ap, zFmt);
  zMsg = sqlite3_vmprintf(zFmt, ap);
  if( pConfig->pzErrmsg ){
    assert( *pConfig->pzErrmsg==0 );
    *pConfig->pzErrmsg = zMsg;
  }else{
    sqlite3_free(zMsg);
