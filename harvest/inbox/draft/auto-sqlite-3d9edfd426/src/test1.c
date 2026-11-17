// AUTO-DRAFT from sqlite/sqlite PR #0d885a61d22b66bb5472616a1512665c9d05d59f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
  // <<< BUG ANCHOR
      }
      for(j=0; rc==SQLITE_OK && j<nData; j++){
        Tcl_Size n = 0;
        unsigned char *v = Tcl_GetByteArrayFromObj(objv[i+i], &n);
        a[j].iov_len = (size_t)n;
        a[j].iov_base = sqlite3_malloc64( n );
        if( a[j].iov_base==0 ){
/* …（同文件无关代码省略）… */
      if( xDel!=SQLITE_TRANSIENT ) xDel = SQLITE_STATIC;
      isTransient = 0;
      isStatic = 0;
      break;
    }
  }
