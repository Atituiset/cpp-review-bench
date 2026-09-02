// AUTO-DRAFT from sqlite/sqlite PR #62c805e36c43e015dce80f3a5d15ac528abf2584
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <ctype.h>
#include <string.h>
  // <<< BUG ANCHOR
  zIn = sqlite3_sql(pStmt);
  if( zIn==0 ) return 0;
  nIn = strlen(zIn);
  while( nIn>0 && (isspace(zIn[nIn-1]) || zIn[nIn-1]==';') ) nIn--;
  if( strchr(zIn, '?') ) return 0;
  pTest = sqlite3_str_new(0);
  sqlite3_str_appendf(pTest, "SELECT %s* FROM (",  
/* …（同文件无关代码省略）… */
    const char *zColName = sqlite3_column_name(pBase,i);
    const char *zSuffix = zColName ? strrchr(zColName, ':') : 0;
    if( zSuffix 
     && isdigit(zSuffix[1])
     && (zSuffix[1]>'3' || isdigit(zSuffix[2]))
    ){
      /* This is a randomized column name and so cannot be used in the
      ** WHERE clause. */
