// AUTO-DRAFT from sqlite/sqlite PR #6cdee32bc5cd6dc1e63dd35ab00559bbc9026b11
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <string.h>
  // <<< BUG ANCHOR
  struct iovec {
    void *iov_base;
    size_t iov_len;
  };
/* …（同文件无关代码省略）… */
      unsigned char *z = (unsigned char*)&p[nData];
      for(i=0; i<nData; i++){
        size_t n = ((struct iovec*)aData)[i].iov_len;
        p[i].iov_len = n;
        p[i].iov_base = z;
        z += n;
        memcpy(p[i].iov_base, ((struct iovec*)aData)[i].iov_base, n);
      }
    }else if( sz>0 ){
      memcpy(pNew->aData, aData, sz);
