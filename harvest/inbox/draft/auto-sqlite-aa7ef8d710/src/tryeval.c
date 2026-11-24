// AUTO-DRAFT from sqlite/sqlite PR #d169f045ba96514b362d33f8c84489dd37cdaceb
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
** production use.  The interface might change.  This file might go out
** of support and disappear at any moment.  Do not depend on this extension.
**
** Apparent from comments in the code, this extension is undocumented.
**
** This SQLite extension implements the SQL function tryeval(). The tryeval()
** function takes zero or more arguments, each of which is an SQL statement.
/* …（同文件无关代码省略）… */
** Update:  Also the expire(V) and expire(V,N) functions.  See comments
** below for details.
**
** The purpose of this function is to help evaluate recursive
** behavior, and expecially the bug report at
** sqlite.org/bugs/info/3577bc69c52237a4 (2026-10-25T19:26:00Z)
/* …（同文件无关代码省略）… */
  int bDone = 0;          /* True if a row with non-NULL first column seen */
  int rc;                 /* Result from sqlite3 API calls */
  int i;                  /* Loop counter */

  db = sqlite3_context_db_handle(context);
  for(i=0; !bDone && i<argc; i++){
    zSql = (const char*)sqlite3_value_text(argv[i]);
    if( zSql==0 ) continue;
    rc = sqlite3_prepare_v2(db, zSql, -1, &pStmt, 0);
    if( rc==SQLITE_OK ){
      while( (rc = sqlite3_step(pStmt))==SQLITE_ROW ){
/* …（同文件无关代码省略）… */
        bDone = 1;
        break;
      }
    }
    sqlite3_finalize(pStmt);
  }
}
