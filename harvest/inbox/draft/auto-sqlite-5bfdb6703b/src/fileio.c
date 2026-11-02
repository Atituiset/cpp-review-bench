// AUTO-DRAFT from sqlite/sqlite PR #e4f7bc615fd7f77f28a8ba50cd41f4231e7bfff3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#  define STRUCT_STAT struct stat
/* …（同文件无关代码省略）… */
#  define chmod(path,mode) fileio_chmod(path,mode)
#  define mkdir(path,mode) fileio_mkdir(path)
/* …（同文件无关代码省略）… */
# define sqlite3_fopen fopen
/* …（同文件无关代码省略）… */
static wchar_t *winUtf8To16(const char *zText){
  int nChar;
  wchar_t *zWideText;

  nChar = MultiByteToWideChar(CP_UTF8, 0, zText, -1, NULL, 0);
  if( nChar==0 ){
    return 0;
  }
  zWideText = sqlite3_malloc64(nChar*sizeof(WCHAR) );
  if( zWideText==0 ){
    return 0;
  }
  nChar = MultiByteToWideChar(CP_UTF8, 0, zText, -1, zWideText,
                                nChar);
  if( nChar==0 ){
    sqlite3_free(zWideText);
    zWideText = 0;
  }
  return zWideText;
}
/* …（同文件无关代码省略）… */
static int fileio_chmod(const char *zPath, int pmode){
  int rc;
  wchar_t *b1 = winUtf8To16(zPath);
  if( b1==0 ) return -1;
  rc = _wchmod(b1, pmode);
  sqlite3_free(b1);
  return rc;
}
/* …（同文件无关代码省略）… */
static int fileio_mkdir(const char *zPath){
  int rc;
  wchar_t *b1 = winUtf8To16(zPath);
  if( b1==0 ) return -1;
  rc = _wmkdir(b1);
  sqlite3_free(b1);
  return rc;
}
/* …（同文件无关代码省略）… */
static sqlite3_uint64 fileTimeToUnixTime(
  LPFILETIME pFileTime
){
  SYSTEMTIME epochSystemTime;
  ULARGE_INTEGER epochIntervals;
  FILETIME epochFileTime;
  ULARGE_INTEGER fileIntervals;

  memset(&epochSystemTime, 0, sizeof(SYSTEMTIME));
  epochSystemTime.wYear = 1970;
  epochSystemTime.wMonth = 1;
  epochSystemTime.wDay = 1;
  SystemTimeToFileTime(&epochSystemTime, &epochFileTime);
  epochIntervals.LowPart = epochFileTime.dwLowDateTime;
  epochIntervals.HighPart = epochFileTime.dwHighDateTime;

  fileIntervals.LowPart = pFileTime->dwLowDateTime;
  fileIntervals.HighPart = pFileTime->dwHighDateTime;

  return (fileIntervals.QuadPart - epochIntervals.QuadPart) / 10000000;
}
/* …（同文件无关代码省略）… */
static int fileStat(
  const char *zPath,
  STRUCT_STAT *pStatBuf
){
#if defined(_WIN32)
  int rc;
  wchar_t *b1 = winUtf8To16(zPath);
  if( b1==0 ) return 1;
  rc = _wstat(b1, pStatBuf);
  if( rc==0 ){
    HANDLE hFindFile;
    WIN32_FIND_DATAW fd;
    memset(&fd, 0, sizeof(WIN32_FIND_DATAW));
    hFindFile = FindFirstFileW(b1, &fd);
    if( hFindFile!=NULL ){
      pStatBuf->st_ctime = (time_t)fileTimeToUnixTime(&fd.ftCreationTime);
      pStatBuf->st_atime = (time_t)fileTimeToUnixTime(&fd.ftLastAccessTime);
      pStatBuf->st_mtime = (time_t)fileTimeToUnixTime(&fd.ftLastWriteTime);
      FindClose(hFindFile);
    }
  }
  sqlite3_free(b1);
  return rc;
#else
  return stat(zPath, pStatBuf);
#endif
}
/* …（同文件无关代码省略）… */
static int makeDirectory(
  const char *zFile
){
  char *zCopy = sqlite3_mprintf("%s", zFile);
  int rc = SQLITE_OK;

  if( zCopy==0 ){
    rc = SQLITE_NOMEM;
  }else{
    int nCopy = (int)strlen(zCopy);
    int i = 1;

    while( rc==SQLITE_OK ){
      STRUCT_STAT sStat;
      int rc2;

      for(; zCopy[i]!='/' && i<nCopy; i++);
      if( i==nCopy ) break;
      zCopy[i] = '\0';

      rc2 = fileStat(zCopy, &sStat);
      if( rc2!=0 ){
        if( mkdir(zCopy, 0777) ) rc = SQLITE_ERROR;
      }else{
        if( !S_ISDIR(sStat.st_mode) ) rc = SQLITE_ERROR;
      }
      zCopy[i] = '/';
      i++;
    }

    sqlite3_free(zCopy);
  }

  return rc;
}
/* …（同文件无关代码省略）… */
static int writeFile(
  sqlite3_context *pCtx,          /* Context to return bytes written in */
  const char *zFile,              /* File to write */
  sqlite3_value *pData,           /* Data to write */
  mode_t mode,                    /* MODE parameter passed to writefile() */
  sqlite3_int64 mtime             /* MTIME parameter (or -1 to not set time) */
){
  if( zFile==0 ) return 1;
#if !defined(_WIN32) && !defined(WIN32)
  if( S_ISLNK(mode) ){
    const char *zTo = (const char*)sqlite3_value_text(pData);
    if( zTo==0 ) return 1;
    unlink(zFile);
    if( symlink(zTo, zFile)<0 ) return 1;
  }else
#endif
  {
    if( S_ISDIR(mode) ){
      if( mkdir(zFile, mode) ){
        /* The mkdir() call to create the directory failed. This might not
        ** be an error though - if there is already a directory at the same
        ** path and either the permissions already match or can be changed
        ** to do so using chmod(), it is not an error.  */
        STRUCT_STAT sStat;
        if( errno!=EEXIST
         || 0!=fileStat(zFile, &sStat)
         || !S_ISDIR(sStat.st_mode)
         || ((sStat.st_mode&0777)!=(mode&0777) && 0!=chmod(zFile, mode&0777))
        ){
          return 1;
        }
      }
    }else{
      sqlite3_int64 nWrite = 0;
      const char *z;
      int rc = 0;
      FILE *out = sqlite3_fopen(zFile, "wb");
      if( out==0 ) return 1;
      z = (const char*)sqlite3_value_blob(pData);
      if( z ){
        sqlite3_int64 n = fwrite(z, 1, sqlite3_value_bytes(pData), out);
        nWrite = sqlite3_value_bytes(pData);
        if( nWrite!=n ){
          rc = 1;
        }
      }
      fclose(out);
      if( rc==0 && mode && chmod(zFile, mode & 0777) ){
        rc = 1;
      }
      if( rc ) return 2;
      sqlite3_result_int64(pCtx, nWrite);
    }
  }

  if( mtime>=0 ){
#if defined(_WIN32)
    /* Windows */
    FILETIME lastAccess;
    FILETIME lastWrite;
    SYSTEMTIME currentTime;
    LONGLONG intervals;
    HANDLE hFile;
    LPWSTR zUnicodeName;

    GetSystemTime(&currentTime);
    SystemTimeToFileTime(&currentTime, &lastAccess);
    intervals = (mtime*10000000) + 116444736000000000;
    lastWrite.dwLowDateTime = (DWORD)intervals;
    lastWrite.dwHighDateTime = intervals >> 32;
    zUnicodeName = winUtf8To16(zFile);
    if( zUnicodeName==0 ){
      return 1;
    }
    hFile = CreateFileW(
      zUnicodeName, FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS, NULL
    );
    sqlite3_free(zUnicodeName);
    if( hFile!=INVALID_HANDLE_VALUE ){
      BOOL bResult = SetFileTime(hFile, NULL, &lastAccess, &lastWrite);
      CloseHandle(hFil
