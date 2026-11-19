// AUTO-DRAFT from sqlite/sqlite PR #57eb96ac7d5bd8cd1bf1b71233b4c8e811e8c9ff
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
  // <<< BUG ANCHOR
#define osMunmap ((int(*)(void*,size_t))aSyscall[23].pCurrent)
/* …（同文件无关代码省略）… */
#endif
    for(i=0; i<p->nRegion; i+=nShmPerMap){
      if( p->hShm>=0 ){
        osMunmap(p->apRegion[i], p->szRegion);
      }else{
        sqlite3_free(p->apRegion[i]);
      }
