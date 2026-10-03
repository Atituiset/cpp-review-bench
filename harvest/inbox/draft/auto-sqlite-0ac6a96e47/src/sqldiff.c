// AUTO-DRAFT from sqlite/sqlite PR #ac5f698ccbaba677323cc56f31131ef4d5fad693
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef sqlite3_int64 i64;
/* …（同文件无关代码省略）… */
static char *safeId(const char *zId){
  int i, x;
  char c;
  if( zId[0]==0 ) return sqlite3_mprintf("\"\"");
  for(i=x=0; (c = zId[i])!=0; i++){
    if( !isalpha(c) && c!='_' ){
      if( i>0 && isdigit(c) ){
        x++;
      }else{
        return sqlite3_mprintf("\"%w\"", zId);
      }
    }
  }
  if( x || !sqlite3_keyword_check(zId,i) ){
    return sqlite3_mprintf("%s", zId);
  }
  return sqlite3_mprintf("\"%w\"", zId);
}
/* …（同文件无关代码省略）… */
  int truePk = 0;          /* PRAGMA table_info identifies the PK to use */
  i64 nPK = 0;             /* Number of PRIMARY KEY columns */
  i64 i, j;                /* Loop counters */

  if( g.bSchemaPK==0 ){
    /* Normal case:  Figure out what the true primary key is for the table.
/* …（同文件无关代码省略）… */
  }
  while( SQLITE_ROW==sqlite3_step(pStmt) ){
    char * sid = safeId((char*)sqlite3_column_text(pStmt,1));
    int iPKey;
    if( truePk && (iPKey = sqlite3_column_int(pStmt,5))>0 ){
      az[iPKey-1] = sid;
    }else{
      if( !g.bSchemaCompare
          || !(strcmp(sid,"rootpage")==0
