// AUTO-DRAFT from sqlite/sqlite PR #695a96ebf04069a5f9917830414d7f3574ba6ff9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
      assert( j<=n/2 );
      zCopy[j] = 0;
      rc = sqlite3AtoF(zCopy, pValue);
      if( i<n ) rc = -100;
      sqlite3DbFree(pMem->db, zCopy);
    }
    return rc;
