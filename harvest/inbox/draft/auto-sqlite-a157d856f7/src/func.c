// AUTO-DRAFT from sqlite/sqlite PR #5e19f7e4bb5d37afa41eaa30f0334721ec3aae41
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>

  }
}

/*
** Implementation of the substr() function.
**
/* …（同文件无关代码省略）… */
  }
  assert( p1>=0 && p2>=0 );
  if( p0type!=SQLITE_BLOB ){
    for( ; p1>0; p1--){
                    /*  vvvvvv----  See tag-20260418-01 */
      if( (u8)(z[0]-1)<(0x80-1) ){
