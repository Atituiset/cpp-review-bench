// AUTO-DRAFT from sqlite/sqlite PR #6e5c04ce5ffe1984f37b25e19b020d7021fffb56
static void errorMPrintf(sqlite3_context *pCtx, const char *zFmt, ...){
  sqlite3 *db = sqlite3_context_db_handle(pCtx);
  char *zErr = 0;
  va_list ap;
  va_start(ap, zFmt);
  zErr = sqlite3VMPrintf(db, zFmt, ap);
  va_end(ap);
  if( zErr ){
    sqlite3_result_error(pCtx, zErr, -1);
    sqlite3DbFree(db, zErr);
  }else{
    sqlite3_result_error_nomem(pCtx);
  }
}
/* …（同文件无关代码省略）… */
            errorMPrintf(ctx, "constraint may not be dropped: %s", zCons);
            return;
          }
          iEnd = iOff;
          break;
        }
