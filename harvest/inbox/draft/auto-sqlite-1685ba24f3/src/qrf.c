// AUTO-DRAFT from sqlite/sqlite PR #e8cbe2c96a69b2c674947f2efff4c7d1ada8b66e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
  // <<< BUG ANCHOR
int sqlite3_qrf_ckformat(const char *z){
  size_t n;
  if( z==0 ) return 0;
  if( z[0]!='%' ) return 0;
  z++;
  n = strspn(z,"+-,#0");
  if( n>5 ) return 0;
  z += n;
  if( z[0]>='1' && z[0]<='9' ){
    n = strspn(z,"0123456789");
    if( n>3 ) return 0;
    z += n;
  }
  if( z[0]=='.' ){
    z++;
    n = strspn(z,"0123456789");
    if( n>3 ) return 0;
    z += n;
  }
  n = strspn(z,"eEgGf");
  if( n==1 && z[1]==0 ) return 2;
  n = strspn(z,"dxXo");
  if( n==1 && z[1]==0 ) return 1;
  return 0;
}
/* …（同文件无关代码省略）… */
  if( p->spec.eTitle>QRF_TEXT_Relaxed ) p->spec.eTitle = QRF_Auto;
  if( p->spec.eBlob>QRF_BLOB_Size ) p->spec.eBlob = QRF_Auto;
qrf_reinit:
  switch( p->spec.eStyle ){
    case QRF_Auto: {
      switch( sqlite3_stmt_isexplain(pStmt) ){
/* …（同文件无关代码省略）… */
    case QRF_STYLE_Json: {
      p->spec.eText = QRF_TEXT_Json;
      p->spec.zNull = "null";
      break;
    }
    case QRF_STYLE_Html: {
/* …（同文件无关代码省略）… */
      if( p->spec.zTableName==0 || p->spec.zTableName[0]==0 ){
        p->spec.zTableName = "tab";
      }
      p->u.nIns = 0;
      break;
    }
/* …（同文件无关代码省略）… */
    p->zFmt[n+2] = 0;
    p->spec.zIFmt = p->zFmt;
  }
  if( p->spec.zFpFmt && sqlite3_qrf_ckformat(p->spec.zFpFmt)!=2 ){
    p->spec.zFpFmt = 0;
  }
}

/*
