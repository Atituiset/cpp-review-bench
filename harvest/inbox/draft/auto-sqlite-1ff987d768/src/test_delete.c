// AUTO-DRAFT from sqlite/sqlite PR #be3dc6699ec31a45116750812a9567cdb0a84d7a
# define SQLITE_MULTIPLEX_WAL_8_3_OFFSET 700
/* …（同文件无关代码省略）… */
  int *pbExists
){
  int rc = SQLITE_ERROR;
#ifdef _WIN32
  if( pVfs ){
    if( pbExists ) *pbExists = 1;
    rc = pVfs->xDelete(pVfs, zFile, 0);
/* …（同文件无关代码省略）… */
    { "%s-wal%03d",     SQLITE_MULTIPLEX_WAL_8_3_OFFSET, 1 },
  };

#ifdef _WIN32
  sqlite3_vfs *pVfs = sqlite3_vfs_find("win32");
#else
  sqlite3_vfs *pVfs = 0;
