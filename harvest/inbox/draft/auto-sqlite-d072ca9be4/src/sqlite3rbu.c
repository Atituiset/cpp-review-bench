// AUTO-DRAFT from sqlite/sqlite PR #1e946f434429ebf27c5c567876cb47eee29dc5a6
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

# define SWAP(TYPE,A,B) {TYPE t=A; A=B; B=t;}
/* …（同文件无关代码省略）… */
typedef unsigned char u8;
/* …（同文件无关代码省略）… */
static char *rbuStrndup(const char *zStr, int *pRc){
  char *zRet = 0;

  if( *pRc==SQLITE_OK ){
    if( zStr ){
      size_t nCopy = strlen(zStr) + 1;
      zRet = (char*)sqlite3_malloc64(nCopy);
      if( zRet ){
        memcpy(zRet, zStr, nCopy);
      }else{
        *pRc = SQLITE_NOMEM;
      }
    }
  }

  return zRet;
}
/* …（同文件无关代码省略）… */
  if( bIndex==0 ) pIter->abIndexed = 0;
}


/*
** If they are not already populated, populate the pIter->azTblCol[],
/* …（同文件无关代码省略）… */
    int bRbuRowid = 0;            /* If input table has column "rbu_rowid" */
    int iOrder = 0;
    int iTnum = 0;

    /* Figure out the type of table this step will deal with. */
    assert( pIter->eType==0 );
/* …（同文件无关代码省略）… */
      );
    }

    /* Check that all non-HIDDEN columns in the destination table are also
    ** present in the input table. Populate the abTblPk[], azTblType[] and
    ** aiTblOrder[] arrays at the same time.
/* …（同文件无关代码省略）… */
          SWAP(char*, pIter->azTblCol[i], pIter->azTblCol[iOrder]);
        }

        pIter->azTblType[iOrder] = rbuStrndup(zType, &p->rc);
        assert( iPk>=0 );
        pIter->abTblPk[iOrder] = (u8)iPk;
