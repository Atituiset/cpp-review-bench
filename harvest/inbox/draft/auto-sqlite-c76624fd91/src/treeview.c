// AUTO-DRAFT from sqlite/sqlite PR #caa0b5886eaa4455146d34f747b74ae94523cf3a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdio.h>

void sqlite3TreeViewLine(TreeView *p, const char *zFormat, ...){
  va_list ap;
  int i;
  StrAccum acc;
  char zBuf[1000];
  sqlite3StrAccumInit(&acc, 0, zBuf, sizeof(zBuf), 0);
  if( p ){
    for(i=0; i<p->iLevel && i<(int)sizeof(p->bLine)-1; i++){
      sqlite3_str_append(&acc, p->bLine[i] ? "|   " : "    ", 4);
    }
    sqlite3_str_append(&acc, p->bLine[i] ? "|-- " : "'-- ", 4);
  }
  if( zFormat!=0 ){
    va_start(ap, zFormat);
    sqlite3_str_vappendf(&acc, zFormat, ap);
    va_end(ap);
    assert( acc.nChar>0 || acc.accError );
    sqlite3_str_append(&acc, "\n", 1);
  }
  sqlite3StrAccumFinish(&acc);
  fprintf(stdout,"%s", zBuf);
  fflush(stdout);
}
/* …（同文件无关代码省略）… */
      sqlite3TreeViewLine(pView,"ID \"%w\"", pExpr->u.zToken);
      break;
    }
#ifndef SQLITE_OMIT_CAST
    case TK_CAST: {
      /* Expressions of the form:   CAST(pLeft AS token) */
