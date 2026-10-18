// AUTO-DRAFT from sqlite/sqlite PR #ea9d6e8419604047ff7b105421702f7654f88292
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
typedef unsigned char u8;
/* …（同文件无关代码省略）… */
  sqlite3_int64 iValue;      /* Current value */
  u8 bDesc;                  /* iStep is really negative */
  u8 bDone;                  /* True if stepped past last element */
};

/*
/* …（同文件无关代码省略）… */
static sqlite3_int64 add64(sqlite3_int64 a, sqlite3_uint64 b){
  sqlite3_uint64 x = *(sqlite3_uint64*)&a;
  x += b;
  return *(sqlite3_int64*)&x;
}
static sqlite3_int64 sub64(sqlite3_int64 a, sqlite3_uint64 b){
  sqlite3_uint64 x = *(sqlite3_uint64*)&a;
  x -= b;
  return *(sqlite3_int64*)&x;
}
/* …（同文件无关代码省略）… */
    pCur->bDone = 1;
  }else if( pCur->bDesc ){
    pCur->iValue = sub64(pCur->iValue, pCur->iStep);
    assert( pCur->iValue>=pCur->iTerm );
  }else{
    pCur->iValue = add64(pCur->iValue, pCur->iStep);
    assert( pCur->iValue<=pCur->iTerm );
  }
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
    if( idxNum & 0x40 ){
      iOffset = sqlite3_value_int64(argv[iArg++]);
    }
  }

  /* Narrow the range of iMin and iMax (the minimum and maximum outputs)
