// AUTO-DRAFT from sqlite/sqlite PR #449144b917b46c449cb491a9e5566731b69d1956
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
  p1 = sqlite3_value_int64(argv[1]);
  if( p0type==SQLITE_BLOB ){
    len = sqlite3_value_bytes(argv[0]);
    z = sqlite3_value_blob(argv[0]);
    if( z==0 ) return;
    assert( len==sqlite3_value_bytes(argv[0]) );
  }else{
    z = sqlite3_value_text(argv[0]);
