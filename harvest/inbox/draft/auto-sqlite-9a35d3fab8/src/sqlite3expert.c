// AUTO-DRAFT from sqlite/sqlite PR #94852686fec2ebcb353ad65aedc90443af2aae48
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef sqlite3_int64 i64;
/* …（同文件无关代码省略）… */
#define STRLEN  (int)strlen
/* …（同文件无关代码省略）… */
#define IDX_HASH_SIZE 1023
typedef struct IdxHashEntry IdxHashEntry;
typedef struct IdxHash IdxHash;
/* …（同文件无关代码省略）… */
static void *idxMalloc(int *pRc, i64 nByte){
  void *pRet;
  assert( *pRc==SQLITE_OK );
  assert( nByte>0 );
  pRet = sqlite3_malloc64(nByte);
  if( pRet ){
    memset(pRet, 0, nByte);
  }else{
    *pRc = SQLITE_NOMEM;
  }
  return pRet;
}
/* …（同文件无关代码省略）… */
static int idxHashString(const char *z, int n){
  unsigned int ret = 0;
  int i;
  for(i=0; i<n; i++){
    ret += (ret<<3) + (unsigned char)(z[i]);
  }
  return (int)(ret % IDX_HASH_SIZE);
}
/* …（同文件无关代码省略）… */
static int idxHashAdd(
  int *pRc, 
  IdxHash *pHash, 
  const char *zKey,
  const char *zVal
){
  int nKey = STRLEN(zKey);
  int iHash = idxHashString(zKey, nKey);
  int nVal = (zVal ? STRLEN(zVal) : 0);
  IdxHashEntry *pEntry;
  assert( iHash>=0 );
  for(pEntry=pHash->aHash[iHash]; pEntry; pEntry=pEntry->pHashNext){
    if( STRLEN(pEntry->zKey)==nKey && 0==memcmp(pEntry->zKey, zKey, nKey) ){
      return 1;
    }
  }
  pEntry = idxMalloc(pRc, sizeof(IdxHashEntry) + (i64)nKey+1 + (i64)nVal+1);
  if( pEntry ){
    pEntry->zKey = (char*)&pEntry[1];
    memcpy(pEntry->zKey, zKey, nKey);
    if( zVal ){
      pEntry->zVal = &pEntry->zKey[nKey+1];
      memcpy(pEntry->zVal, zVal, nVal);
    }
    pEntry->pHashNext = pHash->aHash[iHash];
    pHash->aHash[iHash] = pEntry;

    pEntry->pNext = pHash->pFirst;
    pHash->pFirst = pEntry;
  }
  return 0;
}
/* …（同文件无关代码省略）… */
      do{
        int i;
        char *zFind;
        for(i=0; zCols[i]; i++){
          h += ((h<<3) + zCols[i]);
        }
        sqlite3_free(zName);
        zName = sqlite3_mprintf("%s_idx_%08x", zTable, h);
        if( zName==0 ) break;
        /* Is is unique among table, view and index names? */
        zFmt = "SELECT count(*) FROM sqlite_schema WHERE name=%Q"
          " AND type in ('index','table','view')";
/* …（同文件无关代码省略）… */
          break;
        }
        ++collisions;
      }while( collisions<50 && zName!=0 );
      if( collisions ){
        /* This return means "Gave up trying to find a unique index name." */
        rc = SQLITE_BUSY_TIMEOUT;
      }else if( zName==0 ){
/* …（同文件无关代码省略）… */
            idxHashAdd(&rc, &p->hIdx, zName, zIdx);
          }
        }
        sqlite3_free(zName);
        sqlite3_free(zIdx);
      }
    }

    sqlite3_free(zCols);
