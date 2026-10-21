// AUTO-DRAFT from sqlite/sqlite PR #03c38c992ae0f01fd88957a1b55c254bc0d0f9bc
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct Qrf Qrf;
/* …（同文件无关代码省略）… */
static void qrfError(
  Qrf *p,                /* Query result state */
  int iCode,             /* Error code */
  const char *zFormat,   /* Message format (or NULL) */
  ...
){
  p->iErr = iCode;
  if( p->pzErr!=0 ){
    sqlite3_free(*p->pzErr);
    *p->pzErr = 0;
    if( zFormat ){
      va_list ap;
      va_start(ap, zFormat);
      *p->pzErr = sqlite3_vmprintf(zFormat, ap);
      va_end(ap);
    }
  }
}
/* …（同文件无关代码省略）… */
static void qrfOom(Qrf *p){
  qrfError(p, SQLITE_NOMEM, "out of memory");
}
/* …（同文件无关代码省略）… */
        qrfOom(p);
      }else{
        p->spec.pzOutput[0] = zCombined;
        memcpy(zCombined+sz, sqlite3_str_value(p->pOut), n+1);
      }
      sqlite3_str_free(p->pOut);
    }else{
