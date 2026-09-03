// AUTO-DRAFT from sqlite/sqlite PR #e6082d077e7e96c91fa366219ae07a1c5ab0ce70
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <string.h>

static void *contextMalloc(sqlite3_context *context, i64 nByte){
  char *z;
  sqlite3 *db = sqlite3_context_db_handle(context);
  assert( nByte>0 );
  testcase( nByte==db->aLimit[SQLITE_LIMIT_LENGTH] );
  testcase( nByte==(i64)db->aLimit[SQLITE_LIMIT_LENGTH]+1 );
  if( nByte>db->aLimit[SQLITE_LIMIT_LENGTH] ){
    sqlite3_result_error_toobig(context);
    z = 0;
  }else{
    z = sqlite3Malloc(nByte);
    if( !z ){
      sqlite3_result_error_nomem(context);
    }
  }
  return z;
}
/* …（同文件无关代码省略）… */
static void trimFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  const unsigned char *zIn;         /* Input string */
  const unsigned char *zCharSet;    /* Set of characters to trim */
  unsigned int nIn;                 /* Number of bytes in input */
  int flags;                        /* 1: trimleft  2: trimright  3: trim */
  int i;                            /* Loop counter */
  unsigned int *aLen = 0;           /* Length of each character in zCharSet */
  unsigned char **azChar = 0;       /* Individual characters in zCharSet */
  int nChar;                        /* Number of characters in zCharSet */

  if( sqlite3_value_type(argv[0])==SQLITE_NULL ){
    return;
  }
  zIn = sqlite3_value_text(argv[0]);
  if( zIn==0 ) return;
  nIn = (unsigned)sqlite3_value_bytes(argv[0]);
  assert( zIn==sqlite3_value_text(argv[0]) );
  if( argc==1 ){
    static const unsigned lenOne[] = { 1 };
    static unsigned char * const azOne[] = { (u8*)" " };
    nChar = 1;
    aLen = (unsigned*)lenOne;
    azChar = (unsigned char **)azOne;
    zCharSet = 0;
  }else if( (zCharSet = sqlite3_value_text(argv[1]))==0 ){
    return;
  }else{
    const unsigned char *z;
    for(z=zCharSet, nChar=0; *z; nChar++){
      SQLITE_SKIP_UTF8(z);
    }
    if( nChar>0 ){
      azChar = contextMalloc(context,
                     ((i64)nChar)*(sizeof(char*)+sizeof(unsigned)));
      if( azChar==0 ){
        return;
      }
      aLen = (unsigned*)&azChar[nChar];
      for(z=zCharSet, nChar=0; *z; nChar++){
        azChar[nChar] = (unsigned char *)z;
        SQLITE_SKIP_UTF8(z);
        aLen[nChar] = (unsigned)(z - azChar[nChar]);
      }
    }
  }
  if( nChar>0 ){
    flags = SQLITE_PTR_TO_INT(sqlite3_user_data(context));
    if( flags & 1 ){
      while( nIn>0 ){
        unsigned int len = 0;
        for(i=0; i<nChar; i++){
          len = aLen[i];
          if( len<=nIn && memcmp(zIn, azChar[i], len)==0 ) break;
        }
        if( i>=nChar ) break;
        zIn += len;
        nIn -= len;
      }
    }
    if( flags & 2 ){
      while( nIn>0 ){
        unsigned int len = 0;
        for(i=0; i<nChar; i++){
          len = aLen[i];
          if( len<=nIn && memcmp(&zIn[nIn-len],azChar[i],len)==0 ) break;
        }
        if( i>=nChar ) break;
        nIn -= len;
      }
    }
    if( zCharSet ){
      sqlite3_free(azChar);
    }
  }
  sqlite3_result_text(context, (char*)zIn, nIn, SQLITE_TRANSIENT);
}
/* …（同文件无关代码省略）… */
static void filestatFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  sqlite3 *db = sqlite3_context_db_handle(context);
  const char *zDbName;
  sqlite3_str *pStr;
  Btree *pBtree;

  zDbName = (const char*)sqlite3_value_text(argv[0]);
  pBtree = sqlite3DbNameToBtree(db, zDbName);
  if( pBtree ){
    Pager *pPager;
    sqlite3_file *fd;
    int rc;
    sqlite3BtreeEnter(pBtree);
    pPager = sqlite3BtreePager(pBtree);
    assert( pPager!=0 );
    fd = sqlite3PagerFile(pPager);
    pStr = sqlite3_str_new(db);
    if( sqlite3_str_errcode(pStr) ){
      sqlite3_result_error_nomem(context);
    }else{
      sqlite3_str_append(pStr, "{\"db\":", 6);
      rc = sqlite3OsFileControl(fd, SQLITE_FCNTL_FILESTAT, pStr);
      if( rc ) sqlite3_str_append(pStr, "null", 4);
      fd = sqlite3PagerJrnlFile(pPager);
      if( fd && fd->pMethods!=0 ){
        sqlite3_str_appendall(pStr, ",\"journal\":");
        rc = sqlite3OsFileControl(fd, SQLITE_FCNTL_FILESTAT, pStr);
        if( rc ) sqlite3_str_append(pStr, "null", 4);
      }
      sqlite3_str_append(pStr, "}", 1);
      sqlite3_result_str(context, pStr, SQLITE_FINISH);
    }
    sqlite3BtreeLeave(pBtree);
  }else{
    sqlite3_result_text(context, "{}", 2, SQLITE_STATIC);
  }
}
#endif /* SQLITE_DEBUG || SQLITE_ENABLE_FILESTAT */

#ifdef SQLITE_DEBUG
/*
** Implementation of fpdecode(x,y,z) function.
/* …（同文件无关代码省略）… */
#endif
#if defined(SQLITE_DEBUG) || defined(SQLITE_ENABLE_FILESTAT)
    FUNCTION(sqlite_filestat,    1, 0, 0, filestatFunc     ),
#endif
    FUNCTION(ltrim,              1, 1, 0, trimFunc         ),
    FUNCTION(ltrim,              2, 1, 0, trimFunc         ),
