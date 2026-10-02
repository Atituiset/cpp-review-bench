// AUTO-DRAFT from sqlite/sqlite PR #277113a4748fd28b1ad704bde15fcd2b4dc7aa2c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
    if( db->nVdbeActive ){
      sqlite3ErrorWithMsg(db, SQLITE_BUSY,
        "unable to delete/modify user-function due to active statements");
      assert( !db->mallocFailed );
      return SQLITE_BUSY;
    }else{
      sqlite3ExpirePreparedStatements(db, 0);
