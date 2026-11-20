// AUTO-DRAFT from sqlite/sqlite PR #6f9d43469b47de16a198e114e0a037a069dba69c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>
  // <<< BUG ANCHOR
typedef struct DState DState;
/* …（同文件无关代码省略）… */
  return zOut;
}

/*
** Send mprintf-formatted content to the output callback.
*/
/* …（同文件无关代码省略）… */
    return 0;
  }else{
    if( sqlite3_strglob("CREATE TABLE ['\"]*", zSql)==0 ){
      p->xCallback("CREATE TABLE IF NOT EXISTS ", p->pArg);
      p->xCallback(zSql+13, p->pArg);
    }else{
      p->xCallback(zSql, p->pArg);
    }
    p->xCallback(";\n", p->pArg);
  }

  if( strcmp(zType, "table")==0 ){
/* …（同文件无关代码省略）… */


/*
** Execute a query statement that will generate SQL output.  Print
** the result columns, comma-separated, on a line and then add a
** semicolon terminator to the end of that line.
**
** If the number of columns is 1 and that column contains text "--"
** then write the semicolon on a separate line.  That way, if a
** "--" comment occurs at the end of the statement, the comment
** won't consume the semicolon terminator.
*/
static void output_sql_from_query(
  DState *p,               /* Query context */
/* …（同文件无关代码省略）… */
){
  sqlite3_stmt *pSelect;
  int rc;
  int nResult;
  int i;
  const char *z;
  char *zSql;
  va_list ap;
  va_start(ap, zSelect);
/* …（同文件无关代码省略）… */
    return;
  }
  rc = sqlite3_step(pSelect);
  nResult = sqlite3_column_count(pSelect);
  while( rc==SQLITE_ROW ){
    z = (const char*)sqlite3_column_text(pSelect, 0);
    p->xCallback(z, p->pArg);
    for(i=1; i<nResult; i++){
      p->xCallback(",", p->pArg);
      p->xCallback((const char*)sqlite3_column_text(pSelect,i), p->pArg);
    }
    if( z==0 ) z = "";
    while( z[0] && (z[0]!='-' || z[1]!='-') ) z++;
    if( z[0] ){
      p->xCallback("\n;\n", p->pArg);
    }else{
      p->xCallback(";\n", p->pArg);
    }
    rc = sqlite3_step(pSelect);
  }
  rc = sqlite3_finalize(pSelect);
