// AUTO-DRAFT from sqlite/sqlite PR #d3d85df39e5ac1314974fb0b48389235b93e9e14
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
*/
static void readFileContents(sqlite3_context *ctx, const char *zName){
  FILE *in;
  sqlite3_int64 nIn;
  void *pBuf;
  sqlite3 *db;
  int mxBlob;

/* …（同文件无关代码省略）… */
    /* File does not exist or is unreadable. Leave the result set to NULL. */
    return;
  }
  fseek(in, 0, SEEK_END);
  nIn = ftell(in);
  rewind(in);
  db = sqlite3_context_db_handle(ctx);
  mxBlob = sqlite3_limit(db, SQLITE_LIMIT_LENGTH, -1);
  if( nIn>mxBlob ){
    sqlite3_result_error_code(ctx, SQLITE_TOOBIG);
    fclose(in);
    return;
  }
  pBuf = sqlite3_malloc64( nIn ? nIn : 1 );
  if( pBuf==0 ){
    sqlite3_result_error_nomem(ctx);
    fclose(in);
    return;
  }
  if( nIn==(sqlite3_int64)fread(pBuf, 1, (size_t)nIn, in) ){
    sqlite3_result_blob64(ctx, pBuf, nIn, sqlite3_free);
  }else{
    sqlite3_result_error_code(ctx, SQLITE_IOERR);
    sqlite3_free(pBuf);
  }
  fclose(in);
}
