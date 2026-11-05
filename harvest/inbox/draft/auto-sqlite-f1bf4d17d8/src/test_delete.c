// AUTO-DRAFT from sqlite/sqlite PR #01abbada3031c6abc97b1be8777acecda1ceb1af
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>

*/

#ifndef SQLITE_OS_WIN
#  include <unistd.h>
#  include <errno.h>
#endif
/* …（同文件无关代码省略）… */
# define SQLITE_MULTIPLEX_WAL_8_3_OFFSET 700
/* …（同文件无关代码省略）… */
  int *pbExists
){
  int rc = SQLITE_ERROR;
#ifdef SQLITE_OS_WIN
  if( pVfs ){
    if( pbExists ) *pbExists = 1;
    rc = pVfs->xDelete(pVfs, zFile, 0);
/* …（同文件无关代码省略）… */
    { "%s-wal%03d",     SQLITE_MULTIPLEX_WAL_8_3_OFFSET, 1 },
  };

#ifdef SQLITE_OS_WIN
  sqlite3_vfs *pVfs = sqlite3_vfs_find("win32");
#else
  sqlite3_vfs *pVfs = 0;
