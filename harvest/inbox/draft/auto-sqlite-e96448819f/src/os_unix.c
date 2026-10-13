// AUTO-DRAFT from sqlite/sqlite PR #9de75a1245e3d6a66b8283673719f92830a38c3c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
  // <<< BUG ANCHOR
# define HAVE_MREMAP 0
/* …（同文件无关代码省略）… */
#define osMmap ((void*(*)(void*,size_t,int,int,int,off_t))aSyscall[22].pCurrent)
/* …（同文件无关代码省略）… */
#define osMunmap ((int(*)(void*,size_t))aSyscall[23].pCurrent)
/* …（同文件无关代码省略）… */
#define osMremap ((void*(*)(void*,size_t,size_t,int,...))aSyscall[24].pCurrent)
/* …（同文件无关代码省略）… */
#define osGetpagesize ((int(*)(void))aSyscall[25].pCurrent)
/* …（同文件无关代码省略）… */
#endif

  if( pOrig ){
#if HAVE_MREMAP
    i64 nReuse = pFd->mmapSize;
    pNew = osMremap(pOrig, nReuse, nNew, MREMAP_MAYMOVE);
    zErr = "mremap";
#else
    const int szSyspage = osGetpagesize();
    i64 nReuse = (pFd->mmapSize & ~(szSyspage-1));
    u8 *pReq = &pOrig[nReuse];
    pNew = osMmap(pReq, nNew-nReuse, flags, MAP_SHARED, h, nReuse);
    if( pNew!=MAP_FAILED ){
      if( pNew!=pReq ){
        osMunmap(pNew, nNew - nReuse);
        pNew = 0;
      }else{
        pNew = pOrig;
      }
    }
#endif

    /* The attempt to extend the existing mapping failed. Free it. */
    if( pNew==MAP_FAILED || pNew==0 ){
      osMunmap(pOrig, nReuse);
    }
  }
